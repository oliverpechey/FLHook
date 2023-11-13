#pragma once

#include <FLHook.hpp>
#include <plugin.h>

namespace Plugins::Rename
{
	//! Struct to hold a clan/group tag. This contains the passwords used to manage and rename a character to use this tag
	struct TagData : Reflectable
	{
		std::wstring tag;
		std::wstring masterPassword;
		std::wstring renamePassword;
		long long lastAccess;
		std::wstring description;
	};

	//! Struct to hold a pending rename of a character
	struct Rename
	{
		std::wstring charName;
		std::wstring newCharName;

		std::string sourceFile;
		std::string destFile;
	};

	//! Struct to hold a pending move of a character to a new account
	struct Move
	{
		std::wstring destinationCharName;
		std::wstring movingCharName;

		std::string sourceFile;
		std::string destFile;
	};

	//! A struct to hold all the tags
	struct TagList : Reflectable
	{
		std::string File() override
		{
			char szDataPath[MAX_PATH];
			GetUserDataPath(szDataPath);
			return std::string(szDataPath) + R"(\Accts\MultiPlayer\tags.json)";
		}

		std::vector<TagData> tags;
		std::vector<TagData>::iterator FindTag(const std::wstring& tag);
		std::vector<TagData>::iterator FindTagPartial(const std::wstring& tag);
	};

	//! Config data for this plugin
	struct Config : Reflectable
	{
		//! Json file the config is stored at
		std::string File() override { return "config/rename.json"; }

		//! Enable Rename
		bool enableRename = false;

		//! Enable Moving of Characters
		bool enableMoveChar = false;

		//! Cost of the rename in credits
		uint moveCost = 0;

		//! Cost of the rename in credits
		uint renameCost = 0;

		//! Rename is not allowed if attempted within the rename time limit (in seconds)
		int renameTimeLimit = 0;

		//! True if charname tags are supported
		bool enableTagProtection = false;

		//! True if ascii only tags are supported
		bool asciiCharNameOnly = false;

		//! The tag making cost
		uint makeTagCost = 2'500'000;
	};

	//! Minimum character name length constant
	constexpr int MinCharacterNameLength = 3;

	//! Global data for this plugin
	struct Global
	{
		ReturnCode returnCode = ReturnCode::Default;

		std::unique_ptr<Config> config = nullptr;
		std::vector<Move> pendingMoves;
		std::vector<Rename> pendingRenames;
		TagList tagList;
	};
} // namespace Plugins::Rename