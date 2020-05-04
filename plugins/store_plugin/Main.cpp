// Store Plugin
// Originally by ||KOS||Acid
// https://sourceforge.net/projects/kosacid/files/

#include "Main.h"

#define ADDR_FLCONFIG 0x25410

std::vector<HINSTANCE> vDLLs;
std::string utos(uint i)
{
	char szBuf[16];
	sprintf_s(szBuf, "%u", i);
	return szBuf;
}

std::wstring ftows(float f)
{
	wchar_t wszBuf[16];
	swprintf_s(wszBuf, L"%g", f);
	return wszBuf;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	char* szFLConfig = (char*)((char*)GetModuleHandle(0) + ADDR_FLCONFIG);
	HkLoadDLLConf(szFLConfig);
}

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();
	return true;
}

EXPORT void UserCmd_Help(uint iClientID, const std::wstring& wscParam)
{
	PrintUserCmdText(iClientID, L"/sinfo <--- will show if you have enything stored in the current system");
	PrintUserCmdText(iClientID, L"/store <n> <ammount> <--- <n> = the number /enumcargo supplys");
	PrintUserCmdText(iClientID, L"/unstore <n> <ammount> <--- <n> = the number /sinfo supplys");
	PrintUserCmdText(iClientID, L"/enumcargo <--- shows slot number of cargo in your hold");
}

void UserCmd_Store(uint iClientID, const std::wstring& wscParam)
{
	CAccount* acc = Players.FindAccountFromClientID(iClientID);
	std::wstring wscDir;
	HkGetAccountDirName(acc, wscDir);
	scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
	uint iShip;
	pub::Player::GetShip(iClientID, iShip);
	uint iTarget;
	pub::SpaceObj::GetTarget(iShip, iTarget);
	uint iType;
	pub::SpaceObj::GetType(iTarget, iType);
	if (iType == 8192)
	{
		uint wscGoods = ToInt(GetParam(wscParam, ' ', 0));
		int wscCount = ToInt(GetParam(wscParam, ' ', 1));
		if (wscParam.find(L"all") != -1)
		{
			std::list<CARGO_INFO> lstCargo;
			int iRem;
			HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
			for (auto& cargo : lstCargo)
			{
				const GoodInfo* gi = GoodList::find_by_id(cargo.iArchID);
				if (!gi)
					continue;
				if (!cargo.bMounted && gi->iIDS)
				{
					int iGoods = 0;
					iGoods = IniGetI(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(cargo.iArchID), 0);
					IniWrite(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(cargo.iArchID), itos(iGoods + cargo.iCount));
					HkRemoveCargo(ARG_CLIENTID(iClientID), cargo.iID, cargo.iCount);
				}
			}
			PrintUserCmdText(iClientID, L"Ok");
			return;
		}
		if (wscCount <= 0)
		{
			PrintUserCmdText(iClientID, L"Error: You cannot transfer negative or zero amounts");
			return;
		}
		if (wscGoods == 0 || wscCount == 0)
		{
			PrintUserCmdText(iClientID, L"you must enter the right values try /enumcargo id amount");
		}
		else
		{
			std::list<CARGO_INFO> lstCargo;
			int iRem;
			HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
			for (auto& cargo : lstCargo)
			{
				if (cargo.iID == wscGoods)
				{
					if (cargo.iCount - wscCount < 0)
					{
						PrintUserCmdText(iClientID, L"You dont have enough");
					}
					else
					{
						int iGoods = 0;
						iGoods = IniGetI(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(cargo.iArchID), 0);
						IniWrite(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(cargo.iArchID), itos(iGoods + wscCount));
						HkRemoveCargo(ARG_CLIENTID(iClientID), cargo.iID, wscCount);
						PrintUserCmdText(iClientID, L"Ok");
					}
				}

			}
		}
	}
	else
	{
		PrintUserCmdText(iClientID, L"You must target a Storage Container");
	}
}

