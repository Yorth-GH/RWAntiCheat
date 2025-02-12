#include "includes.h"

applicationCore* core = NULL;
socketClient* connection = NULL;
 
void MainLoop()
{
    while (TRUE)
    { 
        AC::update(connection);
        core->Update(connection);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
   
//Later for loading via internal dll :D
//extern "C" __declspec(dllexport) void AC_Run(HMODULE hMod, DWORD dwType)
//{
//   
//}

BOOL WINAPI DllMain(
    void* hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    DisableThreadLibraryCalls(static_cast<HMODULE>(hinstDLL));  

    if (!core)
        core = new applicationCore();

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH: {
        core->Initialize(connection); 
        std::thread s(MainLoop); s.detach();
        break;
    }

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != nullptr)
        {
            break;
        }

        core->Uninitialize();

        break;
    }
    return TRUE;
}