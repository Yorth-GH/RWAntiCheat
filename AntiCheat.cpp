#include "AntiCheat.h"

static std::vector<std::wstring> forbidden_processes;

void AC::socket_setup()
{

}

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
            for (std::wstring ws : forbidden_processes)
                if (process_name == ws)
                    ExitProcess(1);
        } while (Process32Next(snapshot, &process_entry));
    }
    CloseHandle(snapshot);
}

void AC::debugger_scanner()
{
    //remote debugger
    BOOL remote_debugger_present = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote_debugger_present);
    //direct debugger
    if (IsDebuggerPresent() || remote_debugger_present)
        ExitProcess(1);
    
}

ULONG AC::calculate_crc(const BYTE* data, size_t length) {
    DWORD crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
    }
    return ~crc;
}
void AC::game_check()
{
    std::ifstream WarRock("WarRock.exe", std::ios::binary);
    std::vector<BYTE> WarRock_bytes((std::istreambuf_iterator<char>(WarRock)), std::istreambuf_iterator<char>());

    // SHOULD IDEALLY BE ALREADY SAVED SO YOU DONT HAVE TO RECALCULATE IT
    // BUT IS RAN ONLY ONCE SO IT DOESNT AFFECT PERFORMANCE
    ULONG WarRock_CRC = calculate_crc(WarRock_bytes.data(), WarRock_bytes.size());
    
    std::string exe_path;
    DWORD parent_process = GetCurrentProcessId();
    char path[MAX_PATH] = { 0 };
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parent_process);

    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameA(handle, 0, path, &size))
    {
        CloseHandle(handle);
        exe_path = std::string(path);
    }
    CloseHandle(handle);

    std::ifstream exe(exe_path, std::ios::binary);
    std::vector<BYTE> exe_bytes((std::istreambuf_iterator<char>(exe)), std::istreambuf_iterator<char>());

    ULONG exe_CRC = calculate_crc(exe_bytes.data(), exe_bytes.size());
    if (exe_CRC != WarRock_CRC || !WarRock.is_open())
        ExitProcess(1);
}

void AC::injection_scanner()
{

}