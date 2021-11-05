#include <windows.h>
#include <string>
#include <time.h>
#include "discord_rpc/discord_rpc.h"

#define MakePtr(a, b) (DWORD)((DWORD)a + (DWORD)b)

typedef BOOL(__fastcall* _jassRun)(DWORD a0);
_jassRun jassRun;
DWORD jassRunCallOffset = 0x3b0a90;

HMODULE gameBase = GetModuleHandle("game.dll");

LPCSTR APPLICATION_ID = "905895401415114752";

BOOL __fastcall jassRun_Detour(DWORD a0);

void call(LPVOID address, LPVOID function);

void MainThread();

enum MODE {
    INMENU,
    INGAME
};

MODE mode = INMENU;
BOOL sendPresence = 1;

//-----------------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, UINT ul_reason_for_call, LPVOID lpReserve)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (gameBase)  {
            DisableThreadLibraryCalls(hModule);
            
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, NULL, NULL, NULL);
        }
        else {
            return FALSE;
        }

        break;
    case DLL_PROCESS_DETACH:
        Discord_ClearPresence();
        Discord_Shutdown();
        FreeLibraryAndExitThread(hModule, NULL);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------------

void handleDiscordReady(const DiscordUser* connectedUser)
{
	
}

void handleDiscordDisconnected(BOOL errcode, LPCSTR message)
{

}

void handleDiscordError(BOOL errcode, LPCSTR message)
{

}

void handleDiscordJoin(LPCSTR secret)
{
    
}

void handleDiscordSpectate(LPCSTR secret)
{
    
}

void handleDiscordJoinRequest(const DiscordUser* request)
{
    Discord_Respond(request->userId, DISCORD_REPLY_IGNORE);
}

void updateDiscordPresence(DiscordRichPresence& discordPresence)
{
    Discord_UpdatePresence(&discordPresence);
    ZeroMemory(&discordPresence, sizeof(discordPresence));
}

BOOL __fastcall jassRun_Detour(DWORD a0)
{
    mode = INGAME;
    sendPresence = 1;

    return jassRun(a0);
}

//-----------------------------------------------------------------------------------

void MainThread()
{
    jassRun = (_jassRun)(*(DWORD*)MakePtr(gameBase, jassRunCallOffset + 1) + MakePtr(gameBase, jassRunCallOffset + 5));
    call((LPVOID)MakePtr(gameBase, jassRunCallOffset), jassRun_Detour);

    DiscordRichPresence discordPresence;
    ZeroMemory(&discordPresence, sizeof(discordPresence));

    DiscordEventHandlers handlers;
    ZeroMemory(&handlers, sizeof(handlers));
    handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	handlers.joinGame = handleDiscordJoin;
	handlers.spectateGame = handleDiscordSpectate;
	handlers.joinRequest = handleDiscordJoinRequest;

    int* party_size = (int*)MakePtr(gameBase, 0xab4e08);

	Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);

    while (true) {
        if (!*party_size && mode != INMENU) {
            mode = INMENU;
            sendPresence = 1;
        }

        switch (mode)
        {
        case INMENU:
            if (sendPresence) {
                sendPresence = 0;

                discordPresence.details = "In menu";
                discordPresence.largeImageKey = "warcraft_icon";

                updateDiscordPresence(discordPresence);
            }

            break;
        case INGAME:
            if (sendPresence) {
                sendPresence = 0;

                std::string state = (LPSTR)MakePtr(gameBase, 0xaae7c0);

                size_t begin = state.size();
                for (; begin > 0 && state[begin - 1] != '\\'; begin--);
                state = state.substr(begin, state.size() - begin - 4);

                discordPresence.details = "In battle";
                discordPresence.largeImageKey = "warcraft_icon";
                discordPresence.smallImageKey = "battle_icon";
                discordPresence.state = state.c_str();
                discordPresence.partySize = *party_size;
                discordPresence.partyMax = *(int*)(*(DWORD*)MakePtr(gameBase, 0xab65f4) + 0x44);
                discordPresence.startTimestamp = time(0);

                updateDiscordPresence(discordPresence);
            }

            break;
        }

#ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
#endif
        Discord_RunCallbacks();
    }
}

//-----------------------------------------------------------------------------------

void call(LPVOID address, LPVOID function)
{
    DWORD oldProtect;
    VirtualProtect(address, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(BYTE*)address = 0xe8;
    *(DWORD*)((DWORD)address + 1) = (DWORD)function - ((DWORD)address + 5);
    VirtualProtect(address, 5, oldProtect, NULL);
}