#pragma once

namespace TopGear
{
	class IValidation
	{
	public:
		virtual ~IValidation() = default;
		virtual bool IsValid() const = 0;
	};
}
