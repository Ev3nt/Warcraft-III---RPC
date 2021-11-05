#include <windows.h>
#include <string>
#include <time.h>
#include <discord_rpc.h>
#pragma comment(lib, "discord-rpc.lib")

#define MakePtr(a, b) (DWORD)((DWORD)a + (DWORD)b)

enum Mode
{
    NONE,
    INMENU,
    INGAME
};

HMODULE gameBase = GetModuleHandle("game.dll");

LPCSTR APPLICATION_ID = "905895401415114752";
Mode mode = NONE;

void MainThread();

//-----------------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, UINT ul_reason_for_call, LPVOID lpReserve)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (gameBase)
        {
            DisableThreadLibraryCalls(hModule);
            
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, NULL, NULL, NULL);
        }
        else
        {
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
    // if (SendPresence) {
        // DiscordRichPresence discordPresence;
        // ZeroMemory(&discordPresence, sizeof(discordPresence));
        // discordPresence.state = "(2)BootyBay.w3m"; // Location 00AAE7C0
        // discordPresence.details = "Skirmish (Lobby)"; // Detail
        // discordPresence.startTimestamp = time(0);
        // discordPresence.largeImageKey = "ingame";
        // discordPresence.partyId = "party-1";
        // discordPresence.partySize = 1; // Game.dll+AB4E08
        // discordPresence.partyMax = 6; // ["Game.dll"+00AB65F4] + 44

        Discord_UpdatePresence(&discordPresence);
        ZeroMemory(&discordPresence, sizeof(discordPresence));

        // SendPresence = 0;
    // }
    //  else {
    //      Discord_ClearPresence();
    // }
}

//-----------------------------------------------------------------------------------

void MainThread()
{
	DiscordEventHandlers handlers;
    ZeroMemory(&handlers, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	handlers.joinGame = handleDiscordJoin;
	handlers.spectateGame = handleDiscordSpectate;
	handlers.joinRequest = handleDiscordJoinRequest;

	Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);

    std::string state;
    int* party_size = (int*)MakePtr(gameBase, 0xab4e08);

    DiscordRichPresence discordPresence;
    ZeroMemory(&discordPresence, sizeof(discordPresence));

    while (true)
    {
        switch (mode)
        {
        case NONE:
            discordPresence.details = "In menu";
            discordPresence.largeImageKey = "warcraft_icon";

            updateDiscordPresence(discordPresence);

            mode = INMENU;

            break;
        case INMENU:
            if (*party_size && *(DWORD*)MakePtr(gameBase, 0xab65f4))
            {
                Sleep(1000);

                state = (LPSTR)MakePtr(gameBase, 0xaae7c0);
                
                size_t begin = state.size();
                for (; begin > 0 && state[begin - 1] != '\\'; begin--);
                state = state.substr(begin, state.size() - begin - 4);

                discordPresence.details = "In battle";
                discordPresence.largeImageKey = "warcraft_icon";
                discordPresence.smallImageKey = "battle_icon";
                discordPresence.state = state.c_str();
                discordPresence.partySize = *party_size;
                discordPresence.partyMax =  *(int*)(*(DWORD*)MakePtr(gameBase, 0xab65f4) + 0x44);
                discordPresence.startTimestamp = time(0);

                updateDiscordPresence(discordPresence);

                mode = INGAME;
            }

            break;
        case INGAME:
            if (!*party_size)
            {
                discordPresence.details = "In menu";
                discordPresence.largeImageKey = "warcraft_icon";

                updateDiscordPresence(discordPresence);

                mode = INMENU;
            }

            break;
        default:
            break;
        }

#ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
#endif
        Discord_RunCallbacks();
    }
}