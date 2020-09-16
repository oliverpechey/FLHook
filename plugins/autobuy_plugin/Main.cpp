// Autobuy Plugin
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Includes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Main.h"

#define ADDR_COMMON_VFTABLE_MINE 0x139C64
#define ADDR_COMMON_VFTABLE_CM 0x139C90
#define ADDR_COMMON_VFTABLE_GUN 0x139C38

#define PRINT_ERROR() { for(uint i = 0; (i < sizeof(wscError)/sizeof(std::wstring)); i++) PrintUserCmdText(iClientID, L"%s", wscError[i].c_str()); return; }
#define PRINT_OK() PrintUserCmdText(iClientID, L"OK");
#define PRINT_DISABLED() PrintUserCmdText(iClientID, L"Command disabled");
#define GET_USERFILE(a) std::string a; { CAccount *acc = Players.FindAccountFromClientID(iClientID); std::wstring wscDir; HkGetAccountDirName(acc, wscDir); a = scAcctPath + wstos(wscDir) + "\\flhookuser.ini"; }

std::wstring wscError[] =
{
	L"/autobuy <param> [<on/off>]",
	L"<Param>:",
	L"   info - display current autobuy-settings",
	L"   missiles - enable/disable autobuy for missiles",
	L"   torps - enable/disable autobuy for torpedos",
	L"   mines - enable/disable autobuy for mines",
	L"   cd - enable/disable autobuy for cruise disruptors",
	L"   cm - enable/disable autobuy for countermeasures",
	L"   reload - enable/disable autobuy for nanobots/shield batteries",
	L"   all: enable/disable autobuy for all of the above",
	L"Examples:",
	L"\"/autobuy missiles on\" enable autobuy for missiles",
	L"\"/autobuy all off\" completely disable autobuy",
	L"\"/autobuy info\" show autobuy info",
};

struct AUTOBUYINFO {
	bool bAutoBuyMissiles;
	bool bAutoBuyMines;
	bool bAutoBuyTorps;
	bool bAutoBuyCD;
	bool bAutoBuyCM;
	bool bAutoBuyReload;
};

AUTOBUYINFO AutoBuyInfo[MAX_CLIENT_ID + 1];

struct AUTOBUY_CARTITEM
{
	uint iArchID;
	uint iCount;
	std::wstring wscDescription;
};

int HkPlayerAutoBuyGetCount(std::list<CARGO_INFO>& lstCargo, uint iItemArchID)
{
	for(auto& it : lstCargo)
	{
		if (it.iArchID == iItemArchID)
			return it.iCount;
	}

	return 0;
}

#define ADD_EQUIP_TO_CART(desc)	{ aci.iArchID = ((Archetype::Launcher*)eq)->iProjectileArchID; \
								aci.iCount = MAX_PLAYER_AMMO - HkPlayerAutoBuyGetCount(lstCargo, aci.iArchID); \
								aci.wscDescription = desc; \
								lstCart.push_back(aci); }

