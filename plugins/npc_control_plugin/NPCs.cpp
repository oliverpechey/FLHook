// Includes
#include "Main.h"

namespace NPCs {
// Structures and Global Variables
std::map<std::wstring, NPC_ARCHTYPESSTRUCT> mapNPCArchtypes;
std::map<std::wstring, NPC_FLEETSTRUCT> mapNPCFleets;
static std::map<int, NPC> mapNPCStartup;
std::list<uint> lstSpawnedNPCs;
std::vector<const char *> listgraphs;

// Logs the NPC being created to file in the EXE folder
void Log_CreateNPC(std::wstring name) {
    std::wstring wscMsgLog = L"created <%name>";
    wscMsgLog = ReplaceStr(wscMsgLog, L"%name", name.c_str());
    std::string scText = wstos(wscMsgLog);
    Utilities::Logging("%s", scText.c_str());
}

// Function that actually spawns the NPC
uint CreateNPC(std::wstring name, Vector pos, Matrix rot, uint iSystem,
               bool varyPos) {
    // Grab the arch info we have about this npc
    NPC_ARCHTYPESSTRUCT arch = mapNPCArchtypes[name];

    // Set the basic information about the NPC
    pub::SpaceObj::ShipInfo si;
    memset(&si, 0, sizeof(si));
    si.iFlag = 1;
    si.iSystem = iSystem;
    si.iShipArchetype = arch.Shiparch;
    si.mOrientation = rot;
    si.iLoadout = arch.Loadout;
    si.iLook1 = CreateID("li_newscaster_head_gen_hat");
    si.iLook2 = CreateID("pl_female1_journeyman_body");
    si.iComm = CreateID("comm_br_darcy_female");
    si.iPilotVoice = CreateID("pilot_f_leg_f01a");
    si.iHealth = -1;
    si.iLevel = 19;

    // Set the position in space of the NPC. This position can vary slightly if
    // required
    if (varyPos) {
        si.vPos.x = pos.x + Utilities::rand_FloatRange(0, 1000);
        si.vPos.y = pos.y + Utilities::rand_FloatRange(0, 1000);
        si.vPos.z = pos.z + Utilities::rand_FloatRange(0, 2000);
    } else {
        si.vPos.x = pos.x;
        si.vPos.y = pos.y;
        si.vPos.z = pos.z;
    }

    // Define the string used for the scanner name. Because the
    // following entry is empty, the pilot_name is used. This
    // can be overriden to display the ship type instead.
    FmtStr scanner_name(0, 0);
    scanner_name.begin_mad_lib(0);
    scanner_name.end_mad_lib();

    // Define the string used for the pilot name. The example
    // below shows the use of multiple part names.
    FmtStr pilot_name(0, 0);
    pilot_name.begin_mad_lib(16163); // ids of "%s0 %s1"
    if (arch.Infocard != 0) {
        pilot_name.append_string(arch.Infocard);
        if (arch.Infocard2 != 0) {
            pilot_name.append_string(arch.Infocard2);
        }
    } else {
        pilot_name.append_string(Utilities::rand_name()); // ids that replaces %s0
        pilot_name.append_string(
            Utilities::rand_name()); // ids that replaces %s1
    }
    pilot_name.end_mad_lib();

    pub::Reputation::Alloc(si.iRep, scanner_name, pilot_name);

    // Set the faction of the NPC
    pub::Reputation::SetAffiliation(si.iRep, arch.IFF);

    // Create the NPC in space
    uint iSpaceObj;
    pub::SpaceObj::Create(iSpaceObj, si);

    // Get the personality from the Pilot and the Graph
    pub::AI::SetPersonalityParams p;
    p.iStateGraph = pub::StateGraph::get_state_graph(
        listgraphs[arch.Graph], pub::StateGraph::TYPE_STANDARD);
    p.bStateID = true;
    get_personality(arch.Pilot, p.personality);
    pub::AI::SetPersonalityParams pers = p;
    pub::AI::SubmitState(iSpaceObj, &pers);

    // Add the NPC to our list
    lstSpawnedNPCs.push_back(iSpaceObj);
    return iSpaceObj;
}

// Function called by next function to remove spawned NPCs from our data 
bool IsFLHookNPC(CShip *ship) {
    // If it's a player do nothing
    if (ship->is_player() == true) {
        return false;
    }
    // Is it a FLHook NPC?
    std::list<uint>::iterator iter = lstSpawnedNPCs.begin();
    while (iter != lstSpawnedNPCs.end()) {
        if (*iter == ship->get_id()) {
            ship->clear_equip_and_cargo();
            lstSpawnedNPCs.erase(iter);
            Survival::NPCDestroyed(ship);
            return true;
            break;
        }
        iter++;
    }
    return false;
}

// Hook on ship destroyed to remove from our data
void __stdcall ShipDestroyed(DamageList *_dmg, DWORD *ecx, uint iKill) {
    returncode = DEFAULT_RETURNCODE;
    if (iKill) {
        CShip *cship = (CShip *)ecx[4];
        IsFLHookNPC(cship);
    }
}

// Load settings from the cfg file into our data structures
void LoadNPCInfo() {
    // The path to the configuration file.
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    std::string scPluginCfgFile =
        std::string(szCurDir) + "\\flhook_plugins\\npc.cfg";

    INI_Reader ini;
    if (ini.open(scPluginCfgFile.c_str(), false)) {
        while (ini.read_header()) {
            // These are the individual types of NPCs that can be spawned by the
            // plugin or by an admin
            if (ini.is_header("npcs")) {
                NPC_ARCHTYPESSTRUCT npc;
                while (ini.read_value()) {
                    if (ini.is_value("npc")) {
                        std::wstring wscName = stows(ini.get_value_string(0));
                        npc.Shiparch = CreateID(ini.get_value_string(1));
                        npc.Loadout = CreateID(ini.get_value_string(2));

                        // IFF calc
                        pub::Reputation::GetReputationGroup(
                            npc.IFF, ini.get_value_string(3));

                        // Selected graph + Pilot
                        npc.Graph = ini.get_value_int(4);
                        npc.Pilot = CreateID(ini.get_value_string(5));

                        // Infocard
                        npc.Infocard = ini.get_value_int(6);
                        npc.Infocard2 = ini.get_value_int(7);

                        mapNPCArchtypes[wscName] = npc;
                    }
                }
                // These are collections of NPCs that can be spawned all at once
            } else if (ini.is_header("fleet")) {
                NPC_FLEETSTRUCT fleet;
                while (ini.read_value()) {
                    if (ini.is_value("fleetname")) {
                        fleet.fleetname = stows(ini.get_value_string(0));
                    } else if (ini.is_value("fleetmember")) {
                        std::string setmembername = ini.get_value_string(0);
                        std::wstring membername = stows(setmembername);
                        int amount = ini.get_value_int(1);
                        fleet.fleetmember[membername] = amount;
                    }
                }
                mapNPCFleets[fleet.fleetname] = fleet;
                // Infocards to use for name generation when an infocard isn't
                // specified
            } else if (ini.is_header("names")) {
                while (ini.read_value()) {
                    if (ini.is_value("name")) {
                        Utilities::npcnames.push_back(ini.get_value_int(0));
                    }
                }
                // NPCs (that are loaded in above) that are to spawn on server
                // startup in static locations
            } else if (ini.is_header("startupnpcs")) {
                while (ini.read_value()) {
                    if (ini.is_value("startupnpc")) {
                        NPC n;
                        n.name = stows(ini.get_value_string(0));
                        n.pos.x = ini.get_value_float(1);
                        n.pos.y = ini.get_value_float(2);
                        n.pos.z = ini.get_value_float(3);
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
                        mapNPCStartup[mapNPCStartup.size()] = n;
                    }
                }
            }
        }
        ini.close();
    }
    Survival::LoadSurvivalSettings(scPluginCfgFile);
}

// Had to use this hookinstead of LoadSettings otherwise NPCs wouldnt appear on
// server startup
void Startup_AFTER() {
    returncode = DEFAULT_RETURNCODE;

    // Main Load Settings function, calls the one above.
    LoadNPCInfo();

    listgraphs.push_back("FIGHTER");   // 0
    listgraphs.push_back("TRANSPORT"); // 1
    listgraphs.push_back("GUNBOAT");   // 2
    listgraphs.push_back(
        "CRUISER"); // 3, doesn't seem to do anything

    // Spawns the NPCs configured on startup
    for (auto &[id, npc] : mapNPCStartup) {
        CreateNPC(npc.name, npc.pos, npc.rot, npc.system, false);
        Log_CreateNPC(npc.name);
    }
}
}