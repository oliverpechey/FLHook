﻿#include "Global.hpp"

/**************************************************************************************************************
// misc flserver engine function hooks
**************************************************************************************************************/

namespace IEngineHook
{

	/**************************************************************************************************************
	// ship create & destroy
	**************************************************************************************************************/

	FARPROC g_OldInitCShip;

	void __stdcall CShip__Init(CShip* ship)
	{
		CallPluginsAfter(HookedCall::IEngine__CShip__Init, ship);
	}

	__declspec(naked) void Naked__CShip__Init()
	{
		__asm {
        push ecx
        push [esp+8]
        call g_OldInitCShip
        call CShip__Init
        ret 4
		}
	}

	FARPROC g_OldDestroyCShip;

	void __stdcall CShip__Destroy(CShip* ship)
	{
		CallPluginsBefore(HookedCall::IEngine__CShip__Destroy, ship);
	}

	__declspec(naked) void Naked__CShip__Destroy()
	{
		__asm {
        push ecx
        push ecx
        call CShip__Destroy
        pop ecx
        jmp g_OldDestroyCShip
		}
	}

	/**************************************************************************************************************
	// flserver memory leak bugfix
	**************************************************************************************************************/

	int __cdecl FreeReputationVibe(int const& p1)
	{
		__asm {
        mov eax, p1
        push eax
        mov eax, [hModServer]
        add eax, 0x65C20
        call eax
        add esp, 4
		}

		return Reputation::Vibe::Free(p1);
	}

	/**************************************************************************************************************
	**************************************************************************************************************/

	void __cdecl UpdateTime(double interval)
	{
		CallPluginsBefore(HookedCall::IEngine__UpdateTime, interval);

		Timing::UpdateGlobalTime(interval);

		CallPluginsAfter(HookedCall::IEngine__UpdateTime, interval);
	}

	/**************************************************************************************************************
	**************************************************************************************************************/

	uint g_LastTicks = 0;

	static void* dummy;
	void __stdcall ElapseTime(float interval)
	{
		CallPluginsBefore(HookedCall::IEngine__ElapseTime, interval);

		dummy = &Server;
		Server.ElapseTime(interval);

		CallPluginsAfter(HookedCall::IEngine__ElapseTime, interval);

		// low server load missile jitter bug fix
		uint curLoad = GetTickCount() - g_LastTicks;
		if (curLoad < 5)
		{
			uint fakeLoad = 5 - curLoad;
			Sleep(fakeLoad);
		}
		g_LastTicks = GetTickCount();
	}

	/**************************************************************************************************************
	**************************************************************************************************************/

	int __cdecl DockCall(const uint& shipId, const uint& spaceId, int dockPortIndex, DOCK_HOST_RESPONSE response)
	{
		//	dockPortIndex == -1, response -> 2 --> Dock Denied!
		//	dockPortIndex == -1, response -> 3 --> Dock in Use
		//	dockPortIndex != -1, response -> 4 --> Dock ok, proceed (dockPortIndex starts from 0 for docking point 1)
		//	dockPortIndex == -1, response -> 5 --> now DOCK!

		CallPluginsBefore(HookedCall::IEngine__DockCall, shipId, spaceId, dockPortIndex, response);

		int retVal = 0;
		TRY_HOOK
		{
			// Print out a message when a player ship docks.
			if (FLHookConfig::c()->messages.dockingMessages && response == PROCEED_DOCK)
			{
				const auto client = Hk::Client::GetClientIdByShip(shipId);
				if (client.has_value())
				{
					std::wstring wscMsg = L"Traffic control alert: %player has requested to dock";
					wscMsg = ReplaceStr(wscMsg, L"%player", (const wchar_t*)Players.GetActiveCharacterName(client.value()));
					PrintLocalUserCmdText(client.value(), wscMsg, 15000);
				}
			}
			// Actually dock
			retVal = pub::SpaceObj::Dock(shipId, spaceId, dockPortIndex, response);
		}
		CATCH_HOOK({})

		CallPluginsAfter(HookedCall::IEngine__DockCall, shipId, spaceId, dockPortIndex, response);

		return retVal;
	}

	/**************************************************************************************************************
	**************************************************************************************************************/

	FARPROC g_OldLaunchPosition;

	bool __stdcall LaunchPosition(uint spaceId, struct CEqObj& obj, Vector& position, Matrix& orientation, int dock)
	{
		auto [retVal, skip] =
		    CallPluginsBefore<bool>(HookedCall::IEngine__LaunchPosition, spaceId, obj, position, orientation, dock);
		if (skip)
			return retVal;

		return obj.launch_pos(position, orientation, dock);
	}

	__declspec(naked) void Naked__LaunchPosition()
	{
		__asm { 
        push ecx // 4
        push [esp+8+8] // 8
        push [esp+12+4] // 12
        push [esp+16+0] // 16
        push ecx
        push [ecx+176]
        call LaunchPosition	
        pop ecx
        ret 0x0C
		}
	}

	/**************************************************************************************************************
	**************************************************************************************************************/

	struct LOAD_REP_DATA
	{
		uint iRepId;
		float fAttitude;
	};

	struct REP_DATA_LIST
	{
		uint iDunno;
		LOAD_REP_DATA* begin;
		LOAD_REP_DATA* end;
	};

	bool __stdcall LoadReputationFromCharacterFile(REP_DATA_LIST* savedReps, LOAD_REP_DATA* repToSave)
	{
		// check of the rep id is valid
		if (repToSave->iRepId == 0xFFFFFFFF)
			return false; // rep id not valid!

		LOAD_REP_DATA* repIt = savedReps->begin;

		while (repIt != savedReps->end)
		{
			if (repIt->iRepId == repToSave->iRepId)
				return false; // we already saved this rep!

			repIt++;
		}

		// everything seems fine, add
		return true;
	}

	FARPROC g_OldLoadReputationFromCharacterFile;

	__declspec(naked) void Naked__LoadReputationFromCharacterFile()
	{
		__asm {
        push ecx // save ecx because thiscall
        push [esp+4+4+8] // rep data
        push ecx // rep data list
        call LoadReputationFromCharacterFile
        pop ecx // recover ecx
        test al, al
        jz abort_lbl
        jmp [g_OldLoadReputationFromCharacterFile]
abort_lbl:
        ret 0x0C
		}
	}

	/**************************************************************************************************************
	**************************************************************************************************************/

} // namespace IEngine
