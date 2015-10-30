#pragma once

#include <cstdint>
#include <memory>
// ReSharper disable once CppUnusedIncludeDirective
#include <cstring>

namespace TopGear
{
	class ExtensionInfo final
	{
	public:
		uint8_t UnitId = 0;
		uint8_t ExtensionCode[16];
		uint8_t NumControls = 0;
		uint8_t NrInPins = 0;
		std::unique_ptr<uint8_t[]> SourceIDs;
		uint8_t ControlSize = 0;
		std::unique_ptr<uint8_t[]> Controls;
		uint8_t Extension = 0;
		static std::shared_ptr<ExtensionInfo> Parse(uint8_t *data, int size);
		~ExtensionInfo() {}
	private:
		ExtensionInfo() {}
		
	};

	inline std::shared_ptr<ExtensionInfo> ExtensionInfo::Parse(uint8_t* data, int size)
	{
		auto base = 21;
		if (size < base)
			return nullptr;
		std::shared_ptr<ExtensionInfo> info(new ExtensionInfo);
		// ReSharper disable once CppRedundantQualifier
		info->UnitId = data[0];
		std::memcpy(&info->ExtensionCode, data+1, 0x10);
		info->NumControls = data[0x11];
		base += info->NrInPins = data[0x12];
		if (size < base)
			return nullptr;
		info->SourceIDs.reset(new uint8_t[info->NrInPins]);
		auto index = 0x13;
		for (auto i = 0; i < info->NrInPins; i++)
		{
			info->SourceIDs[i] = data[index++];
		}
		base += info->ControlSize = data[index++];
		if (size < base)
			return nullptr;
		info->Controls.reset(new uint8_t[info->ControlSize]);
		for (auto i = 0; i < info->ControlSize; i++)
		{
			info->Controls[i] = data[index++];
		}
		info->Extension = data[index];
		return info;
	}
}
