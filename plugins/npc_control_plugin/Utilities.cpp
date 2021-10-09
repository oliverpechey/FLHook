// Includes
#include "Main.h"

namespace Utilities {

std::vector<const char *> listgraphs;
std::vector<uint> npcnames;
FILE *Logfile;

// Return random infocard ID from list that was loaded in
uint rand_name() {
    int randomIndex = rand() % npcnames.size();
    return npcnames.at(randomIndex);
}

// Function to log output (usually NPCs that have been created)
void Logging(const char *szString, ...) {
    if (Logfile == nullptr)
        fopen_s(&Logfile, "./flhook_logs/npc_log.log", "at");

    char szBufString[1024];
    va_list marker;
    va_start(marker, szString);
    _vsnprintf_s(szBufString, sizeof(szBufString) - 1, szString, marker);
    char szBuf[64];
    time_t tNow = time(0);
    tm t;
    localtime_s(&t, &tNow);
    strftime(szBuf, sizeof(szBuf), "%d/%m/%Y %H:%M:%S", &t);
    fprintf(Logfile, "%s %s\n", szBuf, szBufString);
    fflush(Logfile);
}

// Logs the NPC being created to file in the EXE folder
void Log_CreateNPC(std::wstring name) {
    std::wstring wscMsgLog = L"created <%name>";
    wscMsgLog = ReplaceStr(wscMsgLog, L"%name", name.c_str());
    std::string scText = wstos(wscMsgLog);
    Logging("%s", scText.c_str());
}

// Returns a random float between two numbers
float rand_FloatRange(float a, float b) {
    return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}

// Function that actually spawns the NPC
uint CreateNPC(std::wstring name, Vector pos, Matrix rot, uint iSystem,
               bool varyPos) {
    // Grab the arch info we have about this npc
    Main::NPC_ARCHTYPESSTRUCT arch = Main::mapNPCArchtypes[name];

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

    // Set the position in space of the NPC. This position can vary slightly if required
    if (varyPos) {
        si.vPos.x = pos.x + rand_FloatRange(0, 1000);
        si.vPos.y = pos.y + rand_FloatRange(0, 1000);
        si.vPos.z = pos.z + rand_FloatRange(0, 2000);
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
        pilot_name.append_string(rand_name()); // ids that replaces %s0
        pilot_name.append_string(rand_name()); // ids that replaces %s1
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
    Main::NPCs.push_back(iSpaceObj);
    return iSpaceObj;
}

void SendUniverseChatRedText(std::wstring wscXMLText) {
    // Encode message using the death message style (red text).
    std::wstring wscXMLMsg =
        L"<TRA data=\"" + set_wscDeathMsgStyleSys + L"\" mask=\"-1\"/> <TEXT>";
    wscXMLMsg += wscXMLText;
    wscXMLMsg += L"</TEXT>";

    char szBuf[0x1000];
    uint iRet;
    if (!HKHKSUCCESS(HkFMsgEncodeXML(wscXMLMsg, szBuf, sizeof(szBuf), iRet)))
        return;

    // Send to all players in system
    struct PlayerData *pPD = 0;
    while (pPD = Players.traverse_active(pPD)) {
        uint iClientID = HkGetClientIdFromPD(pPD);
        HkFMsgSendChat(iClientID, szBuf, iRet);
    }
}
} // namespace Utilities