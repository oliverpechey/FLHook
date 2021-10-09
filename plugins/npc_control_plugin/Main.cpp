﻿// NPCs for FLHookPlugin
// December 2015 by BestDiscoveryHookDevs2015
// Extended by Raikkonen in 2021
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

// Includes
#include "Main.h"

namespace Main {

// Structures and Global Variables
std::map<std::wstring, NPC_ARCHTYPESSTRUCT> mapNPCArchtypes;
std::map<std::wstring, NPC_FLEETSTRUCT> mapNPCFleets;
static std::map<int, NPC> mapNPCStartup;
std::list<uint> lstSpawnedNPCs;

PLUGIN_RETURNCODE returncode;

// Function called by next function to remove spawned NPCs from our data
bool IsFLHookNPC(CShip *ship) {
    // If it's a player do nothing
    if (ship->is_player() == true) {
        return false;
    }

    // Is it a FLHook NPC?
    std::list<uint>::iterator iter = Main::lstSpawnedNPCs.begin();
    while (iter != Main::lstSpawnedNPCs.end()) {
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

// Disqualify from survival if these hooks go off
void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection state) {
    returncode = DEFAULT_RETURNCODE;
    Survival::Disqualify(iClientID);
}

void __stdcall BaseEnter(unsigned int iBaseID, unsigned int iClientID) {
    returncode = DEFAULT_RETURNCODE;
    Survival::Disqualify(iClientID);
}

void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID) {
    returncode = DEFAULT_RETURNCODE;
    Survival::Disqualify(iClientID);
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
            // These are the individual types of NPCs that can be spawned by the plugin or by an admin
            if (ini.is_header("npcs")) {
                NPC_ARCHTYPESSTRUCT npc;
                while (ini.read_value()) {
                    if (ini.is_value("npc")) {
                        std::wstring wscName =
                            stows(ini.get_value_string(0));
                        npc.Shiparch =
                            CreateID(ini.get_value_string(1));
                        npc.Loadout =
                            CreateID(ini.get_value_string(2));

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
            // Infocards to use for name generation when an infocard isn't specified
            } else if (ini.is_header("names")) {
                while (ini.read_value()) {
                    if (ini.is_value("name")) {
                        Utilities::npcnames.push_back(ini.get_value_int(0));
                    }
                }
            // NPCs (that are loaded in above) that are to spawn on server startup in static locations
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

// Had to use this hookinstead of LoadSettings otherwise NPCs wouldnt appear on server startup
void Startup_AFTER() {
    returncode = DEFAULT_RETURNCODE;

     // Main Load Settings function, calls the one above.
    LoadNPCInfo();

    Utilities::listgraphs.push_back("FIGHTER"); // 0
    Utilities::listgraphs.push_back("TRANSPORT"); // 1
    Utilities::listgraphs.push_back("GUNBOAT");   // 2
    Utilities::listgraphs.push_back(
        "CRUISER"); // 3, doesn't seem to do anything

    // Spawns the NPCs configured on startup
    for (auto &[id, npc] : mapNPCStartup) {
        Utilities::CreateNPC(npc.name, npc.pos, npc.rot, npc.system, false);
        Utilities::Log_CreateNPC(npc.name);
    }
}
} // namespace Main

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Do things when the dll is loaded
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    srand((uint)time(0));
    // If we're being loaded from the command line while FLHook is running then
    // set_scCfgFile will not be empty so load the settings as FLHook only
    // calls load settings on FLHook startup and .rehash.
    if (fdwReason == DLL_PROCESS_ATTACH) {
        if (set_scCfgFile.length() > 0)
            Main::Startup_AFTER();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
    }
    return true;
}

// Functions to hook
EXPORT PLUGIN_INFO *Get_PluginInfo() {
    PLUGIN_INFO *p_PI = new PLUGIN_INFO();
    p_PI->sName = "NPCs by Alley, Cannon (2015), Raikkonen (2021)";
    p_PI->sShortName = "npc";
    p_PI->bMayPause = true;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &Main::returncode;
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Main::Startup_AFTER,
                                             PLUGIN_HkIServerImpl_Startup_AFTER,
                                             0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&AdminCmds::ExecuteCommandString_Callback,
                        PLUGIN_ExecuteCommandString_Callback, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Main::ShipDestroyed,
                                             PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&Main::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&Main::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Main::PlayerLaunch,
                                             PLUGIN_HkIServerImpl_PlayerLaunch,
                                             0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&UserCmds::UserCmd_Process,
                                             PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&UserCmds::UserCmd_Help,
                                             PLUGIN_UserCmd_Help, 0));
    return p_PI;
}