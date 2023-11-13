﻿/**
 * @date Jan, 2023
 * @author Lazrius
 * @defgroup Warehouse Warehouse
 * @brief
 * The Warehouse plugin allows players to deposit and withdraw various commodities and equipment on NPC stations, stored on internal SQL database.
 *
 * @paragraph cmds Player Commands
 * -warehouse store <ID> <count> - deposits specified equipment or commodity into the base warehouse
 * -warehouse list - lists unmounted equipment and commodities avalable for deposit
 * -warehouse withdraw <ID> <count> - transfers specified equipment or commodity into your ship cargo hold
 * -warehouse liststored - lists equipment and commodities deposited
 *
 * @paragraph adminCmds Admin Commands
 * None
 *
 * @paragraph configuration Configuration
 * @code
 * {
 *     "costPerStackStore": 100,
 *     "costPerStackWithdraw": 50,
 *     "restrictedBases": ["Li01_01_base", "Li01_02_base"],
 *     "restrictedItems": ["li_gun01_mark01","li_gun01_mark02"]
 * }
 * @endcode
 *
 * @paragraph ipc IPC Interfaces Exposed
 * This plugin does not expose any functionality.
 */

#include "Warehouse.h"
#include <Tools/Serialization/Attributes.hpp>

namespace Plugins::Warehouse
{
	const std::unique_ptr<Global> global = std::make_unique<Global>();

	void LoadSettings()
	{
		global->config = Serializer::JsonToObject<Config>();
		for (const auto& i : global->config.restrictedItems)
		{
			global->config.restrictedItemsHashed.emplace_back(CreateID(i.c_str()));
		}
		for (const auto& i : global->config.restrictedBases)
		{
			global->config.restrictedBasesHashed.emplace_back(CreateID(i.c_str()));
		}
		CreateSqlTables();
	}

	void UserCmdStoreItem(uint client, const std::wstring& param, uint base)
	{
		// This is a generated number to allow players to select the item they want to store.
		const uint databaseItemId = ToUInt(GetParam(param, ' ', 1));

		if (!databaseItemId)
		{
			PrintUserCmdText(client, L"Error Invalid Item Number");
			return;
		}

		int _;
		const auto cargo = Hk::Player::EnumCargo(client, _);
		std::vector<CARGO_INFO> filteredCargo;
		for (auto& info : cargo.value())
		{
			if (info.bMounted || info.fStatus < 1.f)
				continue;

			filteredCargo.emplace_back(info);
		}

		const uint itemCount = std::max(1u, ToUInt(GetParam(param, ' ', 2)));

		if (databaseItemId > filteredCargo.size())
		{
			PrintUserCmdText(client, L"Error Invalid Item Number");
			return;
		}
		const auto& item = filteredCargo[databaseItemId - 1];
		if (itemCount > static_cast<uint>(item.iCount))
		{
			PrintUserCmdText(client, L"Error Invalid Item Quantity");
			return;
		}
		if (std::ranges::find(global->config.restrictedItemsHashed, item.iArchId) != global->config.restrictedItemsHashed.end())
		{
			PrintUserCmdText(client, L"Error: This item is restricted from being stored.");
			return;
		}

		if (const uint cash = Hk::Player::GetCash(client).value(); cash < global->config.costPerStackStore)
		{
			PrintUserCmdText(
			    client, std::format(L"Not enough credits. The fee for storing items at this station is {} credits.", global->config.costPerStackStore));
			return;
		}

		Hk::Player::RemoveCash(client, global->config.costPerStackStore);
		Hk::Player::RemoveCargo(client, item.iId, itemCount);

		const auto account = Hk::Client::GetAccountByClientID(client);
		const auto sqlBaseId = GetOrAddBase(base);
		const auto sqlPlayerId = GetOrAddPlayer(sqlBaseId, account);
		const auto wareHouseItem = GetOrAddItem(item.iArchId, sqlPlayerId, itemCount);

		PrintUserCmdText(client, std::format(L"Successfully stored {} item(s) for a total of {}", itemCount, wareHouseItem.quantity, wareHouseItem.id));

		Hk::Player::SaveChar(client);
	}

	void UserCmdGetItems(uint client, [[maybe_unused]] const std::wstring& param, [[maybe_unused]] uint base)
	{
		int _;
		const auto cargo = Hk::Player::EnumCargo(client, _);

		int index = 0;
		for (const auto& info : cargo.value())
		{
			if (info.bMounted || info.fStatus < 1.f)
				continue;

			const auto* equip = Archetype::GetEquipment(info.iArchId);
			index++;
			PrintUserCmdText(client, std::format(L"{}) {} x{}", index, Hk::Message::GetWStringFromIdS(equip->iIdsName), info.iCount));
		}
	}

	void UserCmdGetWarehouseItems(uint client, const std::wstring& param, uint baseId)
	{
		const auto account = Hk::Client::GetAccountByClientID(client);
		const auto sqlBaseId = GetOrAddBase(baseId);
		const auto sqlPlayerId = GetOrAddPlayer(sqlBaseId, account);

		const auto paramCheck = GetParam(param, ' ', 1);
		if (paramCheck == L"all")
		{
			const auto baseMap = GetAllBases(sqlPlayerId);
			if (baseMap.empty())
			{
				PrintUserCmdText(client, L"You have no items stored anywhere.");
				return;
			}

			for (auto& i : baseMap)
			{
				const auto base = Universe::get_base(static_cast<uint>(i.first));
				const auto baseName = Hk::Message::GetWStringFromIdS(base->baseIdS);
				PrintUserCmdText(client, std::format(L"{} : {} item(s)", baseName, i.second.size()));
			}
			return;
		}

		const auto itemList = GetAllItemsOnBase(sqlPlayerId, baseId);
		if (itemList.empty())
		{
			PrintUserCmdText(client, L"You have no items stored at this warehouse.");
			return;
		}

		int index = 0;
		for (const auto& info : itemList)
		{
			const auto* equip = Archetype::GetEquipment(info.equipArchId);
			if (!equip)
			{
				Console::ConWarn(std::format("Item archetype {} no loner exists", info.equipArchId));
				continue;
			}
			index++;
			PrintUserCmdText(client, std::format(L"{}) {} x{}", index, Hk::Message::GetWStringFromIdS(equip->iIdsName), info.quantity));
		}
	}

	void UserCmdWithdrawItem(uint client, const std::wstring& param, uint base)
	{
		// This is a generated number to allow players to select the item they want to store.
		const uint itemId = ToInt(GetParam(param, ' ', 1));

		if (!itemId)
		{
			PrintUserCmdText(client, L"Error Invalid Item Number");
			return;
		}

		int remainingCargo;
		const auto cargo = Hk::Player::EnumCargo(client, remainingCargo);

		const int itemCount = std::max(1, ToInt(GetParam(param, ' ', 2)));

		if (const uint cash = Hk::Player::GetCash(client).value(); cash < global->config.costPerStackWithdraw)
		{
			PrintUserCmdText(
			    client, std::format(L"Not enough credits. The fee for storing items at this station is {} credits.", global->config.costPerStackWithdraw));
			return;
		}

		const auto account = Hk::Client::GetAccountByClientID(client);
		const auto sqlBaseId = GetOrAddBase(base);
		const auto sqlPlayerId = GetOrAddPlayer(sqlBaseId, account);
		const auto itemList = GetAllItemsOnBase(sqlPlayerId, sqlBaseId);

		if (itemId > itemList.size())
		{
			PrintUserCmdText(client, L"Error Invalid Item Number");
			return;
		}

		WareHouseItem warehouseItem = itemList.at(itemId - 1);

		const auto itemArch = Archetype::GetEquipment(warehouseItem.equipArchId);
		if (!itemArch)
		{
			Console::ConWarn("User tried to withdraw an item that no longer exists");
			PrintUserCmdText(client, L"Internal server error. Item does not exist.");
			return;
		}

		if (itemArch->fVolume * static_cast<float>(itemCount) > std::floor(remainingCargo))
		{
			PrintUserCmdText(client, L"Withdraw request denied. Your ship cannot accomodate cargo of this size");
			return;
		}

		const auto withdrawnQuantity = RemoveItem(warehouseItem.id, sqlPlayerId, itemCount);

		if (withdrawnQuantity == 0)
		{
			PrintUserCmdText(client, L"Invalid item Id");
			return;
		}

		Hk::Player::AddCargo(client, warehouseItem.equipArchId, static_cast<int>(withdrawnQuantity), false);
		Hk::Player::RemoveCash(client, global->config.costPerStackWithdraw);

		Hk::Player::SaveChar(client);

		PrintUserCmdText(client,
		    std::format(L"Successfully withdrawn Item: {} x{}", Hk::Message::GetWStringFromIdS(itemArch->iIdsName), std::to_wstring(withdrawnQuantity)));
	}

	void UserCmdWarehouse(ClientId& client, const std::wstring& param)
	{
		const std::wstring cmd = GetParam(param, ' ', 0);
		if (cmd.empty())
		{
			PrintUserCmdText(client,
			    L"Usage: /warehouse store <itemId> <count> : Stores the item number from /warehouse list and the count if it is a stackable item such as "
			    L"goods.\n"
			    L"Usage: /warehouse list :Lists any cargo or unmounted equipment that you may store in this base's warehouse.\n"
			    L"Usage: /warehouse withdraw <itemId> <count> : Withdraws the item number listed from /warehouse liststored and the amount and places it in "
			    L"your cargo.\n"
			    L"Usage: /warehouse liststored [all]: Lists the stored items you have in this base's warehouse. Stating all will show all bases you have items "
			    L"on.");
			return;
		}

		auto base = Hk::Player::GetCurrentBase(client);
		if (base.has_error())
		{
			PrintUserCmdText(client, L"You must be docked in order to use this command.");
			return;
		}
		auto baseVal = base.value();
		if (std::ranges::find(global->config.restrictedBasesHashed, baseVal) != global->config.restrictedBasesHashed.end())
		{
			PrintUserCmdText(client, L"Error: The base you're attempting to use warehouse on is restricted.");
			return;
		}

		if (cmd == L"store")
		{
			UserCmdStoreItem(client, param, base.value());
		}
		else if (cmd == L"list")
		{
			UserCmdGetItems(client, param, base.value());
		}
		else if (cmd == L"withdraw")
		{
			UserCmdWithdrawItem(client, param, base.value());
		}
		else if (cmd == L"liststored")
		{
			UserCmdGetWarehouseItems(client, param, base.value());
		}
		else
		{
			PrintUserCmdText(client, L"Invalid Command. Refer to /warehouse to see usage.");
		}
	}

	const std::vector commands = {{
	    CreateUserCommand(L"/warehouse", L"", UserCmdWarehouse, L""),
	}};

} // namespace Plugins::Warehouse

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace Plugins::Warehouse;

REFL_AUTO(type(Config), field(restrictedBases), field(restrictedItems), field(costPerStackWithdraw, AttrMin {0u}, AttrMax {100000u}),
    field(costPerStackStore, AttrMin {0u}, AttrMax {100000u}))

// Do things when the dll is loaded
BOOL WINAPI DllMain([[maybe_unused]] const HINSTANCE& hinstDLL, [[maybe_unused]] const DWORD fdwReason, [[maybe_unused]] const LPVOID& lpvReserved)
{
	return true;
}

// Functions to hook
extern "C" EXPORT void ExportPluginInfo(PluginInfo* pi)
{
	pi->name("warehouse");
	pi->shortName("warehouse");
	pi->mayUnload(true);
	pi->returnCode(&global->returnCode);
	pi->commands(&commands);
	pi->emplaceHook(HookedCall::FLHook__LoadSettings, &LoadSettings, HookStep::After);
	pi->versionMajor(PluginMajorVersion::VERSION_04);
	pi->versionMinor(PluginMinorVersion::VERSION_00);
}