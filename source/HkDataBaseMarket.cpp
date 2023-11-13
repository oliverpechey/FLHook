﻿#include "Global.hpp"

bool LoadBaseMarket()
{
	INI_Reader ini;

	if (!ini.open("..\\data\\equipment\\market_misc.ini", false))
		return false;

	while (ini.read_header())
	{
		if (!ini.is_header("BaseGood"))
			continue;
		if (!ini.read_value())
			continue;
		if (!ini.is_value("base"))
			continue;

		const char* szBaseName = ini.get_value_string();
		BaseInfo* biBase = 0;
		for (auto& base : CoreGlobals::i()->allBases)
		{
			const char* szBN = base.scBasename.c_str();
			if (!ToLower(base.scBasename).compare(ToLower(szBaseName)))
			{
				biBase = &base;
				break;
			}
		}

		if (!biBase)
			continue; // base not found

		ini.read_value();

		biBase->lstMarketMisc.clear();
		if (!ini.is_value("MarketGood"))
			continue;

		do
		{
			DataMarketItem mi;
			const char* szEquipName = ini.get_value_string(0);
			mi.iArchId = CreateID(szEquipName);
			mi.fRep = ini.get_value_float(2);
			biBase->lstMarketMisc.push_back(mi);
		} while (ini.read_value());
	}

	ini.close();
	return true;
}
