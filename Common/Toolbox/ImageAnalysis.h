#pragma once
#include <cstdint>
#include <memory>
#include "opencv2/opencv.hpp"
#include <future>
#include <mutex>

namespace TopGear
{
	class ImageAnalysis
	{
	public:

		class Result
		{
		public:
			float Sharpness;
			std::vector<std::pair<int,int>> Corners;
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

		static void ConvertRawGB12ToGray(uint16_t *raw, uint8_t *gray, int w, int h);
		static Result Process(uint8_t *data, int w, int h);
	};

}
