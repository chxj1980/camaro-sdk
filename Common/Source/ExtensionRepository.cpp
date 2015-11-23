#include "ExtensionRepository.h"

using namespace TopGear;

std::vector<std::array<uint8_t, 16>> TopGear::ExtensionRepository::Inventory;
ExtensionRepository::_init ExtensionRepository::initializer;

const std::array<uint8_t, 16> ExtensionRepository::DGXuCode =
//{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
{0x63, 0xC3, 0x58, 0x42,            /* 16 byte GUID */
0xD2, 0x4D, 0x11, 0xCC,
0xB3, 0x9C, 0xC2, 0xA1,
0x28, 0xDF, 0xA6, 0xAF};

//{C2B1CCAD - ABF6 - 48B8 - 8E37 - 32D4F3A3FEEC}
const std::array<uint8_t, 16> ExtensionRepository::EtronXuCode =
{ 0xad , 0xcc, 0xb1, 0xc2, 0xf6, 0xab, 0xb8, 0x48, 0x8e, 0x37, 0x32, 0xd4, 0xf3, 0xa3, 0xfe, 0xec };