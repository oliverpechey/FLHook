// Dynamic Missions by w0dk4

// Includes 
#include "FLHook.h"
#include "plugin.h"
#include "Main.h"
#include <math.h>

// Structs and global variables

struct TRIGGER_ARG
{
	uint iTriggerHash;
	uint iTriggerHash2;
	uint iClientID;
};

struct ACTION_DEBUGMSG_DATA
{
	void* freeFunc;
	void* mission_struct;
	uint iTriggerHash;
	uint iDunno;
	char szMessage[255];
};

uint iControllerID = 0;

char* pAddressTriggerAct;

// Functions

void __stdcall HkActTrigger(TRIGGER_ARG* trigger)
{
	std::wstring msg = L"Mission trigger activated with hash id: " + stows(itos(trigger->iTriggerHash)) + L"\n";
	ConPrint(msg);
	HkMsgU(msg);
}

__declspec(naked) void _HkActTrigger()
{
	__asm
	{
		push ecx
		push [esp+8]
		call HkActTrigger
		pop ecx
		// original func instructions
		push ebx
		push esi
		mov esi, [esp + 8 + 4]
		push edi
		mov eax, [pAddressTriggerAct]
		add eax, 7
		jmp eax
	}
}

int __stdcall HkActDebugMsg(ACTION_DEBUGMSG_DATA* action_dbgMsg)
{
	std::wstring msg = L"Mission trigger (" + stows(itos(action_dbgMsg->iTriggerHash)) + L") sent debug msg: " + stows(std::string(action_dbgMsg->szMessage)) + L"\n";
	ConPrint(msg);
	HkMsgU(msg);

	return 1;
}

__declspec(naked) void _HkActDebugMsg()
{
	__asm
	{
		push ecx
		call HkActDebugMsg
		mov eax, 1
		ret
	}
}

int __cdecl HkCreateSolar(uint &iSpaceID, pub::SpaceObj::SolarInfo &solarInfo)
{
	// hack server.dll so it does not call create solar packet send

    char* serverHackAddress = (char*)hModServer + 0x2A62A;
    char serverHack[] = {'\xEB'};
    WriteProcMem(serverHackAddress, &serverHack, 1);

	// create it
	int returnVal = pub::SpaceObj::CreateSolar(iSpaceID, solarInfo);

	uint dunno;
	IObjInspectImpl* inspect;
	if(GetShipInspect(iSpaceID, inspect, dunno))
	{
		CSolar* solar = (CSolar*)inspect->cobject();

		// for every player in the same system, send solar creation packet

		struct SOLAR_STRUCT
		{
			byte dunno[0x100];
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

		struct PlayerData *pPD = 0;
		while(pPD = Players.traverse_active(pPD))
		{
			if(pPD->iSystemID == solarInfo.iSystemID)
				GetClientInterface()->Send_FLPACKET_SERVER_CREATESOLAR(pPD->iOnlineID, (FLPACKET_CREATESOLAR&)packetSolar);
		}

	}

	 // undo the server.dll hack
    char serverUnHack[] = {'\x74'};	
    WriteProcMem(serverHackAddress, &serverUnHack, 1);

	return returnVal;
}

HK_ERROR HkMissionStart()
{
	
	if(iControllerID)
		return HKE_UNKNOWN_ERROR;

	char* pAddress = (char*)hModContent + 0x114388;
	pub::Controller::CreateParms params = {pAddress, 1};
	iControllerID = pub::Controller::Create("Content.dll", "Mission_01a", &params, (pub::Controller::PRIORITY)2);
	pub::Controller::_SendMessage(iControllerID, 0x1000, 0);
	
	return HKE_OK;
}

HK_ERROR HkMissionEnd()
{
	if(!iControllerID)
		return HKE_UNKNOWN_ERROR;

	pub::Controller::Destroy(iControllerID);
	iControllerID = 0;

	return HKE_OK;
}

// Load Settings - Installs some hooks and loads in config file (and possibly starts mission)

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	// patch singleplayer check in Player pos calculation
	char* pAddress = ((char*)GetModuleHandle("content.dll") + 0xD3B0D);
	char szNOP[] = { '\x90', '\x90','\x90','\x90','\x90','\x90' };
	WriteProcMem(pAddress, szNOP, 6);

	// install hook for trigger events

	pAddressTriggerAct = ((char*)GetModuleHandle("content.dll") + 0x182C0);
	FARPROC fpTF = (FARPROC)_HkActTrigger;
	char szMovEDX[] = { '\xBA' };
	char szJmpEDX[] = { '\xFF', '\xE2' };

	WriteProcMem(pAddressTriggerAct, szMovEDX, 1);
	WriteProcMem(pAddressTriggerAct + 1, &fpTF, 4);
	WriteProcMem(pAddressTriggerAct + 5, szJmpEDX, 2);

	// hook debug msg
	char* pAddressActDebugMsg = ((char*)GetModuleHandle("content.dll") + 0x115BC4);
	FARPROC fpADbg = (FARPROC)_HkActDebugMsg;
	WriteProcMem(pAddressActDebugMsg, &fpADbg, 4);

	// hook solar creation to fix fl-bug in MP where loadout is not sent
	char* pAddressCreateSolar = ((char*)GetModuleHandle("content.dll") + 0x1134D4);
	FARPROC fpHkCreateSolar = (FARPROC)HkCreateSolar;
	WriteProcMem(pAddressCreateSolar, &fpHkCreateSolar, 4);

	// The path to the configuration file.
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	std::string configFile = std::string(szCurDir) + "\\flhook_plugins\\missions.cfg";

	// Should we start the mission automatically?
	if (IniGetB(configFile, "General", "Auto-Start", false))
	{
		try
		{
			HkMissionStart();
		}
		catch (...)
		{
			LOG_EXCEPTION
		}
	}
}

// Admin Commands

void StartMission(CCmds* cmds)
{
	if (!(cmds->rights & RIGHT_SUPERADMIN)) { cmds->Print(L"ERR No permission\n"); return; }

	if (((cmds->hkLastErr = HkMissionStart()) == HKE_OK))
		cmds->Print(L"OK\n");
	else
		cmds->PrintError();
}

void EndMission(CCmds* cmds)
{
	if (!(cmds->rights & RIGHT_SUPERADMIN)) { cmds->Print(L"ERR No permission\n"); return; }

	if (((cmds->hkLastErr = HkMissionEnd()) == HKE_OK)) 
		cmds->Print(L"OK\n");
	else
		cmds->PrintError();
}

EXPORT bool ExecuteCommandString_Callback(CCmds* cmds, const std::wstring& wscCmd)
{
	returncode = NOFUNCTIONCALL;  // flhook needs to care about our return code

	if (IS_CMD("startmission")) {

		returncode = SKIPPLUGINS_NOFUNCTIONCALL; // do not let other plugins kick in since we now handle the command

		StartMission(cmds);

		return true;
	}

	if (IS_CMD("endmission")) {

		returncode = SKIPPLUGINS_NOFUNCTIONCALL; // do not let other plugins kick in since we now handle the command

		EndMission(cmds);

		return true;
	}

	return false;
}

EXPORT void CmdHelp_Callback(CCmds* cmds)
{
	returncode = DEFAULT_RETURNCODE;

	cmds->Print(L"startmission\n");
	cmds->Print(L"endmission\n");
}

// FLHook functions

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return true;
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Dynamic Missions";
	p_PI->sShortName = "missions";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CmdHelp_Callback, PLUGIN_CmdHelp_Callback, 0));
	return p_PI;
}
