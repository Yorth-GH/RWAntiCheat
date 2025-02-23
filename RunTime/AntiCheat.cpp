#include "../includes.h"

std::vector<std::string> loaded_modules;

bool AC::check_address_in_module(HMODULE module, FARPROC address) 
{
    MODULEINFO info = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info))) 
        return false;

    BYTE* base = (BYTE*)info.lpBaseOfDll;
    return (BYTE*)address >= base && (BYTE*)address < (base + info.SizeOfImage);
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
 
bool AC::SendModule(socketClient* con, std::string filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::ostringstream hexStream;
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    for (unsigned char byte : buffer) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    auto packet = new packetBuilder(11002); // placeholder opcodes we still need to decide on real ones lol  
    packet->AddString(filePath);
    packet->AddString(hexStream.str());
    // con->Send(packet->Build());
    packet->Send(con->connection, 0x00);

    return true;
}

void AC::update(socketClient* connection)
{
    //  process_scanner(connection);
    //  debugger_scanner(connection);
    injection_scanner(connection);

}

void AC::process_scanner(socketClient* connection)
{
    DWORD gameID = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry))
        do {
                if (process_entry.th32ProcessID != gameID)
                { 
                    SendReport(connection, 1 /*gamecheck*/, process_entry.szExeFile);
                    //ExitProcess(1);

                    // Testing for open handles
                    HANDLE hP = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_entry.th32ProcessID);
                    if (hP)
                    {
                        HANDLE hT = OpenProcess(PROCESS_VM_READ, FALSE, process_entry.th32ProcessID);
                        if (hT)
                        {
                            SendReport(connection, 8, process_entry.szExeFile);
                            CloseHandle(hT);
                        }
                        CloseHandle(hP);
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
        SendReport(connection, 2 /*debugger*/);
        //ExitProcess(1);
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

                            //SendModule(connection, directory + me32.szModule);
                            SendFileToServer("127.0.0.1", 1338, me32.szModule, dumpedData);
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
        DWORD pID;
        GetWindowThreadProcessId(wnd, &pID);
        if (pID != GetCurrentProcessId())
        {
            if (GetWindowLong(wnd, GWL_EXSTYLE) & WS_EX_LAYERED && GetWindowLong(wnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT)
            {
                char title[256];
                GetWindowText(wnd, title, 256);
                SendReport(connection, 9, title);
            }
        }
    }
}

void AC::game_check(socketClient *connection)
{
    std::ifstream WarRock("WarRock.exe", std::ios::binary);
    std::vector<BYTE> WarRock_bytes((std::istreambuf_iterator<char>(WarRock)), std::istreambuf_iterator<char>());

    //// SHOULD BE SAVED BECAUSE WE CAN LAUNCH FAKE WARROCK
    ULONG WarRock_CRC = calculate_crc(WarRock_bytes.data(), WarRock_bytes.size());
    //ULONG WarRock_CRC = VALUE???
    //
    //std::ofstream out("out.txt");
    //out << WarRock_CRC;
    //out.close();
    
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
    if (exe_CRC != WarRock_CRC)
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

bool AC::SendFileToServer(const std::string& serverIp, int port, const std::string& name, const std::vector<char>& file) {
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

    // Prepare header
    uint8_t magicByte = 0xCC;
    int16_t nameLen = name.length();
    int32_t fileLen = file.size();

    std::vector<char> header(7);
    header[0] = magicByte;
    std::memcpy(&header[1], &nameLen, sizeof(nameLen));
    std::memcpy(&header[3], &fileLen, sizeof(fileLen));

    // Send header
    if (send(sock, header.data(), header.size(), 0) == -1) {
        std::cerr << "Failed to send header\n";
        closesocket(sock);
        return false;
    }

    // Send filename
    if (send(sock, name.c_str(), name.length(), 0) == -1) {
        std::cerr << "Failed to send filename\n";
        closesocket(sock);
        return false;
    }

    // Send file content
    if (!file.empty() && send(sock, file.data(), file.size(), 0) == -1) {
        std::cerr << "Failed to send file data\n";
        closesocket(sock);
        return false;
    }

    std::cout << "File sent successfully\n";
    closesocket(sock);
    return true;
}