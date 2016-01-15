#include "ImageAnalysis.h"
#include <atomic>
#include <mutex>
#include <future>
#include <exception>
#include <cmath>
#include <map>
#include <vector>

#include "tbb/tbb.h"
#include "opencv2/opencv.hpp"

#include <fstream>

#ifdef _WIN32
#include <codecvt>
#include <cwchar>
#include <Windows.h>
#include <strsafe.h>
#elif defined(__linux__)
#include <unistd.h>
#include <dirent.h>
#endif

//#define MF_PI  3.1415927f   // pi
//#define MF_1_PI  0.31830988f
#define RAD2DEG  57.29578f

namespace TopGear
{
	int fast_abs(int value)
	{
		uint32_t temp = value >> 31;     // make a mask of the sign bit
		value ^= temp;                   // toggle the bits if value is negative
		value += temp & 1;               // add one if value was negative
		return value;
	}

	float gradient(uint8_t *pixel, int x, int y, uint32_t stride, float &max, int ch = 1)
	{
		max = 0;
		float g = 0;
		auto current = pixel[y*stride + x*ch];
		for (auto i = -1; i <= 1; ++i)
			for (auto j = -1; j <= 1; ++j)
			{
				if (i == 0 && j == 0)
					continue;
				auto val = float(fast_abs(int(current) - int(pixel[(y + i)*stride + (x + j)*ch])));
				if (fast_abs(i) + fast_abs(j) > 1)
					val *= 0.707107f;
				if (val > max)
					max = val;
				g += val;
			}
		return g;
	}

	void ImageAnalysis::ConvertRawGB12ToGray(uint16_t* raw, uint8_t* gray, int w, int h, uint8_t *rgb)
	{
		tbb::parallel_for(0, w*h, [&](int i) {
			raw[i] <<= 4;
		});
		cv::Mat src(h, w, CV_16UC1, raw);
		cv::Mat dst(h, w, CV_16UC1);
		cv::demosaicing(src, dst, cv::COLOR_BayerGB2GRAY);
		tbb::parallel_for(0, w*h, [&](int i) {
			uint8_t val = (reinterpret_cast<uint16_t *>(dst.data)[i]) >> 8;
			if (rgb)
				rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = val;
			gray[i] = val;
		});
	}

	void ImageAnalysis::ConvertGrayToRGB(uint8_t *gray, uint8_t* rgb, int w, int h)
	{
		//
		cv::Mat src(h, w, CV_8UC1);
		tbb::parallel_for(0, w*h, [&](int i) {
			src.data[i] = 255-gray[i];
		});

		cv::Mat dst(h, w, CV_8UC3, rgb);
		cv::applyColorMap(src, dst, cv::COLORMAP_JET);
		//cv::cvtColor(src, dst, cv::COLOR_HSV2RGB);
		//tbb::parallel_for(0, w*h, [&](int i) {
		//	dst.data[i * 3] = gray[i];
		//	dst.data[i * 3 + 1] = 0;
		//	dst.data[i * 3 + 2] = 255;
		//});
		//cv::cvtColor(src, dst, cv::COLOR_GRAY2RGB);
	}

	void ImageAnalysis::ConvertYUVToRGB(uint8_t *yuv2, uint8_t* rgb, int w, int h)
	{
		cv::Mat src(h, w, CV_8UC2, yuv2);
		cv::Mat dst(h, w, CV_8UC3, rgb);
		cv::cvtColor(src, dst, cv::COLOR_YUV2BGR_YUY2);
	}

	float ImageAnalysis::SharpnessInternal(uint8_t* data, int w, int h)
	{
		constexpr auto lamda1 = 0.6f;
		constexpr auto lamda2 = 1 - lamda1;

		std::atomic<uint32_t> sum1(0);
		std::atomic<uint32_t> sum2(0);
		//auto i=0,j=0;
		//#pragma omp parallel for shared(pixel) private(i,j)
		//for (i = 1; i < h - 1; ++i)
		tbb::parallel_for(1, h-1, [&](int i)
		{
			float t1 = 0, t2 = 0;
			for (auto j = 1; j < w - 1; ++j)
			{
				float max = 0;
				auto g = gradient(data, j, i, w, max);
				t1 += g;
				t2 += max;
			}
			//std::unique_lock<std::mutex> lock(count_mutex);
			sum1 += t1*100;
			sum2 += t2*100;
			//atomic_float_add(sum1, t1);
			//atomic_float_add(sum2, t2);
		});
		return (sum1*lamda1 + sum2*lamda2) / ((h - 2)*(w - 2) * 10);
	}

