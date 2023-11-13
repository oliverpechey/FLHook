/**
 * @date August, 2022
 * @author Ported from 88Flak by Raikkonen
 * @defgroup DeathPenalty Death Penalty
 * @brief
 * This plugin charges players credits for dying based on their ship worth. If the killer was a player it also rewards them.
 *
 * @paragraph cmds Player Commands
 * All commands are prefixed with '/' unless explicitly specified.
 * - dp - Shows the credits you would be charged if you died.
 *
 * @paragraph adminCmds Admin Commands
 * There are no admin commands in this plugin.
 *
 * @paragraph configuration Configuration
 * @code
 * {
 *     "DeathPenaltyFraction": 1.0,
 *     "DeathPenaltyFractionKiller": 1.0,
 *     "ExcludedSystems": ["li01"],
 *     "FractionOverridesByShip": {"ge_fighter": 1.0}
 * }
 * @endcode
 *
 * @paragraph ipc IPC Interfaces Exposed
 * This plugin does not expose any functionality.
 *
 * @paragraph optional Optional Plugin Dependencies
 * This plugin has no dependencies.
 */

#include "DeathPenalty.h"

namespace Plugins::DeathPenalty
{
	const std::unique_ptr<Global> global = std::make_unique<Global>();

	// Load configuration file
	void LoadSettings()
	{
		auto config = Serializer::JsonToObject<Config>();
		global->config = std::make_unique<Config>(config);

		for (const auto& system : config.ExcludedSystems)
			global->ExcludedSystemsIds.push_back(CreateID(system.c_str()));

		for (const auto& [shipNickname, penaltyFraction] : config.FractionOverridesByShip)
			global->FractionOverridesByShipIds[CreateID(shipNickname.c_str())] = penaltyFraction;
	}

	void ClearClientInfo(ClientId& client)
	{
		global->MapClients.erase(client);
	}

	/** @ingroup DeathPenalty
	 * @brief Is the player is a system that is excluded from death penalty?
	 */
	bool IsInExcludedSystem(ClientId client)
	{
		// Get System Id
		SystemId iSystemId = Hk::Player::GetSystem(client).value();
		// Search list for system
		return std::ranges::find(global->ExcludedSystemsIds, iSystemId) != global->ExcludedSystemsIds.end();
	}

	/** @ingroup DeathPenalty
	 * @brief This returns the override for the specific ship as defined in the json file.
	 * If there is not override it returns the default value defined as
	 * "DeathPenaltyFraction" in the json file
	 */
	float GetShipFractionOverride(ClientId client)
	{
		// Get ShipArchId
		uint shipArchId = Hk::Player::GetShipID(client).value();

		// Default return value is the default death penalty fraction
		float overrideValue = global->config->DeathPenaltyFraction;

		// See if the ship has an override fraction
		if (global->FractionOverridesByShipIds.contains(shipArchId))
			overrideValue = global->FractionOverridesByShipIds[shipArchId];

		return overrideValue;
	}

	/** @ingroup DeathPenalty
	 * @brief Hook on Player Launch. Used to work out the death penalty and display a message to the player warning them of such
	 */
	void PlayerLaunch([[maybe_unused]] const uint& ship, ClientId& client)
	{
		// No point in processing anything if there is no death penalty
		if (global->config->DeathPenaltyFraction > 0.00001f)
		{
			// Check to see if the player is in a system that doesn't have death
			// penalty
			if (!IsInExcludedSystem(client))
			{
				// Get the players net worth
				auto shipValue = Hk::Player::GetShipValue(client);
				if (shipValue.has_error())
				{
					Console::ConWarn(std::format("Unable to get ship value of undocking player: {}", wstos(Hk::Err::ErrGetText(shipValue.error()))));
					return;
				}

				const auto cash = Hk::Player::GetCash(client);
				if (cash.has_error())
				{
					Console::ConWarn(std::format("Unable to get cash of undocking player: {}", wstos(Hk::Err::ErrGetText(cash.error()))));
					return;
				}

				auto dpCredits = static_cast<uint>(static_cast<float>(shipValue.value()) * GetShipFractionOverride(client));
				if (cash < dpCredits)
					dpCredits = cash.value();

				// Calculate what the death penalty would be upon death
				global->MapClients[client].DeathPenaltyCredits = dpCredits;

				// Should we print a death penalty notice?
				if (global->MapClients[client].bDisplayDPOnLaunch)
					PrintUserCmdText(client,
					    std::format(L"Notice: the death penalty for your ship will be {} credits. Type /dp for more information.",
					        ToMoneyStr(global->MapClients[client].DeathPenaltyCredits)));
			}
			else
				global->MapClients[client].DeathPenaltyCredits = 0;
		}
	}

	/** @ingroup DeathPenalty
	 * @brief Load settings directly from the player's save directory
	 */
	void LoadUserCharSettings(ClientId& client)
	{
		// Get Account directory then flhookuser.ini file
		const CAccount* acc = Players.FindAccountFromClientID(client);
		std::wstring dir = Hk::Client::GetAccountDirName(acc);
		std::string scUserFile = CoreGlobals::c()->accPath + wstos(dir) + "\\flhookuser.ini";

		// Get char filename and save setting to flhookuser.ini
		const auto wscFilename = Hk::Client::GetCharFileName(client);
		std::string scFilename = wstos(wscFilename.value());
		std::string scSection = "general_" + scFilename;

		// read death penalty settings
		CLIENT_DATA c;
		c.bDisplayDPOnLaunch = IniGetB(scUserFile, scSection, "DPnotice", true);
		global->MapClients[client] = c;
	}

	/** @ingroup DeathPenalty
	 * @brief Apply the death penalty on a player death
	 */
	void PenalizeDeath(ClientId client, uint iKillerId)
	{
		if (global->config->DeathPenaltyFraction < 0.00001f)
			return;

		// Valid client and the ShipArch or System isnt in the excluded list?
		if (client != UINT_MAX && !IsInExcludedSystem(client))
		{
			// Get the players cash
			const auto cash = Hk::Player::GetCash(client);
			if (cash.has_error())
			{
				Console::ConWarn("Unable to get cash from client.");
				return;
			}

			// Get how much the player owes
			uint cashOwed = global->MapClients[client].DeathPenaltyCredits;

			// If the amount the player owes is more than they have, set the
			// amount to their total cash
			if (cashOwed > cash.value())
				cashOwed = cash.value();

			// If another player has killed the player
			if (iKillerId != client && (global->config->DeathPenaltyFractionKiller > 0.0f))
			{
				auto killerReward = static_cast<uint>(static_cast<float>(cashOwed) * global->config->DeathPenaltyFractionKiller);
				if (killerReward)
				{
					// Reward the killer, print message to them
					Hk::Player::AddCash(iKillerId, killerReward);
					PrintUserCmdText(iKillerId,
					    std::format(L"Death penalty: given {} credits from {}'s death penalty.",
					        ToMoneyStr(killerReward),
					        Hk::Client::GetCharacterNameByID(client).value()));
				}
			}

			if (cashOwed)
			{
				// Print message to the player and remove cash
				PrintUserCmdText(client, L"Death penalty: charged " + ToMoneyStr(cashOwed) + L" credits.");
				Hk::Player::RemoveCash(client, cashOwed);
			}
		}
	}

	/** @ingroup DeathPenalty
	 * @brief Hook on ShipDestroyed to kick off PenalizeDeath
	 */
	void ShipDestroyed(DamageList** _dmg, const DWORD** ecx, const uint& kill)
	{
		if (kill)
		{
			// Get client
			const CShip* cShip = Hk::Player::CShipFromShipDestroyed(ecx);
			ClientId client = cShip->GetOwnerPlayer();

			// Get Killer Id if there is one
			uint killerId = 0;
			if (client)
			{
				const DamageList* dmg = *_dmg;
				const auto inflictor = dmg->get_cause() == DamageCause::Unknown ? Hk::Client::GetClientIdByShip(ClientInfo[client].dmgLast.get_inflictor_id())
				                                                                : Hk::Client::GetClientIdByShip(dmg->get_inflictor_id());
				if (inflictor.has_value())
				{
					killerId = inflictor.value();
				}
				// Call function to penalize player and reward killer
				PenalizeDeath(client, killerId);
			}
		}
	}

	/** @ingroup DeathPenalty
	 * @brief This will save whether the player wants to receieve the /dp notice or not to the flhookuser.ini file
	 */
	void SaveDPNoticeToCharFile(ClientId client, const std::string& value)
	{
		const CAccount* acc = Players.FindAccountFromClientID(client);
		std::wstring dir = Hk::Client::GetAccountDirName(acc);
		if (const auto file = Hk::Client::GetCharFileName(client); file.has_value())
		{
			std::string scUserFile = CoreGlobals::c()->accPath + wstos(dir) + "\\flhookuser.ini";
			std::string scSection = "general_" + wstos(file.value());
			IniWrite(scUserFile, scSection, "DPnotice", value);
		}
	}

	/** @ingroup DeathPenalty
	 * @brief /dp command. Shows information about death penalty
	 */
	void UserCmd_DP(ClientId& client, const std::wstring& wscParam)
	{
		// If there is no death penalty, no point in having death penalty commands
		if (std::abs(global->config->DeathPenaltyFraction) < 0.0001f)
		{
			Console::ConWarn("DP Plugin active, but no/too low death penalty fraction is set.");
			return;
		}

		const auto param = ViewToWString(wscParam);
		if (wscParam.length()) // Arguments passed
		{
			if (ToLower(Trim(param)) == L"off")
			{
				global->MapClients[client].bDisplayDPOnLaunch = false;
				SaveDPNoticeToCharFile(client, "no");
				PrintUserCmdText(client, L"Death penalty notices disabled.");
			}
			else if (ToLower(Trim(param)) == L"on")
			{
				global->MapClients[client].bDisplayDPOnLaunch = true;
				SaveDPNoticeToCharFile(client, "yes");
				PrintUserCmdText(client, L"Death penalty notices enabled.");
			}
			else
			{
				PrintUserCmdText(client, L"ERR Invalid parameters");
				PrintUserCmdText(client, L"/dp on | /dp off");
			}
		}
		else
		{
			PrintUserCmdText(client, L"The death penalty is charged immediately when you die.");
			if (!IsInExcludedSystem(client))
			{
				auto shipValue = Hk::Player::GetShipValue(client);
				if (shipValue.has_error())
				{
					PrintUserCmdText(client, Hk::Err::ErrGetText(shipValue.error()));
				}
				auto cashOwed = static_cast<uint>(static_cast<float>(shipValue.value()) * GetShipFractionOverride(client));
				uint playerCash = Hk::Player::GetCash(client).value();

				PrintUserCmdText(client, std::format(L"The death penalty for your ship will be {} credits.", ToMoneyStr(std::min(cashOwed, playerCash))));
				PrintUserCmdText(client,
				    L"If you would like to turn off the death penalty notices, run "
				    L"this command with the argument \"off\".");
			}
			else
			{
				PrintUserCmdText(client,
				    L"You don't have to pay the death penalty "
				    L"because you are in a specific system.");
			}
		}
	}

	// Define usable chat commands here
	const std::vector commands = {{
	    CreateUserCommand(L"/dp", L"", UserCmd_DP, L"Shows the credits you would be charged if you died."),
	}};
} // namespace Plugins::DeathPenalty

using namespace Plugins::DeathPenalty;

REFL_AUTO(type(Config), field(DeathPenaltyFraction), field(DeathPenaltyFractionKiller), field(ExcludedSystems), field(FractionOverridesByShip))

DefaultDllMainSettings(LoadSettings);

// Functions to hook
extern "C" EXPORT void ExportPluginInfo(PluginInfo* pi)
{
	pi->name("Death Penalty");
	pi->shortName("death_penalty");
	pi->mayUnload(true);
	pi->commands(&commands);
	pi->returnCode(&global->returncode);
	pi->versionMajor(PluginMajorVersion::VERSION_04);
	pi->versionMinor(PluginMinorVersion::VERSION_00);
	pi->emplaceHook(HookedCall::FLHook__LoadSettings, &LoadSettings, HookStep::After);
	pi->emplaceHook(HookedCall::IEngine__ShipDestroyed, &ShipDestroyed);
	pi->emplaceHook(HookedCall::IServerImpl__PlayerLaunch, &PlayerLaunch);
	pi->emplaceHook(HookedCall::FLHook__LoadCharacterSettings, &LoadUserCharSettings);
	pi->emplaceHook(HookedCall::FLHook__ClearClientInfo, &ClearClientInfo, HookStep::After);
}
