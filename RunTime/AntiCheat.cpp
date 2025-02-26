#include "../includes.h"

std::vector<std::string> loaded_modules;
std::vector<std::string> loaded_procs;

bool AC::check_address_in_module(HMODULE module, FARPROC address) 
{
    MODULEINFO info = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info))) 
        return false;

    BYTE* base = (BYTE*)info.lpBaseOfDll;
    return (BYTE*)address >= base && (BYTE*)address < (base + info.SizeOfImage);
}

std::string AC::hash(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) 
        return "";

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, nullptr, 0))) 
        return "";

    DWORD hashObjectSize = 0, dataSize = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &dataSize, 0);
    std::vector<BYTE> hashObject(hashObjectSize);

    BCRYPT_HASH_HANDLE hHash = nullptr;
    if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0))) 
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    const size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);
    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) 
    {
        if (!BCRYPT_SUCCESS(BCryptHashData(hHash, reinterpret_cast<PUCHAR>(buffer.data()), (ULONG)file.gcount(), 0))) {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return "";
        }
    }

    BYTE hash[20]; // SHA-1 = 20 bytes
    BCryptFinishHash(hHash, hash, sizeof(hash), 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    std::ostringstream oss;
    for (BYTE byte : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;

    return oss.str();
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

bool AC::system_process(std::string path)
{
    std::string path2(path);
    std::transform(path2.begin(), path2.end(), path2.begin(), ::tolower);
    return (path2.find("c:\\windows\\system32\\") == 0 ||
        path2.find("c:\\windows\\syswow64\\") == 0 ||
        path2.find("c:\\windows\\winsxs\\") == 0 ||
        path2.find("c:\\windows\\systemapps\\") == 0);
}

bool AC::system_module(HMODULE h_module) {
    char module_path[MAX_PATH];
    if (GetModuleFileNameEx(GetCurrentProcess(), h_module, module_path, MAX_PATH)) {
        std::string path(module_path);
        std::transform(path.begin(), path.end(), path.begin(), ::tolower);

        return (path.find("c:\\windows\\system32\\") == 0 ||
            path.find("c:\\windows\\syswow64\\") == 0 ||
            path.find("c:\\windows\\winsxs\\") == 0 ||
            path.find("c:\\windows\\systemapps\\") == 0);
    }
    return false;
}
 
void AC::update(socketClient* connection)
{
    //process_scanner(connection);
    //debugger_scanner(connection);
    //injection_scanner(connection);
    game_check(connection);
    //overlay_scanner(connection);
    //iat_scanner(connection);
}

void AC::process_scanner(socketClient* connection)
{
    DWORD gameID = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry))
        do {
            if (count(loaded_procs.begin(), loaded_procs.end(), process_entry.szExeFile) == 0)
            {
                loaded_procs.push_back(process_entry.szExeFile);

                if (process_entry.th32ProcessID != gameID)
                {
                    DWORD session_id = 0;
                    ProcessIdToSessionId(process_entry.th32ProcessID, &session_id);
                    if (session_id != 0)
                    {
                        std::string path = "";
                        char buff[MAX_PATH];
                        DWORD size = MAX_PATH;

                        HANDLE hP = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_entry.th32ProcessID);
                        if (QueryFullProcessImageNameA(hP, 0, buff, &size))
                            path = buff;
                        CloseHandle(hP);
                        if (system_process(path))
                            continue;
                        SendReport(connection, 3, process_entry.szExeFile);
                        SendFileToServer("127.0.0.1", 1338, path);
                    }
                    else
                        continue;
                }
            }
        } while (Process32Next(snapshot, &process_entry));
    CloseHandle(snapshot);
}

void AC::debugger_scanner(socketClient* connection)
{
    //remote debugger
    BOOL remote_debugger_present = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote_debugger_present);
    //direct debugger
    if (IsDebuggerPresent() || remote_debugger_present)
    {
        SendReport(connection, 2/*debugger*/);
        ExitProcess(1);
    }
}

void AC::injection_scanner(socketClient* connection)
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
                    
                    loaded_modules.push_back(me32.szModule);

                    HMODULE h_module = GetModuleHandle(me32.szModule); 
                    if (h_module == GetModuleHandle("asd.asi.dll") || h_module == GetModuleHandle(NULL))
                        continue;
                    
                    if (!system_module(h_module))
                    {
                        std::string directory(processPath);
                        size_t pos = directory.find_last_of("\\/");
                        if (pos != std::string::npos)
                            directory = directory.substr(0, pos + 1); // Keep the trailing slash 

                        SendReport(connection, (verify_module(h_module) == true ? 4 /*module*/ : 7 /*unknown module*/), me32.szModule);

                        if (!verify_module(h_module))
                        { 
                            MODULEINFO info = { 0 };
                            GetModuleInformation(GetCurrentProcess(), h_module, &info, sizeof(info));

                            if (info.lpBaseOfDll == nullptr || info.SizeOfImage == 0)
                                return;

                            std::vector<char> dumpedData(
                                reinterpret_cast<const char*>(info.lpBaseOfDll),
                                reinterpret_cast<const char*>(info.lpBaseOfDll) + info.SizeOfImage
                            );

                            char path[MAX_PATH];
                            GetModuleFileName(h_module, path, MAX_PATH);
                            SendFileToServer("127.0.0.1", 1338, path);
                        }                
                    }                    
                }
        } while (Module32Next(hSnapshot, &me32));
    }
    CloseHandle(hSnapshot);
}