void UserCmd_AutoBuy(uint iClientID, const std::wstring& wscParam)
{
	std::wstring wscType = ToLower(GetParam(wscParam, ' ', 0));
	std::wstring wscSwitch = ToLower(GetParam(wscParam, ' ', 1));

	if (!wscType.compare(L"info"))
	{
		PrintUserCmdText(iClientID, L"Missiles: %s", AutoBuyInfo[iClientID].bAutoBuyMissiles ? L"On" : L"Off");
		PrintUserCmdText(iClientID, L"Mine: %s", AutoBuyInfo[iClientID].bAutoBuyMines ? L"On" : L"Off");
		PrintUserCmdText(iClientID, L"Torpedos: %s", AutoBuyInfo[iClientID].bAutoBuyTorps ? L"On" : L"Off");
		PrintUserCmdText(iClientID, L"Cruise Disruptors: %s", AutoBuyInfo[iClientID].bAutoBuyCD ? L"On" : L"Off");
		PrintUserCmdText(iClientID, L"Countermeasures: %s", AutoBuyInfo[iClientID].bAutoBuyCM ? L"On" : L"Off");
		PrintUserCmdText(iClientID, L"Nanobots/Shield Batteries: %s", AutoBuyInfo[iClientID].bAutoBuyReload ? L"On" : L"Off");
		return;
	}

	if (!wscType.length() || !wscSwitch.length() || ((wscSwitch.compare(L"on") != 0) && (wscSwitch.compare(L"off") != 0)))
		PRINT_ERROR();

	GET_USERFILE(scUserFile);

	std::wstring wscFilename;
	HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
	std::string scSection = "autobuy_" + wstos(wscFilename);

	bool bEnable = !wscSwitch.compare(L"on") ? true : false;
	if (!wscType.compare(L"all")) {
		AutoBuyInfo[iClientID].bAutoBuyMissiles = bEnable;
		AutoBuyInfo[iClientID].bAutoBuyMines = bEnable;
		AutoBuyInfo[iClientID].bAutoBuyTorps = bEnable;
		AutoBuyInfo[iClientID].bAutoBuyCD = bEnable;
		AutoBuyInfo[iClientID].bAutoBuyCM = bEnable;
		AutoBuyInfo[iClientID].bAutoBuyReload = bEnable;
		IniWrite(scUserFile, scSection, "missiles", bEnable ? "yes" : "no");
		IniWrite(scUserFile, scSection, "mines", bEnable ? "yes" : "no");
		IniWrite(scUserFile, scSection, "torps", bEnable ? "yes" : "no");
		IniWrite(scUserFile, scSection, "cd", bEnable ? "yes" : "no");
		IniWrite(scUserFile, scSection, "cm", bEnable ? "yes" : "no");
		IniWrite(scUserFile, scSection, "reload", bEnable ? "yes" : "no");
	}
	else if (!wscType.compare(L"missiles")) {
		AutoBuyInfo[iClientID].bAutoBuyMissiles = bEnable;
		IniWrite(scUserFile, scSection, "missiles", bEnable ? "yes" : "no");
	}
	else if (!wscType.compare(L"mines")) {
		AutoBuyInfo[iClientID].bAutoBuyMines = bEnable;
		IniWrite(scUserFile, scSection, "mines", bEnable ? "yes" : "no");
	}
	else if (!wscType.compare(L"torps")) {
		AutoBuyInfo[iClientID].bAutoBuyTorps = bEnable;
		IniWrite(scUserFile, scSection, "torps", bEnable ? "yes" : "no");
	}
	else if (!wscType.compare(L"cd")) {
		AutoBuyInfo[iClientID].bAutoBuyCD = bEnable;
		IniWrite(scUserFile, scSection, "cd", bEnable ? "yes" : "no");
	}
	else if (!wscType.compare(L"cm")) {
		AutoBuyInfo[iClientID].bAutoBuyCM = bEnable;
		IniWrite(scUserFile, scSection, "cm", bEnable ? "yes" : "no");
	}
	else if (!wscType.compare(L"reload")) {
		AutoBuyInfo[iClientID].bAutoBuyReload = bEnable;
		IniWrite(scUserFile, scSection, "reload", bEnable ? "yes" : "no");
	}
	else
		PRINT_ERROR();

	PRINT_OK();
}

