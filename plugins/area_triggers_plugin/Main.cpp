// Area Triggers v0.1 by [ZSS]~Lemming and [ZSS]~Raikkonen of the Zoner Shadow Syndicate.
// A plugin to detect when players are in certain locations and perform preset actions
// upon them defined in the plugin config files.
//
// Made for Zoner Universe - the best vanilla server
// www.zoneruniverse.com. No rights reserved.

#include "Main.h"

// Globals, Structs and Variables

struct Action				// ɐʇɐp uoıʇɔɐ ɹoɟ ɹǝuıɐʇuoɔ
{
	std::string type;		// ɯɹoɟɹǝd oʇ uoıʇɔɐ ɟo ǝdʎʇ ǝɥʇ
	Vector pos;				// (sdɹɐʍ ɹoɟ)  uoıʇɔɐ ǝɥʇ ɟo uoıʇısod p3
	uint base;				// (oʇ ƃuıɯɐǝq ɹoɟ pǝsn) pı ǝsɐq
	int health;				// ¿
	std::wstring text;		// ¿ʎɐןdsıp oʇ ʇxǝʇ
	uint sound;				// uoıʇɐʌıʇɔɐ uo ʎɐןd oʇ ǝןdɯɐs punos
};

struct TriggerItem			// ɐʇɐp ǝuoz ɹǝƃƃıɹʇ ɹoɟ ɹǝuıɐʇuoɔ
{
	std::string name;		// ¿
	Vector pos;				// ɹǝƃƃıɹʇ ǝɥʇ ɟo uoıʇısod p3
	int radius;				// The radius around the trigger which causes activation.
	std::string system;		// The system our trigger resides in.
	Action action;			// Action to take on trigger activation
};

std::vector< TriggerItem > triggers; // map of trigger zone positions

int tickClock = 0;		// Increments every server tick (up to scan interval). 
int scanInterval = 60;	// How often to scan a player location (changes based on player count).

// We only want to check the position of one player at once to avoid causing instability due to a spike of cpu use.
// This tracks which client to scan next tick
uint iClientID = 1;

///////////////////////////////////////////////////////////////////////////////////////

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
					for (auto& ti : triggers)
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

///////////////////////////////////////////////////////////////////////////////////////

void scanTriggerZones(uint iClientID)
{
	if (HkIsValidClientID(iClientID))
	{
		// Convert iClientID to iShip and see if in space
		uint iShip;
		pub::Player::GetShip(iClientID, iShip);

		if (iShip != 0)
		{
			// Get ship system and position
			Vector pos;
			Matrix rot;
			uint iSystem;
			pub::SpaceObj::GetLocation(iShip, pos, rot);
			pub::SpaceObj::GetSystem(iShip, iSystem);

			// Loop through our triggers
			for (auto& ti : triggers)
			{
				// Check to see if they are in the trigger system and within radius of trigger position
				if (iSystem == CreateID(ti.system.c_str()))
				{
					if (pos.x < ti.pos.x + ti.radius && pos.x > ti.pos.x - ti.radius && pos.y < ti.pos.y + ti.radius && pos.y > ti.pos.y - ti.radius && pos.z < ti.pos.z + ti.radius && pos.z > ti.pos.z - ti.radius)
					{
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
							}

						}

						if (ti.action.type == "heal")
							pub::SpaceObj::SetRelativeHealth(iShip, ti.action.health);

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

///////////////////////////////////////////////////////////////////////////////////////

// Update our scanInterval based on how many players are online. Return true if players are online
bool updateInterval()
{
	int clientsActiveNow = GetNumClients();
	if (clientsActiveNow)
	{
		if (clientsActiveNow < 30)
			scanInterval = 60 / clientsActiveNow; // TODO Replace 30 and 60 with max player count ( / 2 )
		else
			scanInterval = 1;

		if (iClientID > clientsActiveNow + 10) // To combat clients have an ID larger than the amount of players online
			iClientID = 1;

		return true;
		
	}
	// No clients are online so set large ScanInterval to reduce CPU load
	else {
		iClientID = 1;
		scanInterval = 100;
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////

// Hook into the connect of a player to reset scanInterval to 0
void OnConnect()
{
	scanInterval = 0;
}

///////////////////////////////////////////////////////////////////////////////////////

// Check to see if the scanInterval has elapsed
void HkTick()
{
	if (tickClock > scanInterval)
	{
		if (updateInterval()) {
			scanTriggerZones(iClientID);
			iClientID++;
		}
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
	p_PI->sName = "area_triggers";
	p_PI->sShortName = "area_triggers";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTick, PLUGIN_HkIServerImpl_Update, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&OnConnect, PLUGIN_HkIServerImpl_OnConnect_AFTER, 0));
	return p_PI;
}