void AC::overlay_scanner(socketClient* connection)
{
    HWND wnd = nullptr;
    while ((wnd = FindWindowEx(nullptr, wnd, nullptr, nullptr)) != nullptr)
    {
        if (!IsWindowVisible(wnd))
            continue;

        DWORD pID;
        GetWindowThreadProcessId(wnd, &pID);
        if (pID != GetCurrentProcessId())
        {
            if (GetWindowLong(wnd, GWL_EXSTYLE) & WS_EX_LAYERED && GetWindowLong(wnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT)
            {
                RECT rect;
                if (!GetWindowRect(wnd, &rect) || (rect.right - rect.left) <= 10 || (rect.bottom - rect.top) <= 10)
                    continue;

                char title[256];
                GetWindowText(wnd, title, 256);
                SendReport(connection, 9, title);
            }
        }
    }
}

void AC::game_check(socketClient *connection)
{   
    std::string exe_path;
    DWORD parent_process = GetCurrentProcessId();
    char path[MAX_PATH] = { 0 };
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parent_process);

    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameA(handle, 0, path, &size))
        exe_path = std::string(path);

    CloseHandle(handle);

    std::ifstream exe(exe_path, std::ios::binary);
    std::vector<BYTE> exe_bytes((std::istreambuf_iterator<char>(exe)), std::istreambuf_iterator<char>());

    std::string exe_HASH = hash(exe_path);
    if (exe_HASH != "ed768d9050b5ed3f0382adbe9b1bb2507179d828")
    {
        SendReport(connection, 1 /*gamecheck*/, exe_path);
        ExitProcess(1);
    }
}

void AC::SendReport(socketClient* connection, int type, std::string message)
{
    auto packet = new packetBuilder(11001); // placeholder opcodes we still need to decide on real ones lol  
    packet->AddInt(type);
    if (message.length() > 0)
        packet->AddString(message);
    connection->Send(packet->Build());
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

void AC::iat_scanner(socketClient* connection) 
{
    HMODULE hmodule = GetModuleHandle(NULL);

    if (!hmodule)
        return;

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)hmodule;
    PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)((BYTE*)hmodule + dos_header->e_lfanew);

    DWORD import_dir_RVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    if (!import_dir_RVA) 
        return; 

    PIMAGE_IMPORT_DESCRIPTOR import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hmodule + import_dir_RVA);

    while (import_descriptor->Name) 
    {
        LPCSTR module_name = (LPCSTR)((BYTE*)hmodule + import_descriptor->Name);
        HMODULE imported_module = GetModuleHandleA(module_name);
        if (!imported_module) 
        {
            /*std::cout << "missing import: " << module_name << std::endl;*/
            // maybe fucking with IAT table? not 100% sure though so cant flag yet
            import_descriptor++;
            continue;
        }

        PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)hmodule + import_descriptor->FirstThunk);
        PIMAGE_THUNK_DATA orig_thunk = (PIMAGE_THUNK_DATA)((BYTE*)hmodule + import_descriptor->OriginalFirstThunk);

        while (orig_thunk->u1.Function) 
        {
            FARPROC current_address = (FARPROC)thunk->u1.Function;
            FARPROC real_process = GetProcAddress(imported_module, (LPCSTR)((BYTE*)hmodule + orig_thunk->u1.AddressOfData));

            bool in_module = check_address_in_module(imported_module, current_address);

            bool match = (current_address == real_process);

            if (!in_module || !match) 
            {
                std::string finals(module_name + std::string("...") + ((IMAGE_IMPORT_BY_NAME*)((BYTE*)hmodule + orig_thunk->u1.AddressOfData))->Name);
                SendReport(connection, 10, finals);
            }

            thunk++;
            orig_thunk++;
        }
        import_descriptor++;
    }
}

bool AC::SendFileToServer(const std::string& serverIp, int port, const std::string& name) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Error creating socket\n";
        return false;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address\n";
        closesocket(sock);
        return false;
    }
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed\n";
        closesocket(sock); 
        return false;
    }
    std::ifstream file(name, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return false;
    }

    std::string fileName = name.substr(name.find_last_of("\\/") + 1);

    file.seekg(0, std::ios::end);
    int32_t fileLen = file.tellg();
    file.seekg(0, std::ios::beg);

    uint8_t magicByte = 0xCC;
    int32_t nameLen = static_cast<int32_t>(fileName.size());

    char buffer[4096];

    // Send magicByte
    if (send(sock, reinterpret_cast<char*>(&magicByte), sizeof(magicByte), 0) == -1) {
        std::cerr << "Failed to send magicByte\n";
        closesocket(sock);
        return false;
    }

    //send name length
    if (send(sock, reinterpret_cast<char*>(&nameLen), sizeof(nameLen), 0) == -1) {
        std::cerr << "Failed to send filename length\n";
        closesocket(sock);
        return false;
    }

    // Send filename
    if (send(sock, fileName.c_str(), nameLen, 0) == -1) {
        std::cerr << "Failed to send filename\n";
        closesocket(sock);
        return false;
    }

    // Send file size
    if (send(sock, reinterpret_cast<char*>(&fileLen), sizeof(fileLen), 0) == -1) {
        std::cerr << "Failed to send file length\n";
        closesocket(sock);
        return false;
    }

    // Send file content    
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        send(sock, buffer, file.gcount(), 0);
    }
    file.close();

    std::cout << "File sent successfully\n";
    closesocket(sock);
    return true;
}