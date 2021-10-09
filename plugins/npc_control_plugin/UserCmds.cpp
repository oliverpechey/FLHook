// Includes
#include "Main.h"

namespace UserCmds {

// Client command processing
USERCMD UserCmds[] = {
    {L"/survival", Survival::NewGame, L"Usage: /survival"},
};

bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd) {
    returncode = DEFAULT_RETURNCODE;

    std::wstring wscCmdLineLower = ToLower(wscCmd);

    // If the chat string does not match the USER_CMD then we do not handle the
    // command, so let other plugins or FLHook kick in. We require an exact
    // match
    for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++) {

        if (wscCmdLineLower.find(UserCmds[i].wszCmd) == 0) {
            // Extract the parameters string from the chat string. It should
            // be immediately after the command and a space.
            std::wstring wscParam = L"";
            if (wscCmd.length() > wcslen(UserCmds[i].wszCmd)) {
                if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
                    continue;
                wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
            }

            // Dispatch the command to the appropriate processing function.
            if (UserCmds[i].proc(iClientID, wscCmd, wscParam,
                                 UserCmds[i].usage)) {
                // We handled the command tell FL hook to stop processing this
                // chat string.
                returncode =
                    SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command,
                                                // return immediatly
                return true;
            }
        }
    }
    return false;
}

// Hook on /help
EXPORT void UserCmd_Help(uint iClientID, const std::wstring &wscParam) {
    PrintUserCmdText(iClientID, L"/survival ");
    PrintUserCmdText(iClientID,
                     L"Starts a Survival game. All members of the group need "
                     L"to be in space and in the same System.");
}
}