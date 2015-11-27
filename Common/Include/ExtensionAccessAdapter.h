#pragma once
#include "IExtensionAccess.h"
#include "IDeviceControl.h"
#include <vector>
#include <string>
#include <type_traits>

namespace TopGear
{
	class ExtensionAccessAdapter final
	{
	public:
		explicit ExtensionAccessAdapter(std::shared_ptr<IExtensionAccess> &xu_access)
			: extension(xu_access)
		{}
		~ExtensionAccessAdapter() = default;

		template<typename T>
		static void ConvertOrder(T &val)
		{
			static_assert(std::is_arithmetic<T>::value, "Arithmetic Type Supported Only in ConvertOrder");
			auto len = sizeof(T);
			T result;
			auto pSource = reinterpret_cast<uint8_t *>(&val);
			auto pDest = reinterpret_cast<uint8_t *>(&result);
			for (auto i = 0; i < len; ++i)
				pDest[i] = pSource[len - i - 1];
			val = result;
		}

		template<typename T>
		typename std::enable_if<std::is_integral<T>::value, bool>::type 
			SetProperty(int code, IPropertyData &val, const uint8_t *prefix, size_t prefixLen, bool bigendian)
		{
			auto prop = dynamic_cast<PropertyData<T> *>(&val);
			if (prop == nullptr)
				return false;
			if (bigendian)
				ConvertOrder(prop->Payload);
			//No Prefix
			if (prefix == nullptr || prefixLen == 0)
				return extension->SetProperty(code, prop->Payload) == 0;
			//Has Prefix
			std::unique_ptr<uint8_t[]> wrapper;
				wrapper = std::unique_ptr<uint8_t[]>(new uint8_t[prefixLen + sizeof(T)]);
				std::memcpy(wrapper.get(), prefix, prefixLen);
				std::memcpy(wrapper.get() + prefixLen, &prop->Payload, sizeof(T));
			return extension->SetProperty(code, wrapper.get(), prefixLen + sizeof(T))==0;
		}

		template<typename T>
		typename std::enable_if<std::is_same<std::string, T>::value ||
								std::is_same<std::vector<uint8_t>, T>::value, bool>::type
			SetProperty(int code, IPropertyData &val, const uint8_t *prefix, size_t prefixLen, bool bigendian)
		{
			(void)bigendian;
			auto prop = dynamic_cast<PropertyData<T> *>(&val);
			if (prop == nullptr)
				return false;
			if (prefix == nullptr || prefixLen == 0)
				return extension->SetProperty(code,
					reinterpret_cast<const uint8_t *>(prop->Payload.data()), prop->Payload.size()) == 0;
			//Has Prefix
			std::unique_ptr<uint8_t[]> wrapper;
			wrapper = std::unique_ptr<uint8_t[]>(new uint8_t[prefixLen + prop->Payload.size()]);
			std::memcpy(wrapper.get(), prefix, prefixLen);
			std::memcpy(wrapper.get() + prefixLen, prop->Payload.data(), prop->Payload.size());
			return extension->SetProperty(code, wrapper.get(), prefixLen + sizeof(T)) == 0;
		}

		template<typename T>
		typename std::enable_if<std::is_integral<T>::value, bool>::type
			GetProperty(int code, IPropertyData &val, const uint8_t *prefix, size_t prefixLen, bool bigendian)
		{
			auto prop = dynamic_cast<PropertyData<T> *>(&val);
			if (prop == nullptr)
				return false;
			auto len = 0;
			auto data = extension->GetProperty(code, len);
			if (data == nullptr)
				return false;

			//Verify Prefix
			if (prefix != nullptr)
			{
				for (auto i = 0; i < prefixLen; ++i)
					if (data[i] != prefix[i])
						return false;
			}

			if (sizeof(T) < len - prefixLen)
				return false;
			std::memcpy(&prop->Payload, data.get() + prefixLen, len);
			if (bigendian)
				ConvertOrder(prop->Payload);
			return true;
		}

		template<typename T>
		typename std::enable_if<std::is_same<std::string, T>::value ||
								std::is_same<std::vector<uint8_t>, T>::value, bool>::type
			GetProperty(int code, IPropertyData &val, const uint8_t *prefix, size_t prefixLen, bool bigendian)
		{
			(void)bigendian;
			auto prop = dynamic_cast<PropertyData<T> *>(&val);
			if (prop == nullptr)
				return false;
			auto len = 0;
			auto data = extension->GetProperty(code, len);
			if (data == nullptr)
				return false;

			//Verify Prefix
			if (prefix != nullptr)
			{
				for (auto i = 0; i < prefixLen; ++i)
					if (data[i] != prefix[i])
						return false;
			}

			if (std::is_same<std::string, T>::value)
			{
				auto &str = *reinterpret_cast<std::string *>(&prop->Payload);
				str	= std::string(reinterpret_cast<char *>(data.get() + prefixLen));
			}
			else if (std::is_same<std::vector<uint8_t>, T>::value)
			{
				auto &vector = *reinterpret_cast<std::vector<uint8_t> *>(&prop->Payload);
				for (auto i = prefixLen; i < len; ++i)
					vector.emplace_back(data[i]);
			}
			return true;
		}

		IExtensionAccess &Source() { return *extension; }
	private:
		std::shared_ptr<IExtensionAccess> extension;
	};
}
