#pragma once

#include <mfapi.h>

namespace TopGear
{
	namespace Win
	{
		class System
		{
		public:
			static void Initialize()
			{
				CoInitializeEx(nullptr, COINIT_MULTITHREADED);
				MFStartup(MF_VERSION, MFSTARTUP_LITE);
			}

			static void Dispose()
			{
				try
				{
					MFShutdown();
				}
				catch(...)
				{ }
				CoUninitialize();
			}

			template<typename T>
			static void SafeRelease(T** pPtr)
			{
				if (*pPtr)
				{
					auto unknown = reinterpret_cast<IUnknown *>(*pPtr);
					if (unknown)
					{
						unknown->Release();
						*pPtr = nullptr;
					}
				}
			}
			~System() {}
		private:
			System() {}
		};
	}
}
