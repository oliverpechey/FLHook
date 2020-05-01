// Area Triggers v0.1 by [ZSS]~Lemming and [ZSS]~Raikkonen of the Zoner Shadow Syndicate.
// A plugin to detect when players are in certain locations and perform preset actions
// upon them defined in the plugin config files.
//
// Made for Zoner Universe - www.zoneruniverse.com. No rights reserved.

#include "Main.h"

// includes for vector arrays:
#include<iostream>
#include <vector>

// globals:
struct triggerAreasMapElement {
	vector pos;
	int radius;				// The radius around the trigger which causes activation.
	std::string system;		// The system our trigger resides in.
	int triggerAction;		// Action to take on trigger activation as int:
							// 1 warp (in-system), 
							// 2 beam (to base), 
							// 3 heal,
							// 4 kill,
							// 5 
							// 6 
							// 7 spawn npc group,
};

std::vector< triggerAreasMapElement > triggerPoints; // map of trigger zone positions

int tickClock = 0;		// Increments every server tick. 
int scanInterval = 60;	// How often to scan a player location (changes based on player count).
int clientsActiveNow;
uint iClientID = 1;
int thisClientIsOnline = 0;

// We only want to check the position of one player at once to avoid causing instability due to a spike of cpu use.
// To do this we'll use an incremental int to track which player we are currently checking:
int currentPlayerToCheck = 1;
// (we start at 1 because I assume the ids system starts there rather than 0)


///////////////////////////////////////////////////////////////////////////////////////

// Load configuration file
void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	// The path to the configuration file.
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	std::string configFile = std::string(szCurDir) + "\\flhook_plugins\\area_triggers.cfg";

	INI_Reader ini;
	if (ini.open(configFile.c_str(), false))
	{
		while (ini.read_header())
		{	
			if (ini.is_header("Zones"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("zone"))
					{ 
						triggerAreasMapElement dataItem;
						dataItem.pos.x = ini.get_value_int(0);
						dataItem.pos.y = ini.get_value_int(1);
						dataItem.pos.z = ini.get_value_int(2);
						dataItem.radius = ini.get_value_int(3);
						dataItem.system = ini.get_value_string(4);
						triggerPoints.push_back(dataItem);
					}					
				}
			}
		}
		ini.close();
	}
}

void scanTriggerZones()
{
	// update our scanInterval based on how many players are online:
	clientsActiveNow = GetNumClients();
	if (clientsActiveNow)
	{
		if (clientsActiveNow < 30) {
			scanInterval = 60 / clientsActiveNow;
		}
		else {
			scanInterval = 1;
		}

		if (iClientID > clientsActiveNow)
			iClientID = 0;

	} else {
		iClientID = 0;
		scanInterval = 100;
		return;
	}

	uint iShip;
	pub::Player::GetShip(iClientID, iShip);

	if (iShip != 0)
	{
		// scan the player against our defined zones
		Vector pos;
		Matrix rot;
		pub::SpaceObj::GetLocation(iShip, pos, rot); // get position of player's ship as 'pos'

		uint iSystem;
		pub::SpaceObj::GetSystem(iShip, iSystem);

		// loop through the systems in triggerPoints and see if the player is in one
		for(auto& s : triggerPoints)
		{
			if (iSystem == CreateID(s.system.c_str()))
			{ // they are in a warp system so now check their xyz position against the relevant trigger position data:
				if (pos.x < s.pos.x + s.radius && pos.x > s.pos.x - s.radius && pos.y < s.pos.y + s.radius && pos.y > s.pos.y - s.radius && pos.z < s.pos.z + s.radius && pos.z > s.pos.z - s.radius)
				{
					// beam player
					HkRelocateClient(iClientID, s.pos, rot);
				}
			}
		}
	}
	//HkMsgU(L"Scan performed");
	iClientID++;
}

// Do every tick
void HkTick()
{
	// check to see if it's time to check a player's position against trigger zones
	if (tickClock > scanInterval)
	{
		scanTriggerZones();
		tickClock = 0;
	}
	else {
		tickClock++;
	}
}

void OnConnect() {
	// Do something
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER COMMANDS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Demo command
bool UserCmd_Template(uint iClientID, const std::wstring& wscCmd, const std::wstring& wscParam, const wchar_t* usage)
{
	PrintUserCmdText(iClientID, L"OK");
	return true;
}

// Additional information related to the plugin when the /help command is used
void UserCmd_Help(uint iClientID, const std::wstring& wscParam)
{
	returncode = DEFAULT_RETURNCODE;
	PrintUserCmdText(iClientID, L"/template");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER COMMAND PROCESSING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Define usable chat commands here
USERCMD UserCmds[] =
{
	{ L"/template", UserCmd_Template, L"Usage: /template" }
};

// Process user input
bool UserCmd_Process(uint iClientID, const std::wstring& wscCmd)
{
	returncode = DEFAULT_RETURNCODE;

	try
	{
		std::wstring wscCmdLineLower = ToLower(wscCmd);

		// If the chat std::string does not match the USER_CMD then we do not handle the
		// command, so let other plugins or FLHook kick in. We require an exact match
		for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++)
		{
			if (wscCmdLineLower.find(UserCmds[i].wszCmd) == 0)
			{
				// Extract the parameters std::string from the chat std::string. It should
				// be immediately after the command and a space.
				std::wstring wscParam = L"";
				if (wscCmd.length() > wcslen(UserCmds[i].wszCmd))
				{
					if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
						continue;
					wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
				}

				// Dispatch the command to the appropriate processing function.
				if (UserCmds[i].proc(iClientID, wscCmd, wscParam, UserCmds[i].usage))
				{
					// We handled the command tell FL hook to stop processing this
					// chat std::string.
					returncode = SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command, return immediatly
					return true;
				}
			}
		}
	}
	catch (...)
	{
		AddLog("ERROR: Exception in UserCmd_Process(iClientID=%u, wscCmd=%s)", iClientID, wstos(wscCmd).c_str());
		LOG_EXCEPTION;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADMIN COMMANDS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Demo admin command
void AdminCmd_Template(CCmds* cmds, float number)
{
	if (cmds->ArgStrToEnd(1).length() == 0)
	{
		cmds->Print(L"ERR Usage: template <number>\n");
		return;
	}

	if (!(cmds->rights & RIGHT_SUPERADMIN))
	{
		cmds->Print(L"ERR No permission\n");
		return;
	}

	cmds->Print(L"Template is %0.0f\n", number);
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADMIN COMMAND PROCESSING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Define usable admin commands here
void CmdHelp_Callback(CCmds* classptr)
{
	returncode = DEFAULT_RETURNCODE;
	classptr->Print(L"template <number>\n");
}

// Admin command callback. Compare the chat entry to see if it match a command
bool ExecuteCommandString_Callback(CCmds* cmds, const std::wstring& wscCmd)
{
	returncode = DEFAULT_RETURNCODE;

	if (IS_CMD("template"))
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
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
	srand((uint)time(0));

	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if (fdwReason == DLL_PROCESS_ATTACH && set_scCfgFile.length() > 0)
		LoadSettings();

	return true;
}

// Functions to hook
EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "area_triggers_plugin";
	p_PI->sShortName = "area_triggers_plugin";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTick, PLUGIN_HkIServerImpl_Update, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CmdHelp_Callback, PLUGIN_CmdHelp_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&OnConnect, PLUGIN_HkIServerImpl_OnConnect_AFTER, 0));
	
	return p_PI;
}