	void ImageAnalysis::FindImagePairs(std::vector<std::string>& filelist, const std::string &path)
	{
		std::map<std::string, uint8_t> filemap;
#ifdef _WIN32
		//setup converter
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

		auto dir = new wchar_t[MAX_PATH] {0};
		if (path.empty())
		{
			GetCurrentDirectory(MAX_PATH, dir);
		}
		else
		{
			std::wstring wide(path.begin(), path.end());
			wcsncpy(dir, wide.c_str(), wide.size());
		}
		StringCchCat(dir, MAX_PATH, TEXT("\\*.png"));

		WIN32_FIND_DATA ffd;
		auto hFind = FindFirstFile(dir, &ffd);
		if (hFind == INVALID_HANDLE_VALUE)
			return;
		do
		{
			if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
				(ffd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0)
			{
				auto filename(converter.to_bytes(ffd.cFileName));
				filename.resize(filename.size() - 4);
				auto ch = filename[filename.size() - 1];
				filename.resize(filename.size() - 1);
				if (filemap.find(filename) == filemap.end())
					filemap[filename] = 0;
				if (ch == 'L')
					filemap[filename] |= 1;
				else if (ch == 'R')
					filemap[filename] |= 2;
			}
		} while (FindNextFile(hFind, &ffd));
#elif defined(__linux__)
		//To do
#endif
		filelist.clear();
		for (auto &element : filemap)
		{
			if (element.second != 3)
				continue;
			filelist.emplace_back(element.first);
		}
	}

	float ImageAnalysis::Sharpness(std::shared_ptr<uint8_t> pixel, int w, int h)
	{
		//std::unique_ptr<uint8_t[]> pixel(data);
		return SharpnessInternal(pixel.get(), w, h);
	}

