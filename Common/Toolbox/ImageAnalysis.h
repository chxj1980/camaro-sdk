#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <array>

namespace TopGear
{
	int fast_abs(int value);

	struct CameraParameters
	{
		std::pair<float, float> SensorSize;	//In mm
		float PixelSize;	//In um
		std::pair<int, int> UserRes;	//In pixels
		float LensFocus;	//In mm
		float LensFSplash;	
		float TargetDistance;		//In M
		float BlurRadius;	//In pixels
		float Baseline;		//In M
		float ShrinkCoeff;
		float ChessboardSide; //In M
		std::pair<int, int> ChessboardDimension;
	};

	struct FOV
	{
		float AlphaD;
		float AlphaW;
		float AlphaH;
	};

	class CameraFactor
	{
	public:
		void Update(CameraParameters &params);
		bool IsValid() const { return isValid; }
		std::pair<float, float> GetDOF() const { return dof; }
		std::pair<int, int> GetStereoViewInPixel() const { return stereoViewInPixel; }
		FOV GetFOV() const { return alpha; }
		std::vector<std::pair<int, int>> &GetCalLocations() { return calLocations; }
		void GetChessboard(float &square, std::pair<float, float> &size) const
		{
			square = squareSide;
			size = targetSize;
		}

		void GetPosture(float &minHeight, float &pan, float &tilt) const
		{
			minHeight = minOffsetHeight;
			pan = anglePan;
			tilt = angleTilt;
		}
	private:
		static const int MIN_SQUARE_IN_PIXELS = 20;
		bool isValid = false;
		float squareSide = 0; //In M
		std::pair<float, float> targetSize;	//In M
		std::pair<float, float> dof;
		std::pair<int, int> stereoViewInPixel;
		std::vector<std::pair<int, int>> calLocations;
		FOV alpha;
		float minOffsetHeight = 0;
		float anglePan = 0;
		float angleTilt = 0;
	};

	class ImageAnalysis
	{
	public:
		
		class Result
		{
		public:
			float Sharpness;
			std::vector<std::pair<float,float>> Corners;
			Result():Sharpness(0) {};
			~Result() {};
			Result &operator = (Result&&) = default;
			Result &operator = (const Result&) = delete;
			Result(Result&&) = default;
			Result(const Result&) = delete;
			void Reset()
			{
				Sharpness = 0;
				Corners.clear();
			}
		};

		struct CalibrationResult
		{
			bool Completed;
			int Pairs;
			double RMS;
			double EpipolarError;
			std::array<int, 4> ROI;
		};

		explicit ImageAnalysis(std::function<void(const std::string &)> &&logFn) 
			: WriteLog(logFn)
		{}
		ImageAnalysis() {}
		~ImageAnalysis() {}

		void ConvertGrayToRGB(uint8_t *gray, uint8_t* rgb, int w, int h);
		void ConvertYUVToRGB(uint8_t *yuv2, uint8_t* rgb, int w, int h);

		void ConvertRawGB12ToGray(uint16_t *raw, uint8_t *gray, int w, int h, uint8_t *rgb= nullptr);
		float Sharpness(std::shared_ptr<uint8_t> pixel, int w, int h);
		Result Process(std::shared_ptr<uint8_t> pixel, int w, int h, int cb_x, int cb_y, bool fast) const;
		bool WriteImage(uint8_t *data, int w, int h, const std::string &filename, int ch=3) const;
		CalibrationResult StereoCalibrate(CameraParameters& params, const std::string &dirpath ="") const;
	private:
		std::function<void(const std::string &)> WriteLog;
		static float SharpnessInternal(uint8_t *data, int w, int h);
		static void FindImagePairs(std::vector<std::string> &filelist, const std::string &path = "");
	};

}
