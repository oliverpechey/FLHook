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

// Returns a random float between two numbers
float rand_FloatRange(float a, float b) {
    return ((b - a) * ((float)rand() / RAND_MAX)) + a;
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