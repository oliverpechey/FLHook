// Solar Control FLHook plugin
// 25/04/2020
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Includes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Main.h"

#define RIGHT_CHECK(a) if(!(this->rights & a)) { Print(L"ERR No permission\n"); return; }

/// Hook will call this function after calling a plugin function to see if we the
/// processing to continue
EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//STRUCTURES AND DEFINITIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SOLAR_ARCHTYPE_STRUCT
{
	uint Shiparch;
	uint Loadout;
	uint IFF;
	uint Infocard;
	uint Base;
};

static std::map<std::wstring, SOLAR_ARCHTYPE_STRUCT> mapSolarArchtypes;

struct SOLAR
{
	std::wstring name;
	Vector pos;
	uint system;
	Matrix rot;
};

static std::map<int, SOLAR> startupSOLARs;
static std::map<uint, std::wstring> spawnedSOLARs;

float rand_FloatRange(float a, float b)
{
	return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Loading Settings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LoadSolarInfo()
{
	// The path to the configuration file.
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	std::string scPluginCfgFile = std::string(szCurDir) + "\\flhook_plugins\\solar.cfg";

	INI_Reader ini;
	if (ini.open(scPluginCfgFile.c_str(), false))
	{
		while (ini.read_header())
		{
			if (ini.is_header("solars"))
			{
				SOLAR_ARCHTYPE_STRUCT solarstruct;
				while (ini.read_value())
				{
					if (ini.is_value("solar"))
					{
						std::string setsolarname = ini.get_value_string(0);
						std::wstring solarname = stows(setsolarname);
						solarstruct.Shiparch = CreateID(ini.get_value_string(1));
						std::string loadoutstring = ini.get_value_string(2);
						solarstruct.Loadout = CreateID(loadoutstring.c_str());

						//IFF calc
						pub::Reputation::GetReputationGroup(solarstruct.IFF, ini.get_value_string(3));

						solarstruct.Infocard = ini.get_value_int(4);
						solarstruct.Base = CreateID(ini.get_value_string(5));

						mapSolarArchtypes[solarname] = solarstruct;
					}
				}
			}
			else if (ini.is_header("startupsolars")) {
				while (ini.read_value())
				{
					if (ini.is_value("startupsolar")) {
						SOLAR n;
						n.name = stows(ini.get_value_string(0));
						n.pos.x = ini.get_value_int(1);
						n.pos.y = ini.get_value_int(2);
						n.pos.z = ini.get_value_int(3);
						n.rot.data[0][0] = ini.get_value_float(4);
						n.rot.data[0][1] = ini.get_value_float(5);
						n.rot.data[0][2] = ini.get_value_float(6);
						n.rot.data[1][0] = ini.get_value_float(7);
						n.rot.data[1][1] = ini.get_value_float(8);
						n.rot.data[1][2] = ini.get_value_float(9);
						n.rot.data[2][0] = ini.get_value_float(10);
						n.rot.data[2][1] = ini.get_value_float(11);
						n.rot.data[2][2] = ini.get_value_float(12);
						n.system = CreateID(ini.get_value_string(13));
						startupSOLARs[startupSOLARs.size()] = n;
					}
				}
			}
		}
		ini.close();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

FILE* Logfile = fopen("./flhook_logs/solar_log.log", "at");

void Logging(const char* szString, ...)
{
	char szBufString[1024];
	va_list marker;
	va_start(marker, szString);
	_vsnprintf(szBufString, sizeof(szBufString) - 1, szString, marker);

	char szBuf[64];
	time_t tNow = time(0);
	struct tm* t = localtime(&tNow);
	strftime(szBuf, sizeof(szBuf), "%d/%m/%Y %H:%M:%S", t);
	fprintf(Logfile, "%s %s\n", szBuf, szBufString);
	fflush(Logfile);
	fclose(Logfile);
	Logfile = fopen("./flhook_logs/solar_log.log", "at");
}

void Log_CreateSolar(std::wstring name)
{
	//internal log
	std::wstring wscMsgLog = L"created <%name>";
	wscMsgLog = ReplaceStr(wscMsgLog, L"%name", name.c_str());
	std::string scText = wstos(wscMsgLog);
	Logging("%s", scText.c_str());
}

int __cdecl HkCreateSolar(uint& iSpaceID, pub::SpaceObj::SolarInfo& solarInfo)
{
	// hack server.dll so it does not call create solar packet send

	char* serverHackAddress = (char*)hModServer + 0x2A62A;
	char serverHack[] = { '\xEB' };
	WriteProcMem(serverHackAddress, &serverHack, 1);

	// create it
	int returnVal = pub::SpaceObj::CreateSolar(iSpaceID, solarInfo);

	uint dunno;
	IObjInspectImpl* inspect;
	if (GetShipInspect(iSpaceID, inspect, dunno))
	{
		CSolar* solar = (CSolar*)inspect->cobject();

		// for every player in the same system, send solar creation packet

		struct SOLAR_STRUCT
		{
			std::byte dunno[0x100];
		};

		SOLAR_STRUCT packetSolar;

		char* address1 = (char*)hModServer + 0x163F0;
		char* address2 = (char*)hModServer + 0x27950;

		// fill struct
		__asm
		{
			lea ecx, packetSolar
			mov eax, address1
			call eax
			push solar
			lea ecx, packetSolar
			push ecx
			mov eax, address2
			call eax
			add esp, 8
		}

		struct PlayerData* pPD = 0;
		while (pPD = Players.traverse_active(pPD))
		{
			if (pPD->iSystemID == solarInfo.iSystemID)
				GetClientInterface()->Send_FLPACKET_SERVER_CREATESOLAR(pPD->iOnlineID, (FLPACKET_CREATESOLAR&)packetSolar);
		}

	}

	// undo the server.dll hack
	char serverUnHack[] = { '\x74' };
	WriteProcMem(serverHackAddress, &serverUnHack, 1);

	return returnVal;
}

uint CreateSolar(std::wstring name, Vector pos, Matrix rot, uint iSystem, bool varyPos) {

	SOLAR_ARCHTYPE_STRUCT arch = mapSolarArchtypes[name];

	pub::SpaceObj::SolarInfo si{};
	memset(&si, 0, sizeof(si));
	si.iFlag = 4;

	// Prepare the settings for the space object
	si.iArchID = arch.Shiparch;
	si.iLoadoutID = arch.Loadout;
	si.iHitPointsLeft = 1000;
	si.iSystemID = iSystem;
	si.mOrientation = rot;
	si.Costume.head = CreateID("pi_pirate2_head");
	si.Costume.body = CreateID("pi_pirate8_body");
	si.Costume.lefthand = 0;
	si.Costume.righthand = 0;
	si.Costume.accessories = 0;
	si.iVoiceID = CreateID("atc_leg_m01");
	si.iRep = arch.IFF;
	std::string npcid = wstos(name).c_str() + std::to_string(spawnedSOLARs.size());
	strncpy_s(si.cNickName, sizeof(si.cNickName), npcid.c_str(), name.size() + spawnedSOLARs.size());

	// Do we need to vary the starting position slightly? Useful when spawning multiple objects
	si.vPos = pos;
	if (varyPos) {
		si.vPos.x = pos.x + rand_FloatRange(0, 1000);
		si.vPos.y = pos.y + rand_FloatRange(0, 1000);
		si.vPos.z = pos.z + rand_FloatRange(0, 2000);
	}
	else {
		si.vPos.x = pos.x;
		si.vPos.y = pos.y;
		si.vPos.z = pos.z;
	}

	// Which base this links to
	si.iUnk8 = arch.Base;

	// Define the string used for the scanner name. Because the
	// following entry is empty, the pilot_name is used. This
	// can be overriden to display the ship type instead.
	FmtStr scanner_name(0, 0);
	scanner_name.begin_mad_lib(0);
	scanner_name.end_mad_lib();

	// Define the string used for the pilot name.
	FmtStr pilot_name(0, 0);
	pilot_name.begin_mad_lib(16163); // ids of "%s0 %s1"
	pilot_name.append_string(arch.Infocard);
	pilot_name.end_mad_lib();

	// Set Reputation
	pub::Reputation::Alloc(si.iRep, scanner_name, pilot_name);
	pub::Reputation::SetAffiliation(si.iRep, arch.IFF);

	// Spawn the solar object
	uint iSpaceObj;
	HkCreateSolar(iSpaceObj, si);

	// Set the visible health for the Space Object
	pub::SpaceObj::SetRelativeHealth(iSpaceObj, 1);

	spawnedSOLARs[iSpaceObj] = name;

	return iSpaceObj;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Client command processing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AdminCmd_SolarMake(CCmds* cmds, int Amount, std::wstring SolarType)
{
	if (!(cmds->rights & RIGHT_SUPERADMIN))
	{
		cmds->Print(L"ERR No permission\n");
		return;
	}

	if (Amount == 0) { Amount = 1; }

	SOLAR_ARCHTYPE_STRUCT arch;

	std::map<std::wstring, SOLAR_ARCHTYPE_STRUCT>::iterator iter = mapSolarArchtypes.find(SolarType);
	if (iter != mapSolarArchtypes.end())
	{
		arch = iter->second;
	}
	else
	{
		cmds->Print(L"ERR Wrong Solar name\n");
		return;
	}

	uint iShip1;
	pub::Player::GetShip(HkGetClientIdFromCharname(cmds->GetAdminName()), iShip1);
	if (!iShip1)
		return;

	uint iSystem;
	pub::Player::GetSystem(HkGetClientIdFromCharname(cmds->GetAdminName()), iSystem);

	Vector pos;
	Matrix rot;
	pub::SpaceObj::GetLocation(iShip1, pos, rot);

	//Creation counter
	for (int i = 0; i < Amount; i++)
	{
		CreateSolar(SolarType, pos, rot, iSystem, true);
	}

	return;
}

void AdminCmd_SolarKill(CCmds* cmds)
{
	if (!(cmds->rights & RIGHT_SUPERADMIN))
	{
		cmds->Print(L"ERR No permission\n");
		return;
	}

	for (std::map<uint, std::wstring>::iterator i = spawnedSOLARs.begin();
		i != spawnedSOLARs.end(); ++i)
	{
		pub::SpaceObj::SetRelativeHealth(i->first, 0.0f);
	}
	spawnedSOLARs.clear();
	cmds->Print(L"OK\n");

	return;
}

#define IS_CMD(a) !wscCmd.compare(L##a)

bool ExecuteCommandString_Callback(CCmds* cmds, const std::wstring& wscCmd)
{
	returncode = DEFAULT_RETURNCODE;
	if (IS_CMD("solarcreate"))
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		AdminCmd_SolarMake(cmds, cmds->ArgInt(1), cmds->ArgStr(2));
		return true;
	}
	else if (IS_CMD("solardestroy"))
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		AdminCmd_SolarKill(cmds);
		return true;
	}
	return false;
}

void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	LoadSolarInfo();

	// hook solar creation to fix fl-bug in MP where loadout is not sent
	char* pAddressCreateSolar = ((char*)GetModuleHandle("content.dll") + 0x1134D4);
	FARPROC fpHkCreateSolar = (FARPROC)HkCreateSolar;
	WriteProcMem(pAddressCreateSolar, &fpHkCreateSolar, 4);

	for (std::map<int, SOLAR>::iterator i = startupSOLARs.begin();
		i != startupSOLARs.end(); ++i)
	{
		CreateSolar(i->second.name, i->second.pos, i->second.rot, i->second.system, false);
		Log_CreateSolar(i->second.name);
	}
}

void LoadSettings();

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions to hook
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Solar Control by Raikkonen (based from NPCs by Alley)";
	p_PI->sShortName = "solar";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	//p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	return p_PI;
}


