#include "ImageAnalysis.h"
#include "tbb/tbb.h"
#include <atomic>
//#include <mutex>
#include <future>
//#include <QtCore/qlogging.h>

namespace TopGear
{
	inline int fast_abs(int value)
	{
		uint32_t temp = value >> 31;     // make a mask of the sign bit
		value ^= temp;                   // toggle the bits if value is negative
		value += temp & 1;               // add one if value was negative
		return value;
	}

	inline void atomic_float_add(std::atomic<float> &var, float val)
	{
		auto current = var.load();
		while (!var.compare_exchange_weak(current, current + val)) {}
	}

	float gradient(uint8_t *pixel, int x, int y, uint32_t stride, float &max)
	{
		max = 0;
		float g = 0;
		auto current = pixel[y*stride + x * 3];
		for (auto i = -1; i <= 1; ++i)
			for (auto j = -1; j <= 1; ++j)
			{
				if (i == 0 && j == 0)
					continue;
				auto val = float(fast_abs(int(current) - int(pixel[(y + i)*stride + (x + j) * 3])));
				if (fast_abs(i) + fast_abs(j)>1)
					val *= 0.707107f;
				if (val > max)
					max = val;
				g += val;
			}
		return g;
	}

	void ImageAnalysis::ConvertRawGB12ToGray(uint16_t* raw, uint8_t* rgb, int w, int h)
	{
		tbb::parallel_for(0, w*h, [&](int i) {
			raw[i] <<= 4;
		});
		cv::Mat src(h, w, CV_16UC1, raw);
		cv::Mat dst(h, w, CV_16UC1);
		cv::demosaicing(src, dst, cv::COLOR_BayerGB2GRAY);
		tbb::parallel_for(0, w*h, [&](int i) {
			uint8_t val = (reinterpret_cast<uint16_t *>(dst.data)[i]) >> 8;
			rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = val;
		});
	}

	ImageAnalysis::Result ImageAnalysis::Process(uint8_t *data, int w, int h)
	{
		std::unique_ptr<uint8_t[]> pixel(data);
		//std::lock_guard<std::mutex> lock(ev_mutex);
		Result result;

		std::vector<cv::Point2f> corners;
		auto valid = std::async([&]()
		{
			cv::Mat img(h, w, CV_8UC3, pixel.get());
			cv::Size size(9, 6);
			auto found = cv::findChessboardCorners(img, size, corners,
				cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE
				| cv::CALIB_CB_FAST_CHECK);
			//qDebug("C:  %d", corners.size());
			return found;
		});

		constexpr auto lamda1 = 0.6f;
		constexpr auto lamda2 = 1 - lamda1;

		std::atomic<float> sum1(0);
		std::atomic<float> sum2(0);
		//auto i=0,j=0;
		//#pragma omp parallel for shared(pixel) private(i,j)
		//for (i = 1; i < h - 1; ++i)
		tbb::parallel_for(1, h - 1, [&](int i) {
			float t1 = 0, t2 = 0;
			for (auto j = 1; j < w - 1; ++j)
			{
				float max = 0;
				auto g = gradient(pixel.get(), j, i, w * 3, max);
				t1 += g;
				t2 += max;
			}
			//std::unique_lock<std::mutex> lock(count_mutex);
			//sum1 += t1;
			//sum2 += t2;
			atomic_float_add(sum1, t1);
			atomic_float_add(sum2, t2);
		});
		result.Sharpness = (sum1*lamda1 + sum2*lamda2) / ((h - 2)*(w - 2)) * 10;
		//qDebug("P:  %f", s);
		//labelMag->setText(QString::number(s));

		if (valid.get())
		{
			for (auto item : corners)
				result.Corners.emplace_back(std::make_pair(int(item.x), int(item.y)));
		}

		//cv::cornerSubPix(img, corners, cv::Size(11, 11), cv::Size(-1, -1),
		//	cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));

		//OutputDebugStringW(L"Frame Arrival\n");
		//std::cout << "P: " << s << std::endl;
		return result;
	}

}
