#pragma once
#include <memory>
#include <string>

namespace TopGear
{
	class IValidation
	{
	public:
		virtual ~IValidation() = default;
		virtual bool IsValid() const = 0;
	};

	class IGenericVCDevice;

	typedef std::shared_ptr<IGenericVCDevice> IGenericVCDeviceRef;

	class IGenericVCDevice
	{
	public:
		virtual std::string GetSymbolicLink() = 0;
		virtual std::string GetFriendlyName() = 0;
		virtual std::string GetDeviceInfo() = 0;
		virtual ~IGenericVCDevice() = default;
	};

	template<class T>
	class IDiscernibleVCDevice : public IGenericVCDevice
	{
		static_assert(std::is_base_of<IValidation, T>::value, "Class T must derive from IValidation");
	public:
		virtual ~IDiscernibleVCDevice() = default;
		virtual const std::shared_ptr<T> &GetValidator() const = 0;
	};
}
