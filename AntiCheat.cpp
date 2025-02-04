#include "AntiCheat.h"

void AC::process_scanner()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry))
    {
        do
        {
            std::wstring process_name = process_entry.szExeFile;
            for (std::wstring ws : AC::forbidden_processes)
                if (process_name == ws)
                    ExitProcess(1);
        } while (Process32Next(snapshot, &process_entry));
    }
    CloseHandle(snapshot);
}

void AC::debugger_scanner()
{
    if (IsDebuggerPresent())
    {
        ExitProcess(1);
    }
}