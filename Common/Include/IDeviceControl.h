#pragma once

#include <cstdint>
#include <string>
#include <typeinfo>


namespace TopGear
{
	class IPropertyData;

	class IDeviceControl
	{
	public:
		virtual ~IDeviceControl() = default;

		virtual bool SetControl(std::string name, IPropertyData &val) = 0;
		virtual bool SetControl(std::string name, IPropertyData &&val) = 0;
		virtual bool GetControl(std::string name, IPropertyData &val) = 0;

		//virtual int SetSensorTrigger(uint8_t level) = 0;
		//virtual int SetResyncNumber(uint16_t resyncNum) = 0;
		//virtual int QueryDeviceRole() = 0;
		//virtual std::string QueryDeviceInfo() = 0;
	};

	class IPropertyData
	{
	public:
		virtual ~IPropertyData() = default;
		virtual size_t GetTypeHash() const = 0;
		//virtual size_t GetTypeSize() const = 0;
		//virtual void *Ptr() = 0;
	};

	template<class T>
	class PropertyData final
		: public IPropertyData
	{
	public:
		PropertyData() {}
		explicit PropertyData(T t) : Payload(t) {}
		explicit PropertyData(T &t) : Payload(t) {}
		virtual ~PropertyData() override {}
		virtual size_t GetTypeHash() const override
		{
			return typeid(T).hash_code();
		}
		//virtual size_t GetTypeSize() const override
		//{
		//	return sizeof(T);
		//}
		//virtual void *Ptr() override
		//{
		//	return &Payload;
		//}
		T Payload;
	};
}
