#pragma once
#include <array>
#include <memory>

namespace TopGear
{
	class IExtensionAccess
	{
	public:
		virtual ~IExtensionAccess() = default;
		template <size_t n>
		int SetProperty(int index, const std::array<uint8_t, n> &prop)
		{
			return SetProperty(index, prop.data(), n);
		}
		template <class T>
		int SetProperty(int index, T prop)
		{
			static_assert(std::is_fundamental<T>::value, "SetProperty need a fundamental type parameter.");
			return SetProperty(index, reinterpret_cast<uint8_t *>(&prop), sizeof(T));
		}
		virtual int SetProperty(int index, const uint8_t *data, size_t size) = 0;
		virtual std::unique_ptr<uint8_t[]> GetProperty(int index, int &len, bool dynamicLen = false) = 0;
	};
}