void HkPlayerAutoBuy(uint iClientID, uint iBaseID)
{
	// player cargo
	int iRemHoldSize;
	std::list<CARGO_INFO> lstCargo;
	HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

	// shopping cart
	std::list<AUTOBUY_CARTITEM> lstCart;

	if (AutoBuyInfo[iClientID].bAutoBuyReload)
	{ // shield bats & nanobots
		Archetype::Ship* ship = Archetype::GetShip(Players[iClientID].iShipArchetype);

		uint iNanobotsID;
		pub::GetGoodID(iNanobotsID, "ge_s_repair_01");
		uint iRemNanobots = ship->iMaxNanobots;
		uint iShieldBatsID;
		pub::GetGoodID(iShieldBatsID, "ge_s_battery_01");
		uint iRemShieldBats = ship->iMaxShieldBats;
		bool bNanobotsFound = false;
		bool bShieldBattsFound = false;
		for(auto& it : lstCargo)
		{
			AUTOBUY_CARTITEM aci;
			if (it.iArchID == iNanobotsID) {
				aci.iArchID = iNanobotsID;
				aci.iCount = ship->iMaxNanobots - it.iCount;
				aci.wscDescription = L"Nanobots";
				lstCart.push_back(aci);
				bNanobotsFound = true;
			}
			else if (it.iArchID == iShieldBatsID) {
				aci.iArchID = iShieldBatsID;
				aci.iCount = ship->iMaxShieldBats - it.iCount;
				aci.wscDescription = L"Shield Batteries";
				lstCart.push_back(aci);
				bShieldBattsFound = true;
			}
		}

		if (!bNanobotsFound)
		{ // no nanos found -> add all
			AUTOBUY_CARTITEM aci;
			aci.iArchID = iNanobotsID;
			aci.iCount = ship->iMaxNanobots;
			aci.wscDescription = L"Nanobots";
			lstCart.push_back(aci);
		}

		if (!bShieldBattsFound)
		{ // no batts found -> add all
			AUTOBUY_CARTITEM aci;
			aci.iArchID = iShieldBatsID;
			aci.iCount = ship->iMaxShieldBats;
			aci.wscDescription = L"Shield Batteries";
			lstCart.push_back(aci);
		}
	}

	if (AutoBuyInfo[iClientID].bAutoBuyCD || AutoBuyInfo[iClientID].bAutoBuyCM || AutoBuyInfo[iClientID].bAutoBuyMines ||
		AutoBuyInfo[iClientID].bAutoBuyMissiles || AutoBuyInfo[iClientID].bAutoBuyTorps)
	{
		// add mounted equip to a new list and eliminate double equipment(such as 2x lancer etc)
		std::list<CARGO_INFO> lstMounted;
		for (auto& it : lstCargo)
		{
			if (!it.bMounted)
				continue;

			bool bFound = false;
			for (auto& it2 : lstMounted)
			{
				if (it2.iArchID == it.iArchID)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
				lstMounted.push_back(it);
		}

		uint iVFTableMines = (uint)hModCommon + ADDR_COMMON_VFTABLE_MINE;
		uint iVFTableCM = (uint)hModCommon + ADDR_COMMON_VFTABLE_CM;
		uint iVFTableGun = (uint)hModCommon + ADDR_COMMON_VFTABLE_GUN;

		// check mounted equip
		for (auto& it2 : lstMounted)
		{
			uint i = it2.iArchID;
			AUTOBUY_CARTITEM aci;
			Archetype::Equipment* eq = Archetype::GetEquipment(it2.iArchID);
			EQ_TYPE eq_type = HkGetEqType(eq);
			if (eq_type == ET_MINE)
			{
				if (AutoBuyInfo[iClientID].bAutoBuyMines)
					ADD_EQUIP_TO_CART(L"Mines")
			}
			else if (eq_type == ET_CM)
			{
				if (AutoBuyInfo[iClientID].bAutoBuyCM)
					ADD_EQUIP_TO_CART(L"Countermeasures")
			}
			else if (eq_type == ET_TORPEDO)
			{
				if (AutoBuyInfo[iClientID].bAutoBuyTorps)
					ADD_EQUIP_TO_CART(L"Torpedos")
			}
			else if (eq_type == ET_CD)
			{
				if (AutoBuyInfo[iClientID].bAutoBuyCD)
					ADD_EQUIP_TO_CART(L"Cruise Disruptors")
			}
			else if (eq_type == ET_MISSILE)
			{
				if (AutoBuyInfo[iClientID].bAutoBuyMissiles)
					ADD_EQUIP_TO_CART(L"Missiles")
			}
		}
	}

	// search base in base-info list
	BASE_INFO* bi = 0;
	for (auto& it3 : lstBases)
	{
		if (it3.iBaseID == iBaseID)
		{
			bi = &(it3);
			break;
		}
	}

	if (!bi)
		return; // base not found

	int iCash;
	HkGetCash(ARG_CLIENTID(iClientID), iCash);

	for (auto& it4 : lstCart)
	{
		if (!(it4).iCount || !Arch2Good((it4).iArchID))
			continue;

		// check if good is available and if player has the neccessary rep
		bool bGoodAvailable = false;
		for (auto& itmi : bi->lstMarketMisc)
		{
			if (itmi.iArchID == it4.iArchID)
			{
				// get base rep
				int iSolarRep;
				pub::SpaceObj::GetSolarRep(bi->iObjectID, iSolarRep);
				uint iBaseRep;
				pub::Reputation::GetAffiliation(iSolarRep, iBaseRep);
				if (iBaseRep == -1)
					continue; // rep can't be determined yet(space object not created yet?)

				// get player rep
				int iRepID;
				pub::Player::GetRep(iClientID, iRepID);

				// check if rep is sufficient
				float fPlayerRep;
				pub::Reputation::GetGroupFeelingsTowards(iRepID, iBaseRep, fPlayerRep);
				if (fPlayerRep < itmi.fRep)
					break; // bad rep, not allowed to buy
				bGoodAvailable = true;
				break;
			}
		}

		if (!bGoodAvailable)
			continue; // base does not sell this item or bad rep

		float fPrice;
		if (pub::Market::GetPrice(iBaseID, it4.iArchID, fPrice) == -1)
			continue; // good not available

		Archetype::Equipment* eq = Archetype::GetEquipment(it4.iArchID);
		if (iRemHoldSize < (eq->fVolume * it4.iCount))
		{
			uint iNewCount = (uint)(iRemHoldSize / eq->fVolume);
			if (!iNewCount) {
				continue;
			}
			else
				it4.iCount = iNewCount;
		}

		int iCost = ((int)fPrice * it4.iCount);
		if (iCash < iCost)
			PrintUserCmdText(iClientID, L"Auto-Buy(%s): FAILED! Insufficient Credits", it4.wscDescription.c_str());
		else {
			HkAddCash(ARG_CLIENTID(iClientID), -iCost);
			iCash -= iCost;
			iRemHoldSize -= ((int)eq->fVolume * it4.iCount);
			HkAddCargo(ARG_CLIENTID(iClientID), it4.iArchID, it4.iCount, false);
			PrintUserCmdText(iClientID, L"Auto-Buy(%s): Bought %u unit(s), cost: %s$", it4.wscDescription.c_str(), it4.iCount, ToMoneyStr(iCost).c_str());
		}
	}
}

void __stdcall BaseEnter_AFTER(unsigned int iBaseID, unsigned int iClientID)
{
	returncode = DEFAULT_RETURNCODE;

	CAccount* acc = Players.FindAccountFromClientID(iClientID);
	std::wstring wscDir;
	HkGetAccountDirName(acc, wscDir);
	std::string scUserFile = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";

	// read autobuy
	std::wstring wscFilename;
	HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
	std::string scSection = "autobuy_" + wstos(wscFilename);

	AutoBuyInfo[iClientID].bAutoBuyMissiles = IniGetB(scUserFile, scSection, "missiles", false);
	AutoBuyInfo[iClientID].bAutoBuyMines = IniGetB(scUserFile, scSection, "mines", false);
	AutoBuyInfo[iClientID].bAutoBuyTorps = IniGetB(scUserFile, scSection, "torps", false);
	AutoBuyInfo[iClientID].bAutoBuyCD = IniGetB(scUserFile, scSection, "cd", false);
	AutoBuyInfo[iClientID].bAutoBuyCM = IniGetB(scUserFile, scSection, "cm", false);
	AutoBuyInfo[iClientID].bAutoBuyReload = IniGetB(scUserFile, scSection, "reload", false);

	try {
		HkPlayerAutoBuy(iClientID, iBaseID);
	}
	catch (HK_ERROR e) {
		AddLog("Autobuy error: %s", HkErrGetText(e));
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

USERCMD UserCmds[] =
{
	{ L"/autobuy",		UserCmd_AutoBuy},
};

EXPORT bool UserCmd_Process(uint iClientID, const std::wstring& wscCmd)
{

	std::wstring wscCmdLower = ToLower(wscCmd);


	for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++)
	{
		if (wscCmdLower.find(ToLower(UserCmds[i].wszCmd)) == 0)
		{
			std::wstring wscParam = L"";
			if (wscCmd.length() > wcslen(UserCmds[i].wszCmd))
			{
				if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
					continue;
				wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
			}
			UserCmds[i].proc(iClientID, wscParam);

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
	}

	returncode = DEFAULT_RETURNCODE;
	return false;
}

EXPORT void UserCmd_Help(uint iClientID, const std::wstring& wscParam)
{
	PRINT_ERROR();
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	srand((uint)time(0));
	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (set_scCfgFile.length() > 0)
			LoadSettings();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
	}
	return true;
}

/// Hook will call this function after calling a plugin function to see if we the
/// processing to continue
EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions to hook
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Autobuy";
	p_PI->sShortName = "autobuy";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;

	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help, 0));
	return p_PI;
}