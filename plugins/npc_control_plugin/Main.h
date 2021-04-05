#pragma once

#include <FLHook.h>
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <math.h>
#include <plugin.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <windows.h>

typedef bool (*_UserCmdProc)(uint, const std::wstring &, const std::wstring &,
                             const wchar_t *);

struct USERCMD {
    wchar_t *wszCmd;
    _UserCmdProc proc;
    wchar_t *usage;
};

#define IS_CMD(a) !wscCmd.compare(L##a)

namespace Main {

     extern PLUGIN_RETURNCODE returncode;

    struct NPC_ARCHTYPESSTRUCT {
        uint Shiparch;
        uint Loadout;
        uint IFF;
        uint Infocard;
        uint Infocard2;
        uint Pilot;
        int Graph;
    };

    struct NPC_FLEETSTRUCT {
        std::wstring fleetname;
        std::map<std::wstring, int> fleetmember;
    };
    
    struct NPC {
        std::wstring name;
        Vector pos;
        uint system;
        Matrix rot;
    };
    
    extern std::map<std::wstring, NPC_ARCHTYPESSTRUCT> mapNPCArchtypes;
    extern std::map<std::wstring, NPC_FLEETSTRUCT> mapNPCFleets;
    extern std::list<uint> NPCs;

    bool IsFLHookNPC(CShip *ship);
}

namespace Utilities {

    extern std::vector<const char *> listgraphs;
    extern std::vector<uint> npcnames;

    uint CreateNPC(std::wstring name, Vector pos, Matrix rot, uint iSystem,
               bool varyPos);
    void Log_CreateNPC(std::wstring name);
    void Logging(const char *szString, ...);
    uint rand_name();
    float rand_FloatRange(float a, float b);
    void SendUniverseChatRedText(std::wstring wscXMLText);
    }

namespace Survival {

    struct WAVE {
        std::list<std::wstring> NPCs;
        uint iReward;
    };

    struct SURVIVAL {
        uint iSystemID;
        Vector pos;
        std::vector<WAVE> Waves;
    };

    struct GAME {
        uint iWaveNumber;
        uint iGroupID;
        std::vector<uint> StoreMemberList;
        uint iSystemID;
        SURVIVAL Survival;
        std::list<uint> iSpawnedNPCs;

        bool operator==(GAME g) const {
            if (iSystemID == g.iSystemID)
                return true;
            return false;
        };
    };

    void LoadSurvivalSettings(const std::string &scPluginCfgFile);
    bool NewGame(uint iClientID, const std::wstring &wscCmd,
                 const std::wstring &wscParam, const wchar_t *usage);
    void NewWave(GAME &game);
    void NPCDestroyed(CShip *ship);
    void EndWave(GAME &game);
    void EndSurvival(GAME &game);
    void Disqualify(uint iClientID);
}

namespace AdminCmds {
    bool ExecuteCommandString_Callback(CCmds *cmds, const std::wstring &wscCmd);
}