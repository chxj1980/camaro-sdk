#pragma once
#include <map>
#include <string>
#include <typeinfo>
#include <vector>
#include <cstdint>
#include <istream>
#include "json.h"
#include "DeepCamAPI.h"

namespace TopGear
{
	class XuControl
	{
	public:
		const uint8_t Code;
		const size_t TypeHash;
		const std::string Attribute;
		const bool IsBigEndian;
		std::map<std::string, std::pair<bool, std::vector<uint8_t>>> Payloads;
		static std::pair<std::string, XuControl> Parse(Json::Value &value);
	private:
		explicit XuControl(uint8_t code, size_t hash, const std::string &attr,bool be)
			: Code(code),
			TypeHash(hash),
			Attribute(attr),
			IsBigEndian(be)
		{
		}
	};

	class RegisterControl
	{
	public:
		std::vector<uint16_t> AddressArray;
		const std::string Attribute;
		static std::pair<std::string, RegisterControl> Parse(Json::Value &value);
	private:
		explicit RegisterControl(const std::string &attr)
			: Attribute(attr)
		{
		}
	};

	typedef std::map<std::string, RegisterControl> RegisterMap;

	class Configuration
	{
	public:
		const uint8_t RegisterCode;
		//const size_t RegisterHash;
		std::map<std::string, XuControl> XuControls;
		const RegisterMap *QueryRegisterMap(const std::string &identifier) const;

		static std::map<Camera, Configuration> CameraConfigurations;
		static bool Parse(std::istream &stream);
		static Configuration &NullObject();
		virtual ~Configuration() = default;
	private:
		static std::unique_ptr<Configuration> nullObj;
		std::map<std::string, RegisterMap> sensors;
		explicit Configuration(uint8_t registerCode)
			: RegisterCode(registerCode)
		{
		}
	};


}