	ImageAnalysis::Result ImageAnalysis::Process(std::shared_ptr<uint8_t> pixel, int w, int h, int cb_x, int cb_y, bool fast) const
	{
		Result result;
		//std::unique_ptr<uint8_t[]> pixel(data);
		auto sharp = std::async(&SharpnessInternal, pixel.get(), w, h);
		std::vector<cv::Point2f> corners;
		cv::Mat img(h, w, CV_8UC1, pixel.get());
		auto flags = fast ? (cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK) :
			(cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
		auto found = cv::findChessboardCorners(img, cv::Size(cb_x, cb_y), corners, flags);
		if (found)
		{
			for (auto item : corners)
				result.Corners.emplace_back(std::make_pair(item.x, item.y));
		}
		result.Sharpness = sharp.get();
		return result;
	}

	bool ImageAnalysis::WriteImage(uint8_t* data, int w, int h, const std::string& filename, int ch) const
	{
		try
		{
			auto type = (ch == 1) ? CV_8UC1 : CV_8UC3;
			cv::Mat img(h, w, type, data);
			cv::imwrite(filename + ".png", img);
			if (WriteLog)
				WriteLog("File" + filename + ".png saved.");
		}
		catch(...)
		{
			if (WriteLog)
				WriteLog("Save file" + filename + ".png failed!");
			return false;
		}
		return true;
	}

	static bool FindCorners(const std::string filename, std::vector<cv::Point2f> &corners, const cv::Size &size)
	{
		auto src = cv::imread(filename);
		if (src.empty())
			return false;
		//if (imageSize != src.size())
		//	break;
		if (src.channels() == 3)
		{
			cv::Mat img;
			cv::cvtColor(src, img, cv::COLOR_BGR2GRAY);
			if (!findChessboardCorners(img, size, corners,
				cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE))
				return false;
			cornerSubPix(img, corners, cv::Size(11, 11), cv::Size(-1, -1),
				cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01));
		}
		else if (src.channels() == 1)
		{
			if (!findChessboardCorners(src, size, corners,
				cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE))
				return false;
			cornerSubPix(src, corners, cv::Size(11, 11), cv::Size(-1, -1),
				cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01));
		}
		else
			return false;
		return true;
	}

	ImageAnalysis::CalibrationResult ImageAnalysis::StereoCalibrate(CameraParameters& params, const std::string &dirpath) const
	{
		CalibrationResult result;
		result.Completed = true;

		std::vector<std::string> filelist;
		FindImagePairs(filelist, dirpath);

		// collect image pairs
		std::vector<std::vector<cv::Point2f> > imagePoints[2];
		std::vector<std::vector<cv::Point3f> > objectPoints;
		cv::Size imageSize(params.UserRes.first, params.UserRes.second);

		int nimages = filelist.size();
		imagePoints[0].resize(nimages);
		imagePoints[1].resize(nimages);

		try
		{
			auto count = 0;
			auto cbSize = cv::Size(params.ChessboardDimension.first, params.ChessboardDimension.second);
			for (auto &filename : filelist)
			{
				int k;
				for (k = 0; k < 2; k++) 
				{
					if (!FindCorners(dirpath + filename + (k == 0 ? "L.png" : "R.png"), imagePoints[k][count], cbSize))
						break;
				}
				if (k == 2)
					++count;
			}
			if (WriteLog)
				WriteLog(std::to_string(count) + " pairs of images have been successfully detected.");
			result.Pairs = nimages = count;
			if (nimages < 2)
			{
				if (WriteLog)
					WriteLog("Fail: Too little pairs to run the calibration");
				throw std::exception();
			}
			imagePoints[0].resize(nimages);
			imagePoints[1].resize(nimages);
			objectPoints.resize(nimages);

			for (auto i = 0; i < nimages; i++)
			{
				for (auto j = 0; j < params.ChessboardDimension.second; j++)
					for (auto k = 0; k < params.ChessboardDimension.first; k++)
						objectPoints[i].emplace_back(cv::Point3f(k * params.ChessboardSide, j * params.ChessboardSide, 0));
			}
			if (WriteLog)
				WriteLog("Running stereo calibration ...");

			cv::Mat cameraMatrix[2], distCoeffs[2];
			cameraMatrix[0] = initCameraMatrix2D(objectPoints, imagePoints[0], imageSize, 0);
			cameraMatrix[1] = initCameraMatrix2D(objectPoints, imagePoints[1], imageSize, 0);

			double f = params.LensFocus * 1000 / params.PixelSize;
			cameraMatrix[0].at<double>(0, 0) = f;
			cameraMatrix[0].at<double>(1, 1) = f;
			cameraMatrix[1].at<double>(0, 0) = f;
			cameraMatrix[1].at<double>(1, 1) = f;

			cv::Mat R, T, E, F;
			result.RMS = stereoCalibrate(objectPoints, imagePoints[0], imagePoints[1],
				cameraMatrix[0], distCoeffs[0], cameraMatrix[1], distCoeffs[1],
				imageSize, R, T, E, F,
				cv::CALIB_FIX_ASPECT_RATIO +
				//CALIB_ZERO_TANGENT_DIST +
				cv::CALIB_USE_INTRINSIC_GUESS +
				cv::CALIB_SAME_FOCAL_LENGTH +
				cv::CALIB_RATIONAL_MODEL,//+
				//CALIB_FIX_K3 + CALIB_FIX_K4 + CALIB_FIX_K5,
				cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-5));

			if (WriteLog)
				WriteLog("RMS error=" + std::to_string(result.RMS));

			// CALIBRATION QUALITY CHECK
			// because the output fundamental matrix implicitly
			// includes all the output information,
			// we can check the quality of calibration using the
			// epipolar geometry constraint: m2^t*F*m1=0
			double err = 0;
			auto npoints = 0;
			std::vector<cv::Vec3f> lines[2];
			for (auto i = 0; i < nimages; i++)
			{
				auto npt = int(imagePoints[0][i].size());
				cv::Mat imgpt[2];
				for (auto k = 0; k < 2; k++)
				{
					imgpt[k] = cv::Mat(imagePoints[k][i]);
					undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], cv::Mat(), cameraMatrix[k]);
					computeCorrespondEpilines(imgpt[k], k + 1, F, lines[k]);
				}
				for (auto j = 0; j < npt; j++)
				{
					double errij = abs(imagePoints[0][i][j].x*lines[1][j][0] +
						imagePoints[0][i][j].y*lines[1][j][1] + lines[1][j][2]) +
						abs(imagePoints[1][i][j].x*lines[0][j][0] +
							imagePoints[1][i][j].y*lines[0][j][1] + lines[0][j][2]);
					err += errij;
				}
				npoints += npt;
			}

			result.EpipolarError = err / npoints;
			if (WriteLog)
				WriteLog("Average epipolar error = " + std::to_string(result.EpipolarError));

			// save intrinsic parameters
			cv::FileStorage fs(dirpath + "intrinsics.yml", cv::FileStorage::WRITE);
			if (fs.isOpened())
			{
				fs << "M1" << cameraMatrix[0] << "D1" << distCoeffs[0] << "M2"
					<< cameraMatrix[1] << "D2" << distCoeffs[1];
				fs.release();
				if (WriteLog)
					WriteLog("Success to save the intrinsic parameters in '" + dirpath + "intrinsics.yml'");
			}
			else
			{
				if (WriteLog)
					WriteLog("Fail: Cannot save the intrinsic parameters");
				throw std::exception();
			}

			cv::Mat R1, R2, P1, P2, Q;
			cv::Rect validRoi[2];

			stereoRectify(cameraMatrix[0], distCoeffs[0], 
						  cameraMatrix[1], distCoeffs[1],
						  imageSize, R, T, R1, R2, P1, P2, Q,
						  cv::CALIB_ZERO_DISPARITY, 1, imageSize, &validRoi[0], &validRoi[1]);

			auto visibleRoi = validRoi[0] & validRoi[1];
			result.ROI[0] = visibleRoi.x;
			result.ROI[1] = visibleRoi.y;
			result.ROI[2] = visibleRoi.width;
			result.ROI[3] = visibleRoi.height;

			// save extrinsic parameters
			fs.open(dirpath + "extrinsics.yml", cv::FileStorage::WRITE);
			if (fs.isOpened())
			{
				fs << "R" << R << "T" << T << "R1" << R1 << "R2" << R2 << "P1" << P1
					<< "P2" << P2 << "Q" << Q;
				fs.release();
				if (WriteLog)
					WriteLog("Success to save the extrinsics parameters in '" + dirpath + "extrinsics.yml'");
			}
			else
			{
				if (WriteLog)
					WriteLog("Fail: Cannot save the extrinsic parameters");
				throw std::exception();
			}
			if (WriteLog)
				WriteLog("Stereo calibration done!");

			// OR ELSE HARTLEY'S METHOD
			// use intrinsic parameters of each camera, but
			// compute the rectification transformation directly
			// from the fundamental matrix
			//{
			//	std::vector<cv::Point2f> allimgpt[2];
			//	for (auto k = 0; k < 2; k++)
			//	{
			//		for (auto i = 0; i < nimages; i++)
			//			std::copy(imagePoints[k][i].begin(), imagePoints[k][i].end(), std::back_inserter(allimgpt[k]));
			//	}
			//	F = cv::findFundamentalMat(cv::Mat(allimgpt[0]), cv::Mat(allimgpt[1]), cv::FM_8POINT, 0, 0);
			//	cv::Mat H1, H2;
			//	stereoRectifyUncalibrated(cv::Mat(allimgpt[0]), cv::Mat(allimgpt[1]), F, imageSize, H1, H2, 3);

			//	R1 = cameraMatrix[0].inv()*H1*cameraMatrix[0];
			//	R2 = cameraMatrix[1].inv()*H2*cameraMatrix[1];
			//	P1 = cameraMatrix[0];
			//	P2 = cameraMatrix[1];
			//}

			cv::Mat rmap[2][2];

			//Precompute maps for cv::remap()
			initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize, CV_32FC1, rmap[0][0], rmap[0][1]);
			initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize, CV_32FC1, rmap[1][0], rmap[1][1]);

			//cv::Mat image[2];
			cv::Mat rimg;
			for (auto &filename : filelist)
			{
				auto image = cv::imread(dirpath + filename + "L.png" );
				//image[0] = cv::imread("test1.png");
				remap(image, rimg, rmap[0][0], rmap[0][1], CV_INTER_LINEAR);
				imwrite(dirpath + "left.png", rimg);
				image = cv::imread(dirpath + filename + "R.png");
				//image[1] = cv::imread("test2.png");
				remap(image, rimg, rmap[1][0], rmap[1][1], CV_INTER_LINEAR);
				imwrite(dirpath + "right.png", rimg);
				break;
			}

			std::fstream file;
			file.open(dirpath + "map.dat", std::ios::binary | std::ios::out | std::ios::trunc);

			file.write(reinterpret_cast<char *>(rmap[0][0].data), rmap[0][0].cols*rmap[0][0].rows*sizeof(float));
			file.write(reinterpret_cast<char *>(rmap[0][1].data), rmap[0][1].cols*rmap[0][1].rows*sizeof(float));
			file.write(reinterpret_cast<char *>(rmap[1][0].data), rmap[1][0].cols*rmap[1][0].rows*sizeof(float));
			file.write(reinterpret_cast<char *>(rmap[1][1].data), rmap[1][1].cols*rmap[1][1].rows*sizeof(float));

			file.close();

			////-- 1. Read the images
			//auto imgLeft = cv::imread("left.png", cv::IMREAD_GRAYSCALE);
			//auto imgRight = cv::imread("right.png", cv::IMREAD_GRAYSCALE);
			////-- And create the image in which we will save our disparities
			//auto imgDisparity16S = cv::Mat(imgLeft.rows, imgLeft.cols, CV_16S);
			//auto imgDisparity8U = cv::Mat(imgLeft.rows, imgLeft.cols, CV_8UC1);

			//if (imgLeft.empty() || imgRight.empty())
			//{
			//	std::cout << " --(!) Error reading images " << std::endl; 
			//	throw std::exception();
			//}

			////-- 2. Call the constructor for StereoBM
			//auto ndisparities = 16 * 5;   /**< Range of disparity */
			//auto SADWindowSize = 21; /**< Size of the block window. Must be odd */

			//auto sbm = cv::StereoBM::create(ndisparities, SADWindowSize);

			////-- 3. Calculate the disparity image
			//sbm->compute(imgLeft, imgRight, imgDisparity16S);

			////-- Check its extreme values
			//double minVal; double maxVal;

			//minMaxLoc(imgDisparity16S, &minVal, &maxVal);

			//printf("Min disp: %f Max value: %f \n", minVal, maxVal);

			////-- 4. Display it as a CV_8UC1 image
			//imgDisparity16S.convertTo(imgDisparity8U, CV_8UC1, 255 / (maxVal - minVal));

			////cv::namedWindow("Disparity", cv::WINDOW_NORMAL);
			////imshow("Disparity", imgDisparity8U);

			////-- 5. Save the image
			//imwrite("SBM_sample.png", imgDisparity16S);

			////get intersection of both rois or use target image roi, if you know the target image
			//cv::Rect visibleRoi = roiLeft & roiRight;
			//cv::Mat cDisp(disp, visibleRoi);

		}
		catch(...)
		{
			result.Completed = false;
		}
		
		return result;
	}

	void CameraFactor::Update(CameraParameters& params)
	{
		isValid = false;
		std::pair<int, int> maxRes(params.SensorSize.first * 1000 / params.PixelSize,
			params.SensorSize.second * 1000 / params.PixelSize);
		if (params.UserRes.first > maxRes.first || params.UserRes.second > maxRes.second)
			throw std::invalid_argument("User resolution exceed maximum possible value!");
		std::pair<float, float> actualSensor(params.UserRes.first * params.PixelSize / 1000,
			params.UserRes.second * params.PixelSize / 1000);
		auto diagonal(std::sqrt(actualSensor.first * actualSensor.first + actualSensor.second * actualSensor.second));
		auto apertureD(params.LensFocus / params.LensFSplash);

		//Calculate field of view
		alpha.AlphaD = 2*std::atan2(diagonal, 2 * params.LensFocus) * RAD2DEG;
		alpha.AlphaW = 2*std::atan2(actualSensor.first, 2 * params.LensFocus) * RAD2DEG;
		alpha.AlphaH = 2*std::atan2(actualSensor.second, 2 * params.LensFocus) * RAD2DEG;

		auto k = 2 * params.PixelSize * params.BlurRadius / (apertureD*params.LensFocus); //  k = 2R/Df
		std::pair<float, float> stereoRange(params.Baseline*params.LensFocus / diagonal,
			params.Baseline*params.LensFocus * 1000 / params.PixelSize);
		//Calculate depth of field
		dof = std::make_pair(1 / (k + 1 / params.TargetDistance),
			(1 / params.TargetDistance - k > 0) ? 1 / (1 / params.TargetDistance - k) : stereoRange.second);
		if (dof.first < stereoRange.first)
			dof.first = stereoRange.first;

		std::pair<float, float> targetView(actualSensor.first*params.TargetDistance / params.LensFocus,
			actualSensor.second*params.TargetDistance / params.LensFocus);
		std::pair<float, float> stereoView(targetView.first - params.Baseline, targetView.second);
		stereoViewInPixel = std::make_pair(int(stereoView.first*params.UserRes.first / targetView.first),
			int(stereoView.second*params.UserRes.second / targetView.second));
		targetSize = std::make_pair(stereoView.first / params.ShrinkCoeff, stereoView.second / params.ShrinkCoeff);

		auto squareW = targetSize.first / (params.ChessboardDimension.first + 1);
		auto squareH = targetSize.second / (params.ChessboardDimension.second + 1);

		if (squareW<squareH)
		{
			targetSize.second = squareW * (params.ChessboardDimension.second + 1);
			squareSide = squareW;
		}
		else
		{
			targetSize.first = squareH * (params.ChessboardDimension.first + 1);
			squareSide = squareH;
		}

		if (params.ChessboardSide>squareSide)
			throw std::invalid_argument("User target size exceed reference value!");

		if (params.ChessboardSide>0)
		{
			targetSize.first = params.ChessboardSide * (params.ChessboardDimension.first + 1);
			targetSize.second = params.ChessboardSide * (params.ChessboardDimension.second + 1);
			squareSide = params.ChessboardSide;
		}

		std::pair<int, int> targetInPixel(targetSize.first*params.UserRes.first / targetView.first,
			targetSize.second*params.UserRes.second / targetView.second);
		if (squareSide*params.UserRes.first / targetView.first<MIN_SQUARE_IN_PIXELS)
			throw std::invalid_argument("User target size too small!");

		minOffsetHeight = stereoView.second / 2;
		anglePan = std::asin((stereoView.first - targetSize.first) / 2 *
			std::cos(std::atan2(actualSensor.first, 2 * params.LensFocus)) /
			std::sqrt(targetSize.first*targetSize.first / 4 + params.TargetDistance*params.TargetDistance));
		angleTilt = std::asin((stereoView.second - targetSize.second) / 2 *
			std::cos(std::atan2(actualSensor.second, 2 * params.LensFocus)) /
			std::sqrt(targetSize.second*targetSize.second / 4 + params.TargetDistance*params.TargetDistance));
		calLocations.clear();
		auto center = std::make_pair(stereoViewInPixel.first / 2, stereoViewInPixel.second / 2);
		calLocations.push_back(center);
		auto deltaX = center.first - targetInPixel.first/2;// *std::cos(anglePan) / 2;
		auto deltaY = center.second - targetInPixel.second/2;// *std::cos(angleTilt) / 2;
		calLocations.emplace_back(std::make_pair(
			center.first + deltaX,
			center.second));
		calLocations.emplace_back(std::make_pair(
			center.first - deltaX,
			center.second));
		calLocations.emplace_back(std::make_pair(
			center.first,
			center.second + deltaY));
		calLocations.emplace_back(std::make_pair(
			center.first,
			center.second - deltaY));
		calLocations.emplace_back(std::make_pair(
			center.first + deltaX,
			center.second + deltaY));
		calLocations.emplace_back(std::make_pair(
			center.first - deltaX,
			center.second + deltaY));
		calLocations.emplace_back(std::make_pair(
			center.first + deltaX,
			center.second - deltaY));
		calLocations.emplace_back(std::make_pair(
			center.first - deltaX,
			center.second - deltaY));
		calLocations.emplace_back(std::make_pair(
			center.first + deltaX/2,
			center.second + deltaY/2));
		calLocations.emplace_back(std::make_pair(
			center.first - deltaX/2,
			center.second + deltaY/2));
		calLocations.emplace_back(std::make_pair(
			center.first + deltaX/2,
			center.second - deltaY/2));
		calLocations.emplace_back(std::make_pair(
			center.first - deltaX/2,
			center.second - deltaY/2));

		anglePan *= RAD2DEG;
		angleTilt *= RAD2DEG;
		isValid = true;
	}
}
