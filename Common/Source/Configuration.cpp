#include "Configuration.h"


namespace TopGear
{
	const std::map<std::string, Camera> CameraMap = {
		{ "StandardUVC", Camera::StandardUVC },
		{ "Camaro", Camera::Camaro },
		{ "CamaroDual", Camera::CamaroDual },
		{ "Etron3D", Camera::Etron3D }
	};

	std::unique_ptr<Configuration> Configuration::nullObj;
	std::map<Camera, Configuration> Configuration::CameraConfigurations;

	Configuration &Configuration::NullObject()
	{
		if (nullObj == nullptr)
			nullObj = std::unique_ptr<Configuration>(new Configuration(0));
		return *nullObj;
	}

	size_t GetTypeHash(const std::string &str)
	{
		if (str == "byte")
			return typeid(uint8_t).hash_code();
		if (str == "ushort")
			return typeid(uint16_t).hash_code();
		if (str == "int")
			return typeid(int32_t).hash_code();
		if (str == "string")
			return typeid(std::string).hash_code();
		if (str == "data")
			return typeid(std::vector<uint8_t>).hash_code();
		return 0;
	}

	std::pair<std::string, XuControl> XuControl::Parse(Json::Value &value)
	{
		std::string name;
		auto & element = value["control-name"];
		if (element.isString())
			name = element.asString();
		element = value["control-code"];
		auto code = element.isNull() ? 0 : element.asInt();
		element = value["type"];
		auto hash = element.isNull() ? 0 : GetTypeHash(element.asString());
		element = value["attribute"];
		std::string attrs;
		if (element.isString())
			attrs = element.asString();
		element = value["big-endian"];
		auto be = false;
		if (element.isBool())
			be = element.asBool();
		auto pair = std::make_pair(name, XuControl(code, hash, attrs,be));
		
		element = value["payloads"];
		if (!element.isNull())
		{
			for (auto i = 0u; i < element.size(); ++i)
			{
				auto attr = element[i]["attribute"].asString();
				auto prefix = element[i]["prefix"];
				if (prefix.isArray())
				{
					std::vector<uint8_t> data;
					for (auto j = 0u; j < prefix.size(); ++j)
						data.emplace_back(prefix[j].asInt());
					//Verify prefix data
					pair.second.Payloads.emplace(attr, std::make_pair(true, data));
				}
				else if (prefix.isInt())
				{
					std::vector<uint8_t> data(prefix.asInt(), 0);
					//Ignore prefix data
					pair.second.Payloads.emplace(attr, std::make_pair(false, data));
				}
			}
		}
		return pair;
	}

	std::pair<std::string, RegisterControl> RegisterControl::Parse(Json::Value &value)
	{
		std::string name;
		auto & element = value["control-name"];
		if (!element.isNull())
			name = element.asString();
		element = value["attribute"];
		std::string attrs;
		if (!element.isNull())
			attrs = element.asString();
		auto pair = std::make_pair(name, RegisterControl(attrs));
		element = value["register-address"];
		if (!element.isNull())
		{
			pair.second.AddressArray.emplace_back(uint16_t(std::stoul(element.asString(), nullptr, 0)));
		}
		else
		{
			element = value["register-address-array"];
			if (!element.isNull())
				for (auto i = 0u; i < element.size(); ++i)
					pair.second.AddressArray.emplace_back(uint16_t(std::stoul(element[i].asString(), nullptr, 0)));
		}
		return pair;
	}

	bool Configuration::Parse(std::istream &stream)
	{
		Json::Value root;
		stream >> root;
		auto & element = root["xdl-version"];
		if (element.isNull())
			return false;
		auto ver = std::stof(element.asString());
		if (ver < 1.0f)
			return false;
		element = root["camera-name"];
		if (element.isNull())
			return false;

		element = root["register-control-code"];
		auto registerCode = element.isNull() ? 0 : element.asInt();

		//element = root["register-data-type"];
		//auto hash = element.isNull() ? 0 : GetTypeHash(element.asString());
		auto it = CameraMap.find(root["camera-name"].asString());
		if (it == CameraMap.end())
			return false;
		auto result = CameraConfigurations.emplace(it->second, Configuration(registerCode));
		if (result.second == false)
			return false;
		auto &config = (result.first)->second;

		element = root["xu-controls"];
		if (!element.isNull())
		{
			for (auto i = 0u; i < element.size(); ++i)
			{
				auto pair = XuControl::Parse(element[i]);
				if (pair.first.empty())
					continue;
				config.XuControls.emplace(pair.first, pair.second);
			}
		}

		element = root["sensors"];
		if (!element.isNull())
		{
			for (auto i = 0u; i < element.size(); ++i)
			{
				auto id = element[i]["sensor-identifier"];
				if (id.isNull())
					continue;
				auto name = id.asString();
				auto map = element[i]["register-controls"];
				if (id.isNull())
					continue;
				for (auto j = 0u; j < map.size(); ++j)
				{
					auto pair = RegisterControl::Parse(map[j]);
					if (pair.first.empty())
						continue;
					config.sensors[name].emplace(pair.first, pair.second);
				}
			}
		}

		//element = root["register-controls"];
		//if (!element.isNull())
		//{
		//	for (auto i = 0u; i < element.size(); ++i)
		//	{
		//		auto pair = RegisterControl::Parse(element[i]);
		//		if (pair.first.empty())
		//			continue;
		//		config.RegisterControls.emplace(pair.first, pair.second);
		//	}
		//}
		return true;
	}

	const RegisterMap *Configuration::QueryRegisterMap(const std::string &identifier) const
	{
		for(auto &item : sensors)
		{
			if (identifier.find(item.first) != std::string::npos)
				return &item.second;
		}
		return nullptr;
	}
}
