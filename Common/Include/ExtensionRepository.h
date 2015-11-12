#pragma once

#include <vector>
#include <array>

namespace TopGear
{
	class ExtensionRepository
	{
	public:
		static std::vector<std::array<uint8_t, 16>> Inventory;
		class _init
		{
		public:
			_init()
			{
				Inventory.emplace_back(DGXuCode);
				Inventory.emplace_back(EtronXuCode);	
			}
		};
		static const std::array<uint8_t, 16> DGXuCode;
		static const std::array<uint8_t, 16> EtronXuCode;
	private:
		static _init initializer;
		ExtensionRepository() = default;
		~ExtensionRepository() = default;
	};
}
