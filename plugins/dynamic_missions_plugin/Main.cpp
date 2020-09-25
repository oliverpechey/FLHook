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

bool bAutoStart = false;
uint iControllerID = 0;
char* pAddressTriggerAct;

// Functions

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

void __stdcall Startup_AFTER(unsigned int iShip, unsigned int iClientID)
{
	returncode = DEFAULT_RETURNCODE;

	// Should we start the mission automatically?
	if (bAutoStart)
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

// Load Settings - Installs some hooks and loads in config file (and possibly starts mission)
EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	// patch singleplayer check in Player pos calculation
	char* pAddress = ((char*)GetModuleHandle("content.dll") + 0xD3B0D);
	char szNOP[] = { '\x90', '\x90','\x90','\x90','\x90','\x90' };
	WriteProcMem(pAddress, szNOP, 6);

	// The path to the configuration file.
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	std::string configFile = std::string(szCurDir) + "\\flhook_plugins\\missions.cfg";
	bAutoStart = IniGetB(configFile, "General", "Auto-Start", false);
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
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&Startup_AFTER, PLUGIN_HkIServerImpl_Startup_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CmdHelp_Callback, PLUGIN_CmdHelp_Callback, 0));
	return p_PI;
}
