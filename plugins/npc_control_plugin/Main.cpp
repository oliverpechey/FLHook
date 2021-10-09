// NPCs for FLHookPlugin
// December 2015 by BestDiscoveryHookDevs2015
// Extended by Raikkonen in 2021
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

// Includes
#include "Main.h"

PLUGIN_RETURNCODE returncode;

// Do things when the dll is loaded
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    srand((uint)time(0));
    // If we're being loaded from the command line while FLHook is running then
    // set_scCfgFile will not be empty so load the settings as FLHook only
    // calls load settings on FLHook startup and .rehash.
    if (fdwReason == DLL_PROCESS_ATTACH) {
        if (set_scCfgFile.length() > 0)
            NPCs::Startup_AFTER();
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
    p_PI->ePluginReturnCode = &returncode;
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NPCs::Startup_AFTER,
                                             PLUGIN_HkIServerImpl_Startup_AFTER,
                                             0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&AdminCmds::ExecuteCommandString_Callback,
                        PLUGIN_ExecuteCommandString_Callback, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NPCs::ShipDestroyed,
                                             PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&Survival::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&Survival::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Survival::PlayerLaunch,
                                             PLUGIN_HkIServerImpl_PlayerLaunch,
                                             0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&UserCmds::UserCmd_Process,
                                             PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&UserCmds::UserCmd_Help,
                                             PLUGIN_UserCmd_Help, 0));
    return p_PI;
}