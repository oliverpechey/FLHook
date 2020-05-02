// Area Triggers v0.1 by [ZSS]~Lemming and [ZSS]~Raikkonen of the Zoner Shadow Syndicate.
// A plugin to detect when players are in certain locations and perform preset actions
// upon them defined in the plugin config files.
//
// Made for Zoner Universe - www.zoneruniverse.com. No rights reserved.

#include "Main.h"

// globals:
struct Action 
{
	std::string type;
	Vector pos;
	uint base;
	int health;
};

struct TriggerItem 
{
	std::string name;
	Vector pos;
	int radius;				// The radius around the trigger which causes activation.
	std::string system;		// The system our trigger resides in.
	Action action;		// Action to take on trigger activation
};

std::vector< TriggerItem > triggers; // map of trigger zone positions

int tickClock = 0;		// Increments every server tick. 
int scanInterval = 60;	// How often to scan a player location (changes based on player count).

// We only want to check the position of one player at once to avoid causing instability due to a spike of cpu use.
// To do this we'll use an incremental int to track which player we are currently checking:
uint iClientID = 1;


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
						TriggerItem dataItem;
						dataItem.name = ini.get_value_string(0);
						dataItem.pos.x = ini.get_value_int(1);
						dataItem.pos.y = ini.get_value_int(2);
						dataItem.pos.z = ini.get_value_int(3);
						dataItem.radius = ini.get_value_int(4);
						dataItem.system = ini.get_value_string(5);
						triggers.push_back(dataItem);
					}					
				}
			}
			if (ini.is_header("Actions"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("warp"))
					{
						for (auto& ti : triggers)
						{
							if (ti.name == ini.get_value_string(0)) 
							{
								ti.action.type = "warp";
								ti.action.pos.x = ini.get_value_int(1);
								ti.action.pos.y = ini.get_value_int(2);
								ti.action.pos.z = ini.get_value_int(3);
							}
							if (ti.name == ini.get_value_string(0))
							{
								ti.action.type = "beam";
								ti.action.base = CreateID(ini.get_value_string(1));
							}
							if (ti.name == ini.get_value_string(0))
							{
								ti.action.type = "heal";
								ti.action.health = ini.get_value_int(1);
							}
							if (ti.name == ini.get_value_string(0))
							{
								ti.action.type = "kill";
							}
						}
					}
				}
			}
		}
		ini.close();
	}
}

void scanTriggerZones(uint iClientID)
{
	uint iShip;
	pub::Player::GetShip(iClientID, iShip);

	// is player in space?
	if (iShip != 0)
	{
		// loop through the systems in triggerPoints and see if the player is in one
		for(auto& ti : triggers)
		{
			// scan the player against our defined zones
			Vector pos;
			Matrix rot;
			pub::SpaceObj::GetLocation(iShip, pos, rot); // get position of player's ship as 'pos'

			uint iSystem;
			pub::SpaceObj::GetSystem(iShip, iSystem);

			if (iSystem == CreateID(ti.system.c_str()))
			{ // they are in trigger system so now check their xyz position against the relevant trigger position data:
				if (pos.x < ti.pos.x + ti.radius && pos.x > ti.pos.x - ti.radius && pos.y < ti.pos.y + ti.radius && pos.y > ti.pos.y - ti.radius && pos.z < ti.pos.z + ti.radius && pos.z > ti.pos.z - ti.radius)
				{
					if (ti.action.type == "warp")
						HkRelocateClient(iClientID, ti.action.pos, rot);

					if (ti.action.type == "beam")
					{
						Universe::IBase* base = Universe::get_base(ti.action.base);

						pub::Player::ForceLand(iClientID, ti.action.base);

						// if not in the same system, emulate F1 charload
						if (base->iSystemID != iSystem)
						{
							Server.BaseEnter(ti.action.base, iClientID);
							Server.BaseExit(ti.action.base, iClientID);
							std::wstring wscCharFileName;
							HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);
							wscCharFileName += L".fl";
							CHARACTER_ID cID;
							strcpy(cID.szCharFilename, wstos(wscCharFileName.substr(0, 14)).c_str());
							Server.CharacterSelect(cID, iClientID);
						}

					}

					if (ti.action.type == "heal")
						pub::SpaceObj::SetRelativeHealth(iShip, ti.action.health);

					if (ti.action.type == "kill")
						pub::SpaceObj::Destroy(iShip, DestroyType::FUSE);
				}
			}
		}
	}
}

void updateInterval()
{
	// update our scanInterval based on how many players are online:
	int clientsActiveNow = GetNumClients();
	if (clientsActiveNow)
	{
		if (clientsActiveNow < 30)
			scanInterval = 60 / clientsActiveNow;
		else
			scanInterval = 1;

		if (iClientID > clientsActiveNow)
			iClientID = 1;
		scanTriggerZones(iClientID);
		iClientID++;
	}
	else {
		iClientID = 1;
		scanInterval = 100;
		return;
	}
}

// Do every tick
void HkTick()
{
	// check to see if it's time to check a player's position against trigger zones
	if (tickClock > scanInterval)
	{
		updateInterval();
		tickClock = 0;
	}
	else
		tickClock++;
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
	return p_PI;
}
