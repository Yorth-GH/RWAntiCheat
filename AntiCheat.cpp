#include "AntiCheat.h"

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
    send(client_socket, str.c_str(), strlen(str.c_str()), 0);
}
bool AC::socket_setup()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cout << "This works" << std::endl;
        WSACleanup();
        return false;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    InetPton(AF_INET, IP, &server_addr.sin_addr);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        closesocket(client_socket);
        WSACleanup();
        return false;
    }

    send_to_server("0");
    return true;
}

void AC::process_scanner()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry))
        do {
                //if (process_entry.szExeFile == s)
                {
                    std::string a = process_entry.szExeFile;
                    std::string finals = "3" + a;
                    send_to_server(finals);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    //ExitProcess(1);
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
        //ExitProcess(1);
    }
}
void AC::injection_scanner()
{
    DWORD process_id = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return;

    char processPath[MAX_PATH];
    GetModuleFileNameA(nullptr, processPath, MAX_PATH);

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(hSnapshot, &me32)) {
        do {
                if (count(loaded_modules.begin(), loaded_modules.end(), me32.szModule) == 0)
                {
                    std::string a = me32.szModule;
                    std::string finals = "4" + a;
                   
                    loaded_modules.push_back(me32.szModule);

                    HMODULE h_module = GetModuleHandle(me32.szModule);
                    std::string dumpstring = a;
                    
                    std::string directory(processPath);
                    size_t pos = directory.find_last_of("\\/");
                    if (pos != std::string::npos)
                        directory = directory.substr(0, pos + 1); // Keep the trailing slash

                    // Construct full path
                    std::string fullPath = directory + dumpstring;


                    dump_module(h_module, fullPath);
                    if (!verify_module(h_module))
                        finals[0] = '7';
                       send_to_server(finals);
                       std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    // SHOULD BE SAVED BECAUSE WE CAN LAUNCH FAKE WARROCK
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

bool AC::verify_module(HMODULE moduleBase)
{
    if (!moduleBase)
        return false;

    // Get the DOS header
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(moduleBase);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return false; // Not a valid PE file

    // Get the NT headers
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(
        reinterpret_cast<BYTE*>(moduleBase) + dosHeader->e_lfanew
        );
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return false; // Not a valid PE file

    // Check the IMAGE_DIRECTORY_ENTRY_SECURITY
    IMAGE_DATA_DIRECTORY securityDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];

    // If the security directory is present and non-zero, the file is signed
    return (securityDir.VirtualAddress != 0 && securityDir.Size != 0);
}

void AC::dump_module(HMODULE module, std::string path)
{
    MODULEINFO info = { 0 };
    GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info));

    if (info.lpBaseOfDll == nullptr || info.SizeOfImage == 0)
        return;
    // there is a chance its manually mapped, will find out how to detect and dump

    std::ofstream dumped(path, std::ios::binary);
    if (!dumped.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return;
    }
    dumped.write(reinterpret_cast<const char*>(info.lpBaseOfDll), info.SizeOfImage);
    dumped.flush();
    dumped.close();
}