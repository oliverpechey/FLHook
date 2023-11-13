/**
 * @date Feb 2010
 * @author Cannon, ported by Raikkonen and Nen
 * @defgroup Restarts Restarts
 * @brief
 * The plugin allows the players to apply a predefined template onto their character (ship, location, reps).
 * Available restarts are stored as .fl files and should be located in EXE/config/restarts
 *
 * @paragraph cmds Player Commands
 * -showrestarts - lists available templates
 * -restart <restartName> - applies the chosen template onto this character
 *
 * @paragraph adminCmds Admin Commands
 * None
 *
 * @paragraph configuration Configuration
 * @code
 * {
 *     "availableRestarts": {"zoner": 0},
 *     "enableRestartCost": false,
 *     "maxCash": 1000000,
 *     "maxRank": 5
 * }
 *
 * @paragraph ipc IPC Interfaces Exposed
 * This plugin does not expose any functionality.
 */

#include "Restarts.h"

namespace Plugins::Restart
{
	const std::unique_ptr<Global> global = std::make_unique<Global>();

	void LoadSettings()
	{
		auto config = Serializer::JsonToObject<Config>();

		global->config = std::make_unique<Config>(config);
	}

	/* User Commands */

	void UserCmd_ShowRestarts(ClientId& client, [[maybe_unused]] const std::wstring& param)
	{
		if (global->config->availableRestarts.empty())
		{
			PrintUserCmdText(client, L"There are no restarts available.");
			return;
		}

		PrintUserCmdText(client, L"You can use these restarts:");
		for (const auto& [key, value] : global->config->availableRestarts)
		{
			if (global->config->enableRestartCost)
			{
				PrintUserCmdText(client, std::format(L"{} - ${}", key, value));
			}
			else
			{
				PrintUserCmdText(client, key);
			}
		}
	}

