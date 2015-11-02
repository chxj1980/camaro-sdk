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
			template<class U>
			static std::shared_ptr<IVideoStream> CreateInstance(U &device);
		private:
			static std::shared_ptr<IVideoStream> CreateVideoStreamReader(std::shared_ptr<IMSource> &source);
			CameraFactory() = default;
		protected:
			~CameraFactory() = default;
		};
	}
}
