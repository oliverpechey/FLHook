// This is a template with the bare minimum to have a functional plugin.
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.


#include <FLHook.h>
#include <plugin.h>

ReturnCode returncode;

bool bSelfDestructEnabled = false;
bool bLightToggleEnabled = false;
bool bShieldToggleEnabled = false;
bool bPosEnabled = false;
bool bStuckEnabled = false;
bool bDiceEnabled = false;
bool bCoinEnabled = false;
bool bSmiteAllEnabled = false;

struct INFO {
    bool bLightsOn = false;
    bool bShieldsDown = false;
    bool bSelfDestruct = false;
};

std::map<uint, INFO> mapInfo;

// Load configuration file
void LoadSettings()
{
	// The path to the configuration file.
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	std::string dir = std::string(szCurDir) + "\\flhook_plugins\\Misc_Commands_Plugin.ini";

	bSelfDestructEnabled = IniGetB(dir, "Config", "bSelfDestructEnabled", false);
    bLightToggleEnabled = IniGetB(dir, "Config", "bLightToggleEnabled", false);
    bShieldToggleEnabled = IniGetB(dir, "Config", "bShieldToggleEnabled", false);
    bPosEnabled = IniGetB(dir, "Config", "bPosEnabled", false);
    bStuckEnabled = IniGetB(dir, "Config", "bStuckEnabled", false);
    bDiceEnabled = IniGetB(dir, "Config", "bDiceEnabled", false);
    bCoinEnabled = IniGetB(dir, "Config", "bCoinEnabled", false);
    bSmiteAllEnabled = IniGetB(dir, "Config", "bSmiteAllEnabled", false);
}

// Do something every 100 seconds
void HkTimer()
{
	if ((time(0) % 100) == 0)
	{
		// Do something here
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER COMMANDS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Demo command
void UserCmd_Template(uint iClientID, const std::wstring& wscParam)
{
	PrintUserCmdText(iClientID, L"OK");
}

// Additional information related to the plugin when the /help command is used
void UserCmd_Help(uint iClientID, const std::wstring& wscParam)
{
	PrintUserCmdText(iClientID, L"/template");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER COMMAND PROCESSING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Define usable chat commands here
USERCMD UserCmds[] =
{
	{ L"/template", UserCmd_Template },
};

// Process user input
bool UserCmd_Process(uint iClientID, const std::wstring& wscCmd)
{
    DefaultUserCommandHandling(iClientID, wscCmd, UserCmds, returncode);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADMIN COMMANDS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Demo admin command
void AdminCmd_Template(CCmds* cmds, float number)
{
	if (cmds->ArgStrToEnd(1).length() == 0)
	{
		cmds->Print(L"ERR Usage: template <number>");
		return;
	}

	if (!(cmds->rights & RIGHT_SUPERADMIN))
	{
		cmds->Print(L"ERR No permission");
		return;
	}

	cmds->Print(L"Template is %0.0f", number);
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADMIN COMMAND PROCESSING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Define usable admin commands here
void CmdHelp_Callback(CCmds* classptr)
{
	classptr->Print(L"template <number>");
}

// Admin command callback. Compare the chat entry to see if it match a command
bool ExecuteCommandString_Callback(CCmds* cmds, const std::wstring& wscCmd)
{
	if (IS_CMD("template"))
	{
		returncode = ReturnCode::SkipFunctionCall;
		AdminCmd_Template(cmds, cmds->ArgFloat(1));
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Do things when the dll is loaded
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	srand(uint(time(nullptr)));

	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if (fdwReason == DLL_PROCESS_ATTACH && set_scCfgFile.length() > 0)
		LoadSettings();

	return true;
}

// Functions to hook
extern "C" EXPORT void ExportPluginInfo(PluginInfo *pi) {
    pi->name("Misc Commands");
    pi->shortName("MiscCommands");
    pi->mayPause(true);
    pi->mayUnload(true);
    pi->returnCode(&returncode);
	pi->versionMajor(PluginMajorVersion::VERSION_04);
	pi->versionMinor(PluginMinorVersion::VERSION_00);
    pi->emplaceHook(HookedCall::FLHook__LoadSettings, &LoadSettings);
}