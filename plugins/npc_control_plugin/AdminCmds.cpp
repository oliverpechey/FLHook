// Includes
#include "Main.h"

namespace AdminCmds {

// Admin command to make NPCs
void AdminCmd_AIMake(CCmds *cmds, int Amount, std::wstring NpcType) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    if (Amount == 0) {
        Amount = 1;
    }

    Main::NPC_ARCHTYPESSTRUCT arch;

    bool wrongnpcname = 0;

    std::map<std::wstring, Main::NPC_ARCHTYPESSTRUCT>::iterator iter =
        Main::mapNPCArchtypes.find(NpcType);
    if (iter != Main::mapNPCArchtypes.end()) {
        arch = iter->second;
    } else {
        cmds->Print(L"ERR Wrong NPC name\n");
        return;
    }

    uint iShip1;
    pub::Player::GetShip(HkGetClientIdFromCharname(cmds->GetAdminName()),
                         iShip1);
    if (!iShip1)
        return;

    uint iSystem;
    pub::Player::GetSystem(HkGetClientIdFromCharname(cmds->GetAdminName()),
                           iSystem);

    Vector pos;
    Matrix rot;
    pub::SpaceObj::GetLocation(iShip1, pos, rot);

    // Creation counter
    for (int i = 0; i < Amount; i++) {
        Utilities::CreateNPC(NpcType, pos, rot, iSystem, true);
        Utilities::Log_CreateNPC(NpcType);
    }

    return;
}

// Admin command to destroy the AI
void AdminCmd_AIKill(CCmds *cmds) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    for (auto &npc : Main::NPCs)
        pub::SpaceObj::Destroy(npc, DestroyType::FUSE);

    Main::NPCs.clear();
    cmds->Print(L"OK\n");

    return;
}

// Admin command to make AI come to your position
void AdminCmd_AICome(CCmds *cmds) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    uint iShip1;
    pub::Player::GetShip(HkGetClientIdFromCharname(cmds->GetAdminName()),
                         iShip1);
    if (iShip1) {
        Vector pos;
        Matrix rot;
        pub::SpaceObj::GetLocation(iShip1, pos, rot);

        for (auto &npc : Main::NPCs) {
            pub::AI::DirectiveCancelOp cancelOP;
            pub::AI::SubmitDirective(npc, &cancelOP);

            pub::AI::DirectiveGotoOp go;
            go.iGotoType = 1;
            go.vPos = pos;
            go.vPos.x = pos.x + Utilities::rand_FloatRange(0, 500);
            go.vPos.y = pos.y + Utilities::rand_FloatRange(0, 500);
            go.vPos.z = pos.z + Utilities::rand_FloatRange(0, 500);
            go.fRange = 0;
            pub::AI::SubmitDirective(npc, &go);
        }
    }
    cmds->Print(L"OK\n");
    return;
}

// Admin command to make AI follow target (or admin) until death
void AdminCmd_AIFollow(CCmds *cmds, std::wstring &wscCharname) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    // If no player specified follow the admin
    uint iClientId;
    if (wscCharname == L"") {
        iClientId = HkGetClientIdFromCharname(cmds->GetAdminName());
        wscCharname = cmds->GetAdminName();
    }
    // Follow the player specified
    else {
        iClientId = HkGetClientIdFromCharname(wscCharname);
    }
    if (iClientId == -1) {
        cmds->Print(L"%s is not online\n", wscCharname.c_str());
    } else {
        uint iShip1;
        pub::Player::GetShip(iClientId, iShip1);
        if (iShip1) {
            for (auto &npc : Main::NPCs) {
                pub::AI::DirectiveCancelOp cancelOP;
                pub::AI::SubmitDirective(npc, &cancelOP);
                pub::AI::DirectiveFollowOp testOP;
                testOP.iFollowSpaceObj = iShip1;
                testOP.fMaxDistance = 100;
                pub::AI::SubmitDirective(npc, &testOP);
            }
            cmds->Print(L"Following %s\n", wscCharname.c_str());
        } else {
            cmds->Print(L"%s is not in space\n", wscCharname.c_str());
        }
    }
    return;
}

// Admin command to cancel the current operation
void AdminCmd_AICancel(CCmds *cmds) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    uint iShip1;
    pub::Player::GetShip(HkGetClientIdFromCharname(cmds->GetAdminName()),
                         iShip1);
    if (iShip1) {
        for (auto &npc : Main::NPCs) {
            pub::AI::DirectiveCancelOp testOP;
            pub::AI::SubmitDirective(npc, &testOP);
        }
    }
    cmds->Print(L"OK\n");
    return;
}

// Admin command to list NPC fleets
void AdminCmd_ListNPCFleets(CCmds *cmds) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    cmds->Print(L"Available fleets: %d\n", Main::mapNPCFleets.size());
    for (auto &[name, npcstruct] : Main::mapNPCFleets)
        cmds->Print(L"|%s\n", name.c_str());

    cmds->Print(L"OK\n");

    return;
}

// Admin command to spawn a Fleet
void AdminCmd_AIFleet(CCmds *cmds, std::wstring FleetName) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    int wrongnpcname = 0;

    std::map<std::wstring, Main::NPC_FLEETSTRUCT>::iterator iter =
        Main::mapNPCFleets.find(FleetName);
    if (iter != Main::mapNPCFleets.end()) {
        Main::NPC_FLEETSTRUCT &fleetmembers = iter->second;
        for (auto &[name, amount] : fleetmembers.fleetmember)
            AdminCmd_AIMake(cmds, amount, name);
    } else {
        wrongnpcname = 1;
    }

    if (wrongnpcname == 1) {
        cmds->Print(L"ERR Wrong Fleet name\n");
        return;
    }

    cmds->Print(L"OK fleet spawned\n");
    return;
}

// Admin command processing
bool ExecuteCommandString_Callback(CCmds *cmds, const std::wstring &wscCmd) {
    Main::returncode = DEFAULT_RETURNCODE;
    if (IS_CMD("aicreate")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_AIMake(cmds, cmds->ArgInt(1), cmds->ArgStr(2));
        return true;
    } else if (IS_CMD("aidestroy")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_AIKill(cmds);
        return true;
    } else if (IS_CMD("aicancel")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_AICancel(cmds);
        return true;
    } else if (IS_CMD("aifollow")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_AIFollow(cmds, cmds->ArgCharname(1));
        return true;
    } else if (IS_CMD("aicome")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_AICome(cmds);
        return true;
    } else if (IS_CMD("aifleet")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_AIFleet(cmds, cmds->ArgStr(1));
        return true;
    } else if (IS_CMD("fleetlist")) {
        Main::returncode = SKIPPLUGINS_NOFUNCTIONCALL;
        AdminCmd_ListNPCFleets(cmds);
        return true;
    }
    return false;
}
}