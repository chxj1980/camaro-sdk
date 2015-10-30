#pragma once
#include "ICameraFactory.h"
#include "IMSource.h"
#include "IVideoStream.h"

namespace TopGear
{
	namespace Win
	{
		template<class T>
		class CameraFactory : public ICameraFactory<T> 
		{
		public:
			static std::shared_ptr<IVideoStream> CreateInstance(IGenericVCDeviceRef &device);
		private:
			static std::shared_ptr<IVideoStream> CreateVideoStreamReader(std::shared_ptr<IMSource> &source);
			CameraFactory() = default;
		protected:
			~CameraFactory() = default;
		};

		template<class T>
		class CameraComboFactory : public ICameraComboFactory<T>
		{
		public:
			static std::shared_ptr<IVideoStream> CreateInstance(std::vector<IGenericVCDeviceRef> &devices);
		private:
			CameraComboFactory() = default;
		protected:
			~CameraComboFactory() = default;
		};
	}
}
