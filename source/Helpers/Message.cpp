#include <Global.hpp>

// Very hacky way to call non-header function
namespace IServerImplHook
{
	void __stdcall SubmitChat(CHAT_ID cidFrom, ulong size, void const* rdlReader, CHAT_ID cidTo, int _genArg1);
}

bool g_bMsg;
bool g_bMsgU;
bool g_bMsgS;

_RCSendChatMsg RCSendChatMsg;

namespace Hk::Message
{
	cpp::result<void, Error> Msg(const std::variant<uint, std::wstring>& player, const std::wstring& message)
	{
		ClientId client = Hk::Client::ExtractClientID(player);

		if (client == UINT_MAX)
			return cpp::fail(Error::PlayerNotLoggedIn);

		const CHAT_ID ci = {0};
		const CHAT_ID ciClient = {client};

		const std::wstring wscXML = L"<TRA data=\"0x19BD3A00\" mask=\"-1\"/><TEXT>" + XMLText(message) + L"</TEXT>";
		uint iRet;
		char szBuf[1024];
		if (const auto err = FMsgEncodeXML(wscXML, szBuf, sizeof(szBuf), iRet); err.has_error())
			return cpp::fail(err.error());

		IServerImplHook::SubmitChat(ci, iRet, szBuf, ciClient, -1);
		return {};
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> MsgS(const std::variant<std::wstring, uint>& system, const std::wstring& message)
	{
		uint systemId = 0;
		if (!system.index())
		{
			std::wstring systemName = std::get<std::wstring>(system);
			pub::GetSystemID(systemId, wstos(systemName).c_str());
		}
		else
		{
			systemId = std::get<uint>(system);
		}

		// prepare xml
		const std::wstring xml = L"<TRA data=\"0xE6C68400\" mask=\"-1\"/><TEXT>" + XMLText(message) + L"</TEXT>";
		uint ret;
		char buffer[1024];
		if (const auto err = FMsgEncodeXML(xml, buffer, sizeof(buffer), ret); err.has_error())
			return cpp::fail(err.error());

		const CHAT_ID ci = {0};

		// for all players in system...

		for (auto player : Hk::Client::getAllPlayersInSystem(systemId))
		{
			const CHAT_ID ciClient = {player};
			IServerImplHook::SubmitChat(ci, ret, buffer, ciClient, -1);
		}

		return {};
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> MsgU(const std::wstring& message)
	{
		const CHAT_ID ci = {0};
		const CHAT_ID ciClient = {0x00010000};

		const std::wstring xml = L"<TRA font=\"1\" color=\"#FFFFFF\"/><TEXT>" + XMLText(message) + L"</TEXT>";
		uint iRet;
		char szBuf[1024];
		if (const auto err = FMsgEncodeXML(xml, szBuf, sizeof(szBuf), iRet); err.has_error())
			return cpp::fail(err.error());

		IServerImplHook::SubmitChat(ci, iRet, szBuf, ciClient, -1);
		return {};
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> FMsgEncodeXML(const std::wstring& xmlString, char* buffer, uint size, uint& ret)
	{
		XMLReader rdr;
		RenderDisplayList rdl;
		std::wstring wscMsg = L"<?xml version=\"1.0\" encoding=\"UTF-16\"?><RDL><PUSH/>";
		wscMsg += xmlString;
		wscMsg += L"<PARA/><POP/></RDL>\x000A\x000A";
		if (!rdr.read_buffer(rdl, (const char*)wscMsg.c_str(), wscMsg.length() * 2))
			return cpp::fail(Error::WrongXmlSyntax);

		BinaryRDLWriter rdlwrite;
		rdlwrite.write_buffer(rdl, buffer, size, ret);
		return {};
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void FMsgSendChat(ClientId client, char* buffer, uint size)
	{
		auto p4 = (uint)buffer;
		uint p3 = size;
		uint p2 = 0x00010000;
		uint p1 = client;

		__asm {
        push [p4]
        push [p3]
        push [p2]
        push [p1]
        mov ecx, [HookClient]
        add ecx, 4
        call [RCSendChatMsg]
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> FMsg(ClientId client, const std::wstring& xmlString)
	{
		char szBuf[0xFFFF];
		uint iRet;
		if (const auto err = FMsgEncodeXML(xmlString, szBuf, sizeof(szBuf), iRet); err.has_error())
			return cpp::fail(err.error());

		FMsgSendChat(client, szBuf, iRet);
		return {};
	}

	cpp::result<void, Error> FMsg(const std::variant<uint, std::wstring>& player, const std::wstring& xmlString)
	{
		ClientId client = Hk::Client::ExtractClientID(player);

		if (client == UINT_MAX)
			return cpp::fail(Error::PlayerNotLoggedIn);

		return FMsg(client, xmlString);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> FMsgS(const std::variant<std::wstring, uint>& system, const std::wstring& xmlString)
	{
		uint systemId = 0;
		if (!system.index())
		{
			std::wstring systemName = std::get<std::wstring>(system);
			pub::GetSystemID(systemId, wstos(systemName).c_str());
		}
		else
		{
			systemId = std::get<uint>(system);
		}
		// encode xml std::string
		char szBuf[0xFFFF];
		uint iRet;
		if (const auto err = FMsgEncodeXML(xmlString, szBuf, sizeof(szBuf), iRet); err.has_error())
			return cpp::fail(err.error());

		// for all players in system...
		for (auto player : Hk::Client::getAllPlayersInSystem(systemId))
		{
			FMsgSendChat(player, szBuf, iRet);
		}

		return {};
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	cpp::result<void, Error> FMsgU(const std::wstring& xmlString)
	{
		// encode xml std::string
		char szBuf[0xFFFF];
		uint iRet;
		const auto err = FMsgEncodeXML(xmlString, szBuf, sizeof(szBuf), iRet);
		if (err.has_error())
			return cpp::fail(err.error());

		// for all players
		PlayerData* playerDb = nullptr;
		while ((playerDb = Players.traverse_active(playerDb)))
		{
			ClientId client = playerDb->iOnlineId;
			FMsgSendChat(client, szBuf, iRet);
		}

		return {};
	}

	/** Format a chat std::string in accordance with the receiver's preferences and
	send it. Will check that the receiver accepts messages from sender and
	refuses to send if necessary. */
	cpp::result<void, Error> FormatSendChat(uint toClientId, const std::wstring& sender, const std::wstring& text, const std::wstring& textColor)
	{
#define HAS_FLAG(a, b) ((a).wscFlags.find(b) != -1)

		if (FLHookConfig::i()->userCommands.userCmdIgnore)
		{
			for (const auto& ignore : ClientInfo[toClientId].lstIgnore)
			{
				if (!HAS_FLAG(ignore, L"i") && !(ToLower(sender).compare(ToLower(ignore.character))))
					return {}; // ignored
				else if (HAS_FLAG(ignore, L"i") && (ToLower(sender).find(ToLower(ignore.character)) != -1))
					return {}; // ignored
			}
		}

		uchar cFormat;
		// adjust chatsize
		switch (ClientInfo[toClientId].chatSize)
		{
			case CS_SMALL:
				cFormat = 0x90;
				break;
			case CS_BIG:
				cFormat = 0x10;
				break;
			default:
				cFormat = 0x00;
				break;
		}

		// adjust chatstyle
		switch (ClientInfo[toClientId].chatStyle)
		{
			case CST_BOLD:
				cFormat += 0x01;
				break;
			case CST_ITALIC:
				cFormat += 0x02;
				break;
			case CST_UNDERLINE:
				cFormat += 0x04;
				break;
			default:
				cFormat += 0x00;
				break;
		}

		wchar_t wszFormatBuf[8];
		swprintf(wszFormatBuf, _countof(wszFormatBuf), L"%02X", (long)cFormat);
		const std::wstring wscTRADataFormat = wszFormatBuf;
		const std::wstring wscTRADataSenderColor = L"FFFFFF"; // white

		const std::wstring wscXML = L"<TRA data=\"0x" + wscTRADataSenderColor + wscTRADataFormat + L"\" mask=\"-1\"/><TEXT>" + XMLText(sender) +
		    L": </TEXT>" + L"<TRA data=\"0x" + textColor + wscTRADataFormat + L"\" mask=\"-1\"/><TEXT>" + XMLText(text) + L"</TEXT>";

		if (const auto err = FMsg(toClientId, wscXML); err.has_error())
			return cpp::fail(err.error());

		return {};
	}

	/** Send a player to player message */
	cpp::result<void, Error> SendPrivateChat(uint fromClientId, uint toClientId, const std::wstring& text)
	{
		const auto wscSender = Client::GetCharacterNameByID(fromClientId);
		if (wscSender.has_error())
		{
			Console::ConErr(std::format("Unable to send private chat message from client {}", fromClientId));
			return {};
		}

		if (FLHookConfig::i()->userCommands.userCmdIgnore)
		{
			for (auto const& ignore : ClientInfo[toClientId].lstIgnore)
			{
				if (HAS_FLAG(ignore, L"p"))
					return {};
			}
		}

		// Send the message to both the sender and receiver.
		auto err = FormatSendChat(toClientId, wscSender.value(), text, L"19BD3A");
		if (err.has_error())
			return cpp::fail(err.error());

		err = FormatSendChat(fromClientId, wscSender.value(), text, L"19BD3A");
		if (err.has_error())
			return cpp::fail(err.error());

		return {};
	}

	/** Send a player to system message */
	void SendSystemChat(uint fromClientId, const std::wstring& text)
	{
		const std::wstring wscSender = (const wchar_t*)Players.GetActiveCharacterName(fromClientId);

		// Get the player's current system.
		uint systemId;
		pub::Player::GetSystem(fromClientId, systemId);

		// For all players in system...
		for (auto player : Hk::Client::getAllPlayersInSystem(systemId))
		{
			// Send the message a player in this system.
			FormatSendChat(player, wscSender, text, L"E6C684");
		}
	}

	/** Send a player to local system message */
	void SendLocalSystemChat(uint fromClientId, const std::wstring& text)
	{
		// Don't even try to send an empty message
		if (text.empty())
		{
			return;
		}

		const auto wscSender = Client::GetCharacterNameByID(fromClientId);
		if (wscSender.has_error())
		{
			Console::ConErr(std::format("Unable to send local system chat message from client {}", fromClientId));
			return;
		}

		// Get the player's current system and location in the system.
		uint systemId;
		pub::Player::GetSystem(fromClientId, systemId);

		uint iFromShip;
		pub::Player::GetShip(fromClientId, iFromShip);

		Vector vFromShipLoc;
		Matrix mFromShipDir;
		pub::SpaceObj::GetLocation(iFromShip, vFromShipLoc, mFromShipDir);

		// For all players in system...
		for (auto player : Hk::Client::getAllPlayersInSystem(systemId))
		{
			uint ship;
			pub::Player::GetShip(player, ship);

			Vector vShipLoc;
			Matrix mShipDir;
			pub::SpaceObj::GetLocation(ship, vShipLoc, mShipDir);

			// Cheat in the distance calculation. Ignore the y-axis
			// Is player within scanner range (15K) of the sending char.
			if (static_cast<float>(sqrt(pow(vShipLoc.x - vFromShipLoc.x, 2) + pow(vShipLoc.z - vFromShipLoc.z, 2))) > 14999.0f)
				continue;

			// Send the message a player in this system.
			FormatSendChat(player, wscSender.value(), text, L"FF8F40");
		}
	}

	/** Send a player to group message */
	void SendGroupChat(uint fromClientId, const std::wstring& text)
	{
		auto wscSender = (const wchar_t*)Players.GetActiveCharacterName(fromClientId);
		// Format and send the message a player in this group.
		auto lstMembers = Hk::Player::GetGroupMembers(wscSender);
		if (lstMembers.has_error())
		{
			return;
		}

		for (const auto& gm : lstMembers.value())
		{
			FormatSendChat(gm.client, wscSender, text, L"FF7BFF");
		}
	}

	std::vector<HINSTANCE> vDLLs;

	void UnloadStringDLLs()
	{
		for (uint i = 0; i < vDLLs.size(); i++)
			FreeLibrary(vDLLs[i]);
		vDLLs.clear();
	}

	void LoadStringDLLs()
	{
		UnloadStringDLLs();

		HINSTANCE hDLL = LoadLibraryEx("resources.dll", nullptr,
		    LOAD_LIBRARY_AS_DATAFILE); // typically resources.dll
		if (hDLL)
			vDLLs.push_back(hDLL);

		INI_Reader ini;
		if (ini.open("freelancer.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Resources"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("DLL"))
						{
							hDLL = LoadLibraryEx(ini.get_value_string(0), nullptr, LOAD_LIBRARY_AS_DATAFILE);
							if (hDLL)
								vDLLs.push_back(hDLL);
						}
					}
				}
			}
			ini.close();
		}
	}

	std::wstring GetWStringFromIdS(uint iIdS)
	{
		if (wchar_t wszBuf[1024]; LoadStringW(vDLLs[iIdS >> 16], iIdS & 0xFFFF, wszBuf, 1024))
			return wszBuf;
		return L"";
	}

	std::wstring FormatMsg(MessageColor color, MessageFormat format, const std::wstring& msg)
	{
		const uint bgrColor = Math::RgbToBgr(static_cast<uint>(color));
		const std::wstring tra = Math::UintToHexString(bgrColor, 6, true) + Math::UintToHexString(static_cast<uint>(format), 2);

		return L"<TRA data=\"" + tra + L"\" mask=\"-1\"/><TEXT>" + XMLText(msg) + L"</TEXT>";
	}
} // namespace Hk::Message