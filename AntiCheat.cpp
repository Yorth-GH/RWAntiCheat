#include "AntiCheat.h"

std::vector<std::string> forbidden_processes;
std::vector<std::string> allowed_modules;
std::vector<std::string> loaded_modules;

WSADATA wsa;
SOCKET client_socket;
sockaddr_in server_addr;
char buffer[64];

void AC::close_socket()
{
    send_to_server("6");
    closesocket(client_socket);
    WSACleanup();
}
void AC::send_to_server(std::string str)
{
    send(client_socket, str.c_str(), 64, 0);
}
void AC::receive_processes()
{
    while (true) {
        int bytesReceived = recv(client_socket, buffer, 64, 0);
        if (bytesReceived <= 0)
            break;

        buffer[bytesReceived] = '\0';
        std::string process = buffer;
        forbidden_processes.push_back(process);
    }
}
void AC::receive_modules()
{
    while (true) {
        int bytesReceived = recv(client_socket, buffer, 64, 0);
        if (bytesReceived <= 0)
            break;

        buffer[bytesReceived] = '\0';
        std::string modules = buffer;
        allowed_modules.push_back(modules);
    }
}
void AC::socket_setup()
{
    if (!WSAStartup(MAKEWORD(2, 2), &wsa))
    {
        closesocket(client_socket);
        WSACleanup();
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    InetPton(AF_INET, IP, &server_addr.sin_addr);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        closesocket(client_socket);
        WSACleanup();
    }

    send_to_server("0");
}

void AC::process_scanner()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry))
        do {
            for (std::string s : forbidden_processes)
                if (process_entry.szExeFile == s)
                {
                    std::string finals = "3" + s;
                    send_to_server(finals);
                    ExitProcess(1);
                }
        } while (Process32Next(snapshot, &process_entry));
    CloseHandle(snapshot);
}
void AC::debugger_scanner()
{
    //remote debugger
    BOOL remote_debugger_present = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote_debugger_present);
    //direct debugger
    if (IsDebuggerPresent() || remote_debugger_present)
    {
        send_to_server("2");
        ExitProcess(1);
    }
}
void AC::injection_scanner()
{
    DWORD process_id = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return;

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(hSnapshot, &me32)) {
        do {
            for (std::string s : allowed_modules)
                if (count(allowed_modules.begin(), allowed_modules.end(), s) == 0 && count(loaded_modules.begin(), loaded_modules.end(), s) == 0)
                {
                    std::string finals = "4" + s;
                    send_to_server(finals);
                    loaded_modules.push_back(s);

                    HMODULE h_module = GetModuleHandle(s.c_str());
                    std::string dumpstring = s + ".dmp";
                    dump_module(h_module, dumpstring);
                    if (!verify_module(dumpstring))
                        send_to_server("Unverified");

                    //Not closing, only sending information about the modules!
                    //ExitProcess(1);
                }
        } while (Module32Next(hSnapshot, &me32));
    }
    CloseHandle(hSnapshot);
}
void AC::game_check()
{
    std::ifstream WarRock("WarRock.exe", std::ios::binary);
    std::vector<BYTE> WarRock_bytes((std::istreambuf_iterator<char>(WarRock)), std::istreambuf_iterator<char>());

    // SHOULD IDEALLY BE ALREADY SAVED SO YOU DONT HAVE TO RECALCULATE IT
    // BUT IS RAN ONLY ONCE SO IT DOESNT AFFECT PERFORMANCE
    ULONG WarRock_CRC = calculate_crc(WarRock_bytes.data(), WarRock_bytes.size());

    std::ofstream out("out.txt");
    out << WarRock_CRC;
    out.close();
    
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
    {
        std::string finals = "1" + exe_path;
        send_to_server(finals);
        ExitProcess(1);
    }
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

// for receivig vectors
std::vector<std::string> deserialize_vector(const std::string& data) 
{
    std::vector<std::string> vec;
    std::istringstream iss(data);
    std::string line;

    while (std::getline(iss, line))
        vec.push_back(line);

    return vec;
}

bool AC::verify_module(std::string path)
{
    std::wstring wide_path(path.begin(), path.end());
    WINTRUST_FILE_INFO file_info = { sizeof(WINTRUST_FILE_INFO), wide_path.c_str(), NULL };
    WINTRUST_DATA data = { 0 };

    data.cbStruct = sizeof(WINTRUST_DATA);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &file_info;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    data.dwProvFlags = WTD_REVOCATION_CHECK_NONE;

    GUID action_id = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG status = WinVerifyTrust(NULL, &action_id, &data);

    data.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &action_id, &data);

    return status == ERROR_SUCCESS;
}
void AC::dump_module(HMODULE module, std::string path)
{
    MODULEINFO info = { 0 };
    GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info));

    if (info.lpBaseOfDll == nullptr || info.SizeOfImage == 0)
        return;
    // there is a chance its manually mapped, will find out how to detect and dump

    std::ofstream dumped(path, std::ios::binary);
    dumped.write(reinterpret_cast<const char*>(info.lpBaseOfDll), info.SizeOfImage);
    dumped.close();
}