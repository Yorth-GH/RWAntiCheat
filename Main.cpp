#include "includes.h"

uint32_t heartbeat_timer = 0;

void heartbeat_thread()
{
	heartbeat_timer++;

	std::string heartbeat_string = "5" + heartbeat_timer;
	AC::send_to_server(heartbeat_string);

    std::this_thread::sleep_for(std::chrono::milliseconds(15000));
}

void others_thread()
{
    AC::process_scanner();
    AC::debugger_scanner();
    AC::injection_scanner();

    std::this_thread::sleep_for(std::chrono::milliseconds(60000));
}

extern "C" __declspec(dllexport) void start()
{
    //AC::game_check();

    AC::socket_setup();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!AC::socket_setup())
        ExitProcess(1);

    while (true)
    {
        std::thread hb(heartbeat_thread);
        hb.detach();

       // std::thread ot(others_thread);
       // ot.detach();

        std::this_thread::sleep_for(std::chrono::milliseconds(15000));
    }
}

unsigned long WINAPI initialize(void* instance)
{
    std::thread s(start);
    s.detach();

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(60000));
}

BOOL WINAPI DllMain(
    void* hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    DisableThreadLibraryCalls(static_cast<HMODULE>(hinstDLL));  

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (auto handle = CreateThread(NULL, NULL, initialize, hinstDLL, NULL, NULL))
            CloseHandle(handle);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != nullptr)
        {
            break;
        }

        break;
    }
    return TRUE;
}