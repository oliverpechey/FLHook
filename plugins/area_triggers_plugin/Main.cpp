// Area Triggers v0.1 by [ZSS]~Lemming and [ZSS]~Raikkonen of the Zoner Shadow Syndicate.
// A plugin to detect when players are in certain locations and perform preset actions
//  upon them defined in the plugin config files.
//
// Made for Zoner Universe - the only vanilla server built entirely in Freeport 9.
// www.zoneruniverse.com. No rights reserved*
// * Except if you use any of it your hamster belongs to Lemming. Be sure to cut air holes in the crate. We don't want another incident like last time.

#include "Main.h"
#include <chrono> // used for timing cooldowns

/* List of actions for reference:
Warp	- Move inside a system
Beam	- To a place in another system
Heal	- Heal the ship. For example 
Kill	- An area that kills or damages ships. Probably want gradual damage applied each tick?..
Chat	- Display a chat message to the player entering the zone. Eg: "This area is marked as restricted to Liberty Security Force members"
Sound	- Play a sound to local players? Or just the one player?..
----
Ideas for v2:
Spawn	- Spawn an NPC or group of NPCs)
SpawnAndFollow - Spawn an NPC or group of NPCs that follow the player (probably want a rep check condition associated with it).
- These would be useful for say, spawning Nomads to hunt a player if they enter their space, 
	or police or BHs being sent if you have a pirate rep and enter a zone.


Notes:
If we add data from the planet locations it could be used to automate docking: Eg. fly into a planet's atmosphere to auto-dock with it.
In which case we'd want a 'dock' action. I suppose we'd want to scan the player position rapidly if we did this.


*/


struct Action				// Container for action data
{
	std::string type;		// The type of action to perform
	Vector pos;				// Warp: 3D Position of the action
	uint base;				// Beam: Base ID
	int health;				// Heal: Percentage of health to heal
	std::wstring text;		// Chat: Text to display
	uint sound;				// Sound: ID of sound to play on activation
};

struct Zone			// Container for the zone data
{
	std::string name;		// Unique ID of the zone
	Vector pos;				// 3D position of zone
	int radius;				// The radius around the zone which causes activation.
	std::string system;		// The system our zone resides in.
	Action action;			// Action to take on zone activation
};

std::vector< Zone > zones; // List of trigger zone positions

std::vector< std::chrono::steady_clock::time_point > cooldownTimers(999); // vector of time points type

// auto start = std::chrono::steady_clock::now();
// auto end = std::chrono::steady_clock::now();
// auto cooldownDuration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count(); // seconds elapsed since




int tickClock = 0;		// Increments every server tick (up to scan interval). 
int scanInterval = 60;	// How often to scan a player location (changes based on player count).
int clientsActiveNow;	// Tracks number of players in-game (they may be docked, but not on login screen).

int iClientIndex = 0; // Tracks which player ID we are currently using

std::vector<uint> iClientIDs; // Initialise variable to contain IDs of clients
void updateClientIDs()
{
	iClientIDs.clear(); // Clear the list of IDs so we don't build on top of it
	struct PlayerData* pPD = 0; // Struct to contain player data
	while (pPD = Players.traverse_active(pPD)) // Fetches the next player, returns false if theres no more to break the loop
	{
		if (!HkIsInCharSelectMenu(HkGetClientIdFromPD(pPD))) // Ignore players on login menu
			iClientIDs.push_back(HkGetClientIdFromPD(pPD)); 
			// 1st HkGetClientIdFromPD converts the pPD variable into a client ID, 
			// 2nd push the clientID onto the list.
	}
}

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
						Zone dataItem;
						dataItem.name = ini.get_value_string(0);
						dataItem.pos.x = ini.get_value_int(1);
						dataItem.pos.y = ini.get_value_int(2);
						dataItem.pos.z = ini.get_value_int(3);
						dataItem.radius = ini.get_value_int(4);
						dataItem.system = ini.get_value_string(5);
						zones.push_back(dataItem);
					}					
				}
			}
			if (ini.is_header("Actions"))
			{
				while (ini.read_value())
				{
					for (auto& ti : zones)
					{
						if (ti.name == ini.get_value_string(0))
						{
							if (ini.is_value("warp"))
							{
								ti.action.type = "warp";
								ti.action.pos.x = ini.get_value_int(1);
								ti.action.pos.y = ini.get_value_int(2);
								ti.action.pos.z = ini.get_value_int(3);
							}
							if (ini.is_value("beam"))
							{
								ti.action.type = "beam";
								ti.action.base = CreateID(ini.get_value_string(1));
							}
							if (ini.is_value("heal"))
							{
								ti.action.type = "heal";
								ti.action.health = ini.get_value_int(1);
							}
							if (ini.is_value("kill"))
							{
								ti.action.type = "kill";
							}
							if (ini.is_value("chat"))
							{
								ti.action.type = "chat";
								ti.action.text = stows(ini.get_value_string());
							}
							if (ini.is_value("sound"))
							{
								ti.action.type = "sound";
								ti.action.sound = CreateID(ini.get_value_string(1));
							}
						}
					}
				}
			}
		}
	}
	ini.close();
}

void scanTriggerZones(uint iClientID)	// Scan player ID's position and if inside a zone trigger the zone's action on them
{
	if (HkIsValidClientID(iClientID))	// Check ID valid
	{
		// Convert iClientID to iShip and see if in space
		uint iShip;
		pub::Player::GetShip(iClientID, iShip);

		if (iShip != 0) // If this iShip is actually in space and not docked (we can't get the position of a ship that isn't in space)
		{
			// Get ship system and position
			Vector pos;
			Matrix rot;
			uint iSystem;
			pub::SpaceObj::GetLocation(iShip, pos, rot);
			pub::SpaceObj::GetSystem(iShip, iSystem);

			// Loop through our triggers
			for (auto& ti : zones)
			{
				// Check to see if they are in the trigger system and within radius of trigger position
				if (iSystem == CreateID(ti.system.c_str()))
				{
					if (pos.x < ti.pos.x + ti.radius && pos.x > ti.pos.x - ti.radius && pos.y < ti.pos.y + ti.radius && pos.y > ti.pos.y - ti.radius && pos.z < ti.pos.z + ti.radius && pos.z > ti.pos.z - ti.radius)
					{
						// Calculate duration between time points:
						std::chrono::duration<double> durationElapsed = std::chrono::system_clock::now() - cooldownTimers[iClientID];

						if (cooldownTimers[iClientID] == NULL || durationElapsed.count() > 9)
						{
							// store the time we triggered this event:
							cooldownTimers[iClientID] = std::chrono::steady_clock::now();


							// Check what the trigger action is and execute it
							if (ti.action.type == "warp")
								HkRelocateClient(iClientID, ti.action.pos, rot);

							if (ti.action.type == "beam")
							{
								Universe::IBase* base = Universe::get_base(ti.action.base);
								pub::Player::ForceLand(iClientID, ti.action.base);

								// If not in the same system, emulate F1 charload
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

									// Actually move the player to the new location:
									// TO DO TO DO TO DO TO DO TO DO TO DO TO DO TO DO TO DO TO DO

									// Presumeably just this:?
									// HkRelocateClient(iClientID, ti.action.pos, rot);


									// TO DO TO DO TO DO TO DO TO DO TO DO TO DO TO DO TO DO TO DO

								}

							}

							if (ti.action.type == "heal")
							{
								float currentHealth;
								pub::SpaceObj::GetRelativeHealth(iShip, currentHealth);
								pub::SpaceObj::SetRelativeHealth(iShip, currentHealth + (ti.action.health / 100));
							}

							if (ti.action.type == "kill")
								pub::SpaceObj::Destroy(iShip, DestroyType::FUSE);

							if (ti.action.type == "chat")
								PrintUserCmdText(iClientID, ti.action.text);

							if (ti.action.type == "sound")
								pub::Audio::PlaySoundEffect(iClientID, ti.action.sound);
						}
					}
				}
			}
		}
	}
}


void updateInterval()	// Update scanInterval based on how many players are in-game
{
	clientsActiveNow = GetNumClients();
	if (clientsActiveNow)	// Stop division by 0. Probably a more elegant solution to this..
	{
		if (clientsActiveNow < 30)
			scanInterval = 60 / clientsActiveNow;	// Scan at same rate regardless of number of players (more players = faster scanning)
		else
			scanInterval = 1; // Limit scan interval to maximum rate
		
	} else {
		scanInterval = 60;
	}
}

void HkTick()	// Check to see if the scanInterval has elapsed every tick, and if so scan the next player online
{
	if (tickClock >= scanInterval)
	{
		updateInterval(); // update our scanInterval based on the number of players once per scan, ready for next scan	

		if (iClientIDs.size())
			scanTriggerZones(iClientIDs[iClientIndex]); // Perform the scan on the client

		tickClock = 0;	// reset our clock ready for the next countdown
		iClientIndex++;

		if (iClientIndex >= iClientIDs.size())
			iClientIndex = 0;
	}
	else
		tickClock++;
}

// This hook occurs every time the player count changes
void UpdatePlayerHook()
{
	updateClientIDs(); // Update the list of players ready for new scan. 
	scanInterval = 0; // Also make sure scan happens next tick.
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
	p_PI->sName = "area_triggers";
	p_PI->sShortName = "area_triggers";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTick, PLUGIN_HkIServerImpl_Update, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UpdatePlayerHook, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UpdatePlayerHook, PLUGIN_HkIServerImpl_DisConnect_AFTER, 0));
	return p_PI;
}