	void UserCmd_Restart(ClientId& client, const std::wstring& param)
	{
		std::wstring restartTemplate = GetParam(param, ' ', 0);
		if (!restartTemplate.length())
		{
			PrintUserCmdText(client, L"ERR Invalid parameters");
			PrintUserCmdText(client, L"/restart <template>");
		}

		// Get the character name for this connection.
		Restart restart;
		restart.characterName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));

		// Searching restart
		std::filesystem::path directory = "config\\restarts";
		if (!std::filesystem::exists(directory))
		{
			PrintUserCmdText(client, L"There has been an error with the restarts plugin. Please contact an Administrator.");
			AddLog(LogType::Normal, LogLevel::Err, "Missing restarts folder in config folder.");
			return;
		}

		for (const auto& entity : std::filesystem::directory_iterator(directory))
		{
			if (entity.is_directory() || entity.path().extension().string() != ".fl")
			{
				continue;
			}
			if (entity.path().filename().string() == wstos(restartTemplate + L".fl"))
			{
				restart.restartFile = entity.path().string();
			}
		}
		if (restart.restartFile.empty())
		{
			PrintUserCmdText(client, L"ERR Template does not exist");
			return;
		}

		// Saving the characters forces an anti-cheat checks and fixes
		// up a multitude of other problems.
		Hk::Player::SaveChar(client);
		if (!Hk::Client::IsValidClientID(client))
			return;

		if (auto base = Hk::Player::GetCurrentBase(client); base.has_error())
		{
			PrintUserCmdText(client, L"ERR Not in base");
			return;
		}

		if (global->config->maxRank != 0)
		{
			const auto rank = Hk::Player::GetRank(restart.characterName);
			if (rank.has_error() || rank.value() > global->config->maxRank)
			{
				PrintUserCmdText(client,
				    L"ERR You must create a new char to "
				    L"restart. Your rank is too high");
				return;
			}
		}

		const auto cash = Hk::Player::GetCash(restart.characterName);
		if (cash.has_error())
		{
			PrintUserCmdText(client, L"ERR " + Hk::Err::ErrGetText(cash.error()));
			return;
		}

		if (global->config->maxCash != 0 && cash > global->config->maxCash)
		{
			PrintUserCmdText(client,
			    L"ERR You must create a new char to "
			    L"restart. Your cash is too high");
			return;
		}

		if (global->config->enableRestartCost)
		{
			if (cash < global->config->availableRestarts[restartTemplate])
			{
				PrintUserCmdText(client,
				    L"You need $" + std::to_wstring(global->config->availableRestarts[restartTemplate] - cash.value()) + L" more credits to use this template");
				return;
			}
			restart.cash = cash.value() - global->config->availableRestarts[restartTemplate];
		}
		else
			restart.cash = cash.value();

		if (const CAccount* acc = Players.FindAccountFromClientID(client))
		{
			restart.directory = Hk::Client::GetAccountDirName(acc);
			restart.characterFile = Hk::Client::GetCharFileName(restart.characterName).value();
			global->pendingRestarts.push_back(restart);
			Hk::Player::KickReason(restart.characterName, L"Updating character, please wait 10 seconds before reconnecting");
		}
		return;
	}

	/* Hooks */
	void ProcessPendingRestarts()
	{
		while (global->pendingRestarts.size())
		{
			Restart restart = global->pendingRestarts.back();
			if (!Hk::Client::GetClientIdFromCharName(restart.characterName).has_error())
				return;

			global->pendingRestarts.pop_back();

			try
			{
				// Overwrite the existing character file
				std::string scCharFile = CoreGlobals::c()->accPath + wstos(restart.directory) + "\\" + wstos(restart.characterFile) + ".fl";
				std::string scTimeStampDesc = IniGetS(scCharFile, "Player", "description", "");
				std::string scTimeStamp = IniGetS(scCharFile, "Player", "tstamp", "0");
				if (!::CopyFileA(restart.restartFile.c_str(), scCharFile.c_str(), FALSE))
					throw std::runtime_error("copy template");

				FlcDecodeFile(scCharFile.c_str(), scCharFile.c_str());
				IniWriteW(scCharFile, "Player", "name", restart.characterName);
				IniWrite(scCharFile, "Player", "description", scTimeStampDesc);
				IniWrite(scCharFile, "Player", "tstamp", scTimeStamp);
				IniWrite(scCharFile, "Player", "money", std::to_string(restart.cash));

				if (!FLHookConfig::i()->general.disableCharfileEncryption)
					FlcEncodeFile(scCharFile.c_str(), scCharFile.c_str());

				AddLog(
				    LogType::Normal, LogLevel::Info, std::format("User restart {} for {}", restart.restartFile.c_str(), wstos(restart.characterName).c_str()));
			}
			catch (char* err)
			{
				AddLog(LogType::Normal, LogLevel::Err, std::format("User restart failed ({}) for {}", err, wstos(restart.characterName).c_str()));
			}
			catch (...)
			{
				AddLog(LogType::Normal, LogLevel::Err, std::format("User restart failed for {}", wstos(restart.characterName)));
			}
		}
	}

	const std::vector<Timer> timers = {{ProcessPendingRestarts, 1}};

	// Client command processing
	const std::vector commands = {{
	    CreateUserCommand(L"/restart", L"<name>", UserCmd_Restart, L"Restart with a template. This wipes your character!"),
	    CreateUserCommand(L"/showrestarts", L"", UserCmd_ShowRestarts, L"Shows the available restarts on the server."),
	}};
} // namespace Plugins::Restart
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace Plugins::Restart;

REFL_AUTO(type(Config), field(maxCash), field(maxRank), field(enableRestartCost), field(availableRestarts))

DefaultDllMainSettings(LoadSettings);

extern "C" EXPORT void ExportPluginInfo(PluginInfo* pi)
{
	pi->name("Restarts");
	pi->shortName("restarts");
	pi->mayUnload(true);
	pi->commands(&commands);
	pi->timers(&timers);
	pi->returnCode(&global->returnCode);
	pi->versionMajor(PluginMajorVersion::VERSION_04);
	pi->versionMinor(PluginMinorVersion::VERSION_00);
	pi->emplaceHook(HookedCall::FLHook__LoadSettings, &LoadSettings, HookStep::After);
}
