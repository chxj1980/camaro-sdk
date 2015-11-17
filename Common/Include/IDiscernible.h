#pragma once
#include <type_traits>
#include <memory>
#include "IValidation.h"


namespace TopGear
{
	template<class T>
	class IDiscernible
	{
		static_assert(std::is_base_of<IValidation, T>::value, "Template parameter T must derive from IValidation");
	public:
		virtual ~IDiscernible() = default;
		virtual const std::shared_ptr<T> &GetValidator() const = 0;
	};
}
