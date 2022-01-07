#pragma once

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <list>
#include <map>
#include <algorithm>
#include <FLHook.h>
#include <plugin.h>

struct CLIENT_DATA {
    bool bDisplayDPOnLaunch = true;
    int DeathPenaltyCredits = 0;
};

float set_fDeathPenalty = 0;
float set_fDeathPenaltyKiller = 0;
std::list<uint> ExcludedSystems;
std::map<uint, CLIENT_DATA> MapClients;
std::map<uint, float> FractionOverridesbyShip;

PLUGIN_RETURNCODE returncode;

typedef bool (*_UserCmdProc)(uint, const std::wstring &, const std::wstring &,
                             const wchar_t *);

struct USERCMD {
    wchar_t *wszCmd;
    _UserCmdProc proc;
    wchar_t *usage;
};

#define IS_CMD(a) !wscCmd.compare(L##a)