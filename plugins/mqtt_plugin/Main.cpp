// MQTT Plugin by Raikkonen
// This is designed to send messages to a MQTT broker based on events happening
// on the server. This can be used with other programs that connect to an MQTT
// broker to consume these messages e.g. a Discord bot
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

// Includes
#include "Main.h"

// JSON Library
using json = nlohmann::json;

// MQTT pointers
std::tuple<mqtt::async_client_ptr, mqtt::connect_options_ptr,
           mqtt::callback_ptr>
    mqttClient;

class callback : public virtual mqtt::callback {
  public:
    // Callback to handle disconnects
    void connection_lost(const std::string &cause) override {
        ConPrint(L"MQTT: Connection lost\n");
        if (!cause.empty())
            ConPrint(L"Cause: " + stows(cause));
    }

    // Callback to handle received messages
    void message_arrived(mqtt::const_message_ptr msg) override {
        if (msg->get_topic() == "allplayers")
            SendAllPlayers();
    }
};

// Connect to our MQTT Broker
std::tuple<mqtt::async_client_ptr, mqtt::connect_options_ptr,
           mqtt::callback_ptr>
ConnectBroker() {

    auto connOpts = std::make_shared<mqtt::connect_options>();
    connOpts->set_automatic_reconnect(5, 120);
    auto client = std::make_shared<mqtt::async_client>("tcp://localhost:1883",
                                                       "FLServer");
    auto cb = std::make_shared<callback>();
    client->set_callback(*cb);

    try {
        ConPrint(L"MQTT: connecting...\n");
        mqtt::token_ptr conntok = client->connect(*connOpts);

        // Wait for connection to be established TODO: This just waits if the
        // mqtt server is down and lags the server
        ConPrint(L"MQTT: Waiting for connection...\n");
        conntok->wait();
        ConPrint(L"MQTT: Connected\n");

        // Subscribe to /allplayers
        const std::string TOPIC("allplayers");
        client->subscribe(TOPIC, 0);

    } catch (const mqtt::exception &exc) {
        ConPrint(L"MQTT: error " + stows(exc.what()) + L"\n");
    }
    return std::make_tuple(client, connOpts, cb);
}

// Load configuration file
void LoadSettings() {
    returncode = DEFAULT_RETURNCODE;

    // The path to the configuration file.
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    std::string configFile =
        std::string(szCurDir) + "\\flhook_plugins\\MQTT_Plugin.ini";

    INI_Reader ini;
    if (ini.open(configFile.c_str(), false)) {
        while (ini.read_header()) {
            if (ini.is_header("General")) {
                while (ini.read_value()) {
                    if (ini.is_value("debug")) {
                        set_iPluginDebug = ini.get_value_int(0);
                    }
                }
            }
        }

        if (set_iPluginDebug & 1) {
            ConPrint(L"Debug\n");
        }

        ini.close();
    }

    mqttClient = ConnectBroker();
}

void SendJson(std::string topic, json json) {
    // Create payload for mqtt message
    mqtt::message_ptr pubmsg = mqtt::make_message(topic, json.dump());
    pubmsg->set_qos(1);

    // Check to ensure we are connected to the broker
    if (!std::get<0>(mqttClient)->is_connected())
        mqttClient = ConnectBroker();

    // Send the message
    std::get<0>(mqttClient)->publish(pubmsg);
}

void SendPlayerMessage(uint iClientID, bool online) {
    json jPlayerContainer;
    json jPlayer;

    std::wstring wscCharname =
        (const wchar_t *)Players.GetActiveCharacterName(iClientID);

    int rank;
    HkGetRank(wscCharname, rank);

    std::wstring ip;
    HkGetPlayerIP(iClientID, ip);

    jPlayer["name"] = wstos(wscCharname);
    jPlayer["online"] = online;
    jPlayer["id"] = iClientID;
    jPlayer["rank"] = rank;
    jPlayer["ip"] = wstos(ip);
    jPlayer["system"] = HkGetPlayerSystemS(iClientID);

    jPlayerContainer.push_back(jPlayer);

    SendJson("players", jPlayerContainer);
}

void SendAllPlayers() {
    struct PlayerData *pPD = 0;
    while (pPD = Players.traverse_active(pPD)) {
        SendPlayerMessage(HkGetClientIdFromPD(pPD), true);
    }
}

void __stdcall PlayerLaunch_AFTER(uint iShip, uint iClientID) {
    returncode = DEFAULT_RETURNCODE;
    SendPlayerMessage(iClientID, true);
}

void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection state) {
    returncode = DEFAULT_RETURNCODE;
    SendPlayerMessage(iClientID, false);
}

// This hook fires around once a second. Using this to report load times back to
// the MQTT server
void __stdcall CheckKick() {
    returncode = DEFAULT_RETURNCODE;

    // Create payload for load message
    mqtt::message_ptr loadmsg =
        mqtt::make_message("load", std::to_string(g_iServerLoad));
    loadmsg->set_qos(1);

    // Check to ensure we are connected to the broker
    if (!std::get<0>(mqttClient)->is_connected())
        mqttClient = ConnectBroker();

    // Send the message
    std::get<0>(mqttClient)->publish(loadmsg);
}

// Send chat to MQTT server
void __stdcall SubmitChat(struct CHAT_ID cId, unsigned long lP1,
                          void const *rdlReader, struct CHAT_ID cIdTo,
                          int iP2) {
    returncode = DEFAULT_RETURNCODE;

    // Ignore group join/leave commands
    if (cIdTo.iID == 0x10004)
        return;

    // Extract text from rdlReader
    BinaryRDLReader rdl;
    wchar_t wszBuf[1024];
    uint iRet1;
    rdl.extract_text_from_buffer((unsigned short *)wszBuf, sizeof(wszBuf),
                                 iRet1, (const char *)rdlReader, lP1);

    std::wstring wscChatMsg = ToLower(wszBuf);

    bool bIsGroup = (cIdTo.iID == 0x10003 || !wscChatMsg.find(L"/g ") ||
                     !wscChatMsg.find(L"/group "));

    std::wstring wscCharname =
        (const wchar_t *)Players.GetActiveCharacterName(cId.iID);

    json jChat;
    jChat["from"] = wstos(wscCharname);

    switch (cIdTo.iID) {
    case 0x10000:
        jChat["to"] = "Universe";
        break;
    case 0x10001:
        jChat["to"] = "System";
        break;
    case 0x10002:
        jChat["to"] = "Local";
        break;
    case 0x10003:
        jChat["to"] = "Group";
        break;
    default:
        std::wstring wscCharname =
            (const wchar_t *)Players.GetActiveCharacterName(cIdTo.iID);
        jChat["to"] = wstos(wscCharname);
    }

    jChat["msg"] = wstos(wszBuf);
    SendJson("chat", jChat);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Do things when the dll is loaded
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    srand((uint)time(0));

    // If we're being loaded from the command line while FLHook is running then
    // set_scCfgFile will not be empty so load the settings as FLHook only
    // calls load settings on FLHook startup and .rehash.
    if (fdwReason == DLL_PROCESS_ATTACH && set_scCfgFile.length() > 0)
        LoadSettings();

    return true;
}

// Functions to hook
EXPORT PLUGIN_INFO *Get_PluginInfo() {
    PLUGIN_INFO *p_PI = new PLUGIN_INFO();
    p_PI->sName = "MQTT by Raikkonen";
    p_PI->sShortName = "MQTT";
    p_PI->bMayPause = true;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &returncode;
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&LoadSettings, PLUGIN_LoadSettings, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&PlayerLaunch_AFTER,
                        PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&CheckKick, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&SubmitChat, PLUGIN_HkIServerImpl_SubmitChat, 0));
    return p_PI;
}
