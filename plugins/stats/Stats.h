#pragma once
#include <FLHook.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <plugin.h>

namespace Plugins::Stats
{
	//! Reflectable struct that contains the file path of the config file as well as the file to export stats to
	struct FileName : Reflectable
	{
		std::string File() override { return "config/stats.json"; }
		std::string StatsFile = "stats.json";
		std::string FilePath = "EXPORTS";
	};

	//! Global data for this plugin
	struct Global final
	{
		ReturnCode returncode = ReturnCode::Default;

		FileName jsonFileName;
		//! A map containing a shipId and the user friendly name
		std::map<ShipId, std::wstring> Ships;
	};
} // namespace Plugins::Stats