void UserCmd_Ustore(uint iClientID, const std::wstring& wscParam)
{
	CAccount* acc = Players.FindAccountFromClientID(iClientID);
	std::wstring wscDir;
	HkGetAccountDirName(acc, wscDir);
	scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
	std::list<INISECTIONVALUE> lstGoods;
	IniGetSection(scUserStore, wstos(HkGetPlayerSystem(iClientID)), lstGoods);
	int count = 0;
	uint wscGoods;
	for ( auto& it3 : lstGoods)
	{
		count = count + 1;
		if (ToInt(GetParam(wscParam, ' ', 0).c_str()) == count)
		{
			wscGoods = ToInt(stows(it3.scKey).c_str());
		}
	}
	int wscCount = ToInt(GetParam(wscParam, ' ', 1));
	if (wscCount <= 0)
	{
		PrintUserCmdText(iClientID, L"Error: You cannot transfer negative or zero amounts");
		return;
	}
	if (wscGoods == 0 || wscCount == 0)
	{
		PrintUserCmdText(iClientID, L"you must enter the right values try /ustore id amount");
	}
	else
	{
		uint iShip;
		pub::Player::GetShip(iClientID, iShip);
		uint iTarget;
		pub::SpaceObj::GetTarget(iShip, iTarget);
		uint iType;
		pub::SpaceObj::GetType(iTarget, iType);
		if (iType == 8192)
		{
			int iGoods = IniGetI(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(wscGoods), 0);
			if (iGoods == 0)
			{
				PrintUserCmdText(iClientID, L"Goods not found");
			}
			else
			{
				if (iGoods - wscCount < 0)
				{
					PrintUserCmdText(iClientID, L"You dont have enough");
				}
				else
				{
					Archetype::Equipment* eq = Archetype::GetEquipment(wscGoods);
					const GoodInfo* id = GoodList::find_by_archetype(wscGoods);
					float fRemainingHold;
					pub::Player::GetRemainingHoldSize(iClientID, fRemainingHold);
					if (id->iType == 0)
					{
						if (eq->fVolume * wscCount > fRemainingHold)
						{
							PrintUserCmdText(iClientID, L"You dont have enough cargo space");
							wscCount = ToInt(ftows(fRemainingHold));
						}
					}
					int nCount = 0;
					int sCount = 0;
					uint iNanobotsID = 2911012559;
					uint iShieldBatID = 2596081674;
					Archetype::Ship* ship = Archetype::GetShip(Players[iClientID].iShipArchetype);
					std::list<CARGO_INFO> lstCargo;
					int iRem;
					HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
					for ( auto& cargo : lstCargo )
					{
						if (cargo.iArchID == iNanobotsID) { nCount = cargo.iCount; }
						if (cargo.iArchID == iShieldBatID) { sCount = cargo.iCount; }
						if (wscGoods == iNanobotsID)
						{
							uint amount = nCount + wscCount;
							if (amount > ship->iMaxNanobots)
							{
								PrintUserCmdText(iClientID, L"Warning: the number of nanobots is greater than allowed");
								wscCount = ship->iMaxNanobots - nCount;
							}
						}
						if (wscGoods == iShieldBatID)
						{
							uint amount = sCount + wscCount;
							if (amount > ship->iMaxShieldBats)
							{
								PrintUserCmdText(iClientID, L"Warning: the number of shield batteries is greater than allowed");
								wscCount = ship->iMaxShieldBats - sCount;
							}
						}
					}
					if (id->iType == 1)
					{
						int uCount = 0;
						for ( auto& cargo : lstCargo )
						{
							if (cargo.iArchID == wscGoods) { uCount = cargo.iCount; }
							if (wscCount + uCount > MAX_PLAYER_AMMO)
							{
								PrintUserCmdText(iClientID, L"Warning: the number of goods is greater than allowed");
								wscCount = MAX_PLAYER_AMMO - uCount;
							}
							if (eq->fVolume * wscCount > fRemainingHold)
							{
								PrintUserCmdText(iClientID, L"You dont have enough cargo space");
								wscCount = 0;
							}
						}
					}
					Archetype::Gun* gun = (Archetype::Gun*)eq;
					if (gun->iArchID)//if not here items will stack and not show the amount
					{
						HkAddCargo(ARG_CLIENTID(iClientID), wscGoods, wscCount, false);
						IniWrite(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(wscGoods), itos(iGoods - wscCount));
						PrintUserCmdText(iClientID, L"You recived %s = %u from store", HkGetWStringFromIDS(gun->iIdsName).c_str(), wscCount);
					}
					else
					{
						HkAddCargo(ARG_CLIENTID(iClientID), wscGoods, wscCount, false);
						IniWrite(scUserStore, wstos(HkGetPlayerSystem(iClientID)), utos(wscGoods), itos(iGoods - wscCount));
						const GoodInfo* gi = GoodList::find_by_id(wscGoods);
						PrintUserCmdText(iClientID, L"You recived %s = %u from store", HkGetWStringFromIDS(gi->iIDSName).c_str(), wscCount);
					}
				}
			}
		}
		else
		{
			PrintUserCmdText(iClientID, L"You must target a Storage Container");
		}
	}
}

void UserCmd_Istore(uint iClientID, const std::wstring& wscParam)
{
	CAccount* acc = Players.FindAccountFromClientID(iClientID);
	std::wstring wscDir;
	HkGetAccountDirName(acc, wscDir);
	scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
	std::list<INISECTIONVALUE> lstGoods;
	IniGetSection(scUserStore, wstos(HkGetPlayerSystem(iClientID)), lstGoods);
	IniDelSection(scUserStore, wstos(HkGetPlayerSystem(iClientID)));
	int count = 0;
	for ( auto& it3 : lstGoods )
	{

		int iGoods = ToInt(stows(it3.scValue).c_str());
		if (iGoods > 0)
		{
			IniWrite(scUserStore, wstos(HkGetPlayerSystem(iClientID)), it3.scKey, it3.scValue);
			count = count + 1;
			Archetype::Equipment* eq = Archetype::GetEquipment(ToInt(stows(it3.scKey).c_str()));
			const GoodInfo* gi = GoodList::find_by_id(ToInt(stows(it3.scKey).c_str()));
			if (!gi)
				continue;
			Archetype::Gun* gun = (Archetype::Gun*)eq;
			if (gun->iArchID)
			{
				PrintUserCmdText(iClientID, L"%u=%s=%s", count, HkGetWStringFromIDS(gun->iIdsName).c_str(), stows(it3.scValue).c_str());
			}
			else
			{
				PrintUserCmdText(iClientID, L"%u=%s=%s", count, HkGetWStringFromIDS(gi->iIDSName).c_str(), stows(it3.scValue).c_str());
			}
		}
	}

}

void UserCmd_EnumCargo(uint iClientID, const std::wstring& wscParam)
{
	std::list<CARGO_INFO> lstCargo;
	int iRem;
	HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
	uint iNum = 0;
	for ( auto& it : lstCargo )
	{
		Archetype::Equipment* eq = Archetype::GetEquipment(it.iArchID);
		const GoodInfo* gi = GoodList::find_by_id(it.iArchID);
		if (!gi)
			continue;
		Archetype::Gun* gun = (Archetype::Gun*)eq;
		if (!it.bMounted)
		{
			if (gun->iArchID)
			{
				PrintUserCmdText(iClientID, L"%u: %s=%u", it.iID, HkGetWStringFromIDS(gun->iIdsName).c_str(), it.iCount);
			}
			else
			{
				PrintUserCmdText(iClientID, L"%u: %s=%u", it.iID, HkGetWStringFromIDS(gi->iIDSName).c_str(), it.iCount);
			}
			iNum++;
		}
	}
	if (!iNum)
		PrintUserCmdText(iClientID, L"Error: you have no unmounted equipment or goods");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

USERCMD UserCmds[] =
{
	{ L"/store",				UserCmd_Store},
	{ L"/unstore",			    UserCmd_Ustore},
	{ L"/sinfo",			    UserCmd_Istore},
	{ L"/enumcargo",            UserCmd_EnumCargo},
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

			returncode = SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command, return immediatly
			return true;
		}
	}

	returncode = DEFAULT_RETURNCODE; // we did not handle the command, so let other plugins or FLHook kick in
	return false;
}

std::wstring HkGetWStringFromIDS(uint iIDS) //Only works for names
{
	if (!iIDS)
		return L"";

	uint iDLL = iIDS / 0x10000;
	iIDS -= iDLL * 0x10000;

	wchar_t wszBuf[512];
	if (LoadStringW(vDLLs[iDLL], iIDS, wszBuf, 512))
		return wszBuf;
	return L"";
}

void HkLoadDLLConf(const char* szFLConfigFile)
{
	for (uint i = 0; i < vDLLs.size(); i++)
	{
		FreeLibrary(vDLLs[i]);
	}
	vDLLs.clear();
	HINSTANCE hDLL = LoadLibraryEx((char*)((char*)GetModuleHandle(0) + 0x256C4), NULL, LOAD_LIBRARY_AS_DATAFILE); //typically resources.dll
	if (hDLL)
		vDLLs.push_back(hDLL);
	INI_Reader ini;
	if (ini.open(szFLConfigFile, false))
	{
		while (ini.read_header())
		{
			if (ini.is_header("Resources"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("DLL"))
					{
						hDLL = LoadLibraryEx(ini.get_value_string(0), NULL, LOAD_LIBRARY_AS_DATAFILE);
						if (hDLL)
							vDLLs.push_back(hDLL);
					}
				}
			}
		}
		ini.close();
	}
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Store";
	p_PI->sShortName = "store";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	return p_PI;
}