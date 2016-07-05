#pragma once

#include <cstdint>
#include <string>
#include <typeinfo>
#include <memory>
#include "IVideoStream.h"


namespace TopGear
{
	class IPropertyData
	{
	public:
		virtual ~IPropertyData() = default;
		virtual size_t GetTypeHash() const = 0;
	};

	template<class T>
	class PropertyData final
		: public IPropertyData
	{
	public:
		PropertyData() {}
		explicit PropertyData(const T &t) : Payload(t) {}
		explicit PropertyData(T &&t) : Payload(t) {}

        virtual ~PropertyData() = default;

		size_t GetTypeHash() const override
		{
			return typeid(T).hash_code();
		}
		T Payload;
	};

	class IDeviceControl
	{
	public:
		virtual ~IDeviceControl() = default;

		virtual bool SetControl(std::string name, IPropertyData &val) = 0;
		virtual bool SetControl(std::string name, IPropertyData &&val) = 0;
		virtual bool GetControl(std::string name, IPropertyData &val) = 0;

		template<class T>
		static bool GetProperty(std::shared_ptr<IVideoStream> &stream, std::string name, T &value);

		template<class T>
		static bool SetProperty(std::shared_ptr<IVideoStream> &stream, std::string name, T &&value);
	};

	template <class T>
	bool IDeviceControl::GetProperty(std::shared_ptr<IVideoStream>& stream, std::string name, T &value)
	{
		auto deviceControl = std::dynamic_pointer_cast<IDeviceControl>(stream);
		if (deviceControl == nullptr)
			return false;
		PropertyData<T> data;
		if (deviceControl->GetControl(name, data))
		{
			value = std::move(data.Payload);
			return true;
		}
		return false;
	}

	template <class T>
	bool IDeviceControl::SetProperty(std::shared_ptr<IVideoStream>& stream, std::string name, T &&value)
	{
		auto deviceControl = std::dynamic_pointer_cast<IDeviceControl>(stream);
		if (deviceControl == nullptr)
			return false;
		return deviceControl->SetControl(name, PropertyData<T>(value));
	}
}
