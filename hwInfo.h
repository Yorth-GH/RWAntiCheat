#pragma once

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#pragma comment(lib, "wbemuuid.lib")

typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);

std::string GetOSVersion() {
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr rtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (rtlGetVersion) {
            RTL_OSVERSIONINFOEXW osInfo = { 0 };
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            if (rtlGetVersion(&osInfo) == 0) { // STATUS_SUCCESS
                return std::to_string(osInfo.dwMajorVersion) + "." +
                    std::to_string(osInfo.dwMinorVersion) + "." +
                    std::to_string(osInfo.dwBuildNumber);
            }
        }
    }
    return "Unknown";
}

// ----- Component Classes -----
class GPU {
public:
    std::string name;
    int vram; // in MB
    std::string identifier;

    std::string toJSON() const {
        std::ostringstream ss;
        ss << "{ \"name\": \"" << name << "\", \"vram\": " << vram
            << ", \"identifier\": \"" << identifier << "\" }";
        return ss.str();
    }
};

class Display {
public:
    std::string name;
    std::string resolution; // e.g., "1920x1080"
    std::string identifier;

    std::string toJSON() const {
        std::ostringstream ss;
        ss << "{ \"name\": \"" << name << "\", \"resolution\": \"" << resolution
            << "\", \"identifier\": \"" << identifier << "\" }";
        return ss.str();
    }
};

class NetworkAdapter {
public:
    std::string name;
    std::string ip;
    std::string gateway;
    std::string mac;
    std::string identifier;

    std::string toJSON() const {
        std::ostringstream ss;
        ss << "{ \"name\": \"" << name << "\", \"ip\": \"" << ip
            << "\", \"gateway\": \"" << gateway
            << "\", \"mac\": \"" << mac
            << "\", \"identifier\": \"" << identifier << "\" }";
        return ss.str();
    }
};

// ----- HardwareInfo Class -----
class HardwareInfo {
public:
    std::string machineName;
    std::string userName;
    bool isAdministrator;
    std::string windowsVersion;
    std::string screenDimensions;
    // CPU details
    std::string cpuName;
    int cpuCores;
    int cpuClockSpeed;
    std::string cpuIdentifier;
    // RAM details
    std::string ramName;
    int ramAmount;       // in MB
    std::string ramVersion;
    int ramSpeed;        // in MHz
    std::string ramIdentifier;
    std::vector<GPU> gpus;
    std::vector<Display> displays;
    std::vector<NetworkAdapter> networkAdapters;
    std::string motherboardName;
    std::string motherboardIdentifier;

    std::string toJSON() {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"machineName\": \"" << machineName << "\",\n";
        ss << "  \"userName\": \"" << userName << "\",\n";
        ss << "  \"isAdministrator\": " << (isAdministrator ? "true" : "false") << ",\n";
        ss << "  \"windowsVersion\": \"" << windowsVersion << "\",\n";
        ss << "  \"screenDimensions\": \"" << screenDimensions << "\",\n";
        ss << "  \"cpu\": {\n";
        ss << "    \"name\": \"" << cpuName << "\",\n";
        ss << "    \"cores\": " << cpuCores << ",\n";
        ss << "    \"clockSpeed\": " << cpuClockSpeed << ",\n";
        ss << "    \"identifier\": \"" << cpuIdentifier << "\"\n";
        ss << "  },\n";
        ss << "  \"ram\": {\n";
        ss << "    \"name\": \"" << ramName << "\",\n";
        ss << "    \"amount\": " << ramAmount << ",\n";
        ss << "    \"version\": \"" << ramVersion << "\",\n";
        ss << "    \"speed\": " << ramSpeed << ",\n";
        ss << "    \"identifier\": \"" << ramIdentifier << "\"\n";
        ss << "  },\n";
        ss << "  \"gpus\": [";
        for (size_t i = 0; i < gpus.size(); i++) {
            ss << gpus[i].toJSON();
            if (i < gpus.size() - 1)
                ss << ", ";
        }
        ss << "],\n";
        ss << "  \"displays\": [";
        for (size_t i = 0; i < displays.size(); i++) {
            ss << displays[i].toJSON();
            if (i < displays.size() - 1)
                ss << ", ";
        }
        ss << "],\n";
        ss << "  \"networkAdapters\": [";
        for (size_t i = 0; i < networkAdapters.size(); i++) {
            ss << networkAdapters[i].toJSON();
            if (i < networkAdapters.size() - 1)
                ss << ", ";
        }
        ss << "],\n";
        ss << "  \"motherboard\": {\n";
        ss << "    \"name\": \"" << motherboardName << "\",\n";
        ss << "    \"identifier\": \"" << motherboardIdentifier << "\"\n";
        ss << "  }\n";
        ss << "}\n";
        return ss.str();
    }
};

HardwareInfo getHardwareInfo() {
    HardwareInfo info;
    char buffer[256];
    DWORD size = sizeof(buffer);

    // Machine Name
    if (GetComputerNameExA(ComputerNameDnsHostname, buffer, &size))
        info.machineName = buffer;
    else
        info.machineName = "Unknown";

    // User Name
    size = sizeof(buffer);
    if (GetUserNameA(buffer, &size))
        info.userName = buffer;
    else
        info.userName = "Unknown";

    // Check Admin Rights
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = NULL;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin);
        FreeSid(AdministratorsGroup);
    }
    info.isAdministrator = isAdmin;

    info.windowsVersion = GetOSVersion();

    // Screen Dimensions
    info.screenDimensions = std::to_string(GetSystemMetrics(SM_CXSCREEN)) + "x" +
        std::to_string(GetSystemMetrics(SM_CYSCREEN));

    // --------------------- WMI Setup ---------------------
    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        std::cerr << "Failed to initialize COM library. Error code = 0x"
            << std::hex << hres << std::endl;
        return info;
    }

    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );
    if (FAILED(hres)) {
        std::cerr << "Failed to initialize security. Error code = 0x"
            << std::hex << hres << std::endl;
        CoUninitialize();
        return info;
    }

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc
    );
    if (FAILED(hres)) {
        std::cerr << "Failed to create IWbemLocator object. Error code = 0x"
            << std::hex << hres << std::endl;
        CoUninitialize();
        return info;
    }

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );
    if (FAILED(hres)) {
        std::cerr << "Could not connect to WMI namespace. Error code = 0x"
            << std::hex << hres << std::endl;
        pLoc->Release();
        CoUninitialize();
        return info;
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );
    if (FAILED(hres)) {
        std::cerr << "Could not set proxy blanket. Error code = 0x"
            << std::hex << hres << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return info;
    }

    // --------------------- Motherboard Info ---------------------
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_BaseBoard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR) {
                std::wstring wManufacturer(vtProp.bstrVal);
                VariantClear(&vtProp);
                if (SUCCEEDED(pclsObj->Get(L"Product", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR) {
                    std::wstring wProduct(vtProp.bstrVal);
                    VariantClear(&vtProp);
                    info.motherboardName = std::string(wManufacturer.begin(), wManufacturer.end()) + " " +
                        std::string(wProduct.begin(), wProduct.end());
                }
            }
            if (SUCCEEDED(pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR) {
                info.motherboardIdentifier = std::string(_bstr_t(vtProp.bstrVal));
                VariantClear(&vtProp);
            }
            else {
                info.motherboardIdentifier = "Unknown";
            }
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    // --------------------- GPU Info ---------------------
    pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
            GPU gpu;
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"Name", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                gpu.name = std::string(_bstr_t(vtProp.bstrVal));
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"AdapterRAM", 0, &vtProp, 0, 0)) &&
                (vtProp.vt == VT_UI4 || vtProp.vt == VT_UINT)) {
                unsigned int ramBytes = vtProp.uintVal;
                gpu.vram = ramBytes / (1024 * 1024);
            }
            else {
                gpu.vram = 0;
            }
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                gpu.identifier = std::string(_bstr_t(vtProp.bstrVal));
            else
                gpu.identifier = "Unknown";
            VariantClear(&vtProp);
            info.gpus.push_back(gpu);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    // --------------------- Display Info ---------------------
    pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_DesktopMonitor"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
            Display disp;
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"Name", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                disp.name = std::string(_bstr_t(vtProp.bstrVal));
            else
                disp.name = "Unknown";
            VariantClear(&vtProp);

            int width = 0, height = 0;
            if (SUCCEEDED(pclsObj->Get(L"ScreenWidth", 0, &vtProp, 0, 0)) && vtProp.vt == VT_I4)
                width = vtProp.intVal;
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"ScreenHeight", 0, &vtProp, 0, 0)) && vtProp.vt == VT_I4)
                height = vtProp.intVal;
            VariantClear(&vtProp);
            if (width > 0 && height > 0)
                disp.resolution = std::to_string(width) + "x" + std::to_string(height);
            else
                disp.resolution = "Unknown";
            if (SUCCEEDED(pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                disp.identifier = std::string(_bstr_t(vtProp.bstrVal));
            else
                disp.identifier = "Unknown";
            VariantClear(&vtProp);
            info.displays.push_back(disp);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    // --------------------- Network Adapter Info ---------------------
    pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled = True"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
            NetworkAdapter net;
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"Description", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                net.name = std::string(_bstr_t(vtProp.bstrVal));
            else
                net.name = "Unknown";
            VariantClear(&vtProp);

            if (SUCCEEDED(pclsObj->Get(L"IPAddress", 0, &vtProp, 0, 0)) && (vtProp.vt & VT_ARRAY)) {
                SAFEARRAY* pSafeArray = vtProp.parray;
                if (pSafeArray) {
                    LONG lBound, uBound;
                    SafeArrayGetLBound(pSafeArray, 1, &lBound);
                    SafeArrayGetUBound(pSafeArray, 1, &uBound);
                    if (lBound <= uBound) {
                        BSTR* data;
                        SafeArrayAccessData(pSafeArray, (void**)&data);
                        net.ip = std::string(_bstr_t(data[0]));
                        SafeArrayUnaccessData(pSafeArray);
                    }
                }
            }
            VariantClear(&vtProp);

            if (SUCCEEDED(pclsObj->Get(L"DefaultIPGateway", 0, &vtProp, 0, 0)) && (vtProp.vt & VT_ARRAY)) {
                SAFEARRAY* pSafeArray = vtProp.parray;
                if (pSafeArray) {
                    LONG lBound, uBound;
                    SafeArrayGetLBound(pSafeArray, 1, &lBound);
                    SafeArrayGetUBound(pSafeArray, 1, &uBound);
                    if (lBound <= uBound) {
                        BSTR* data;
                        SafeArrayAccessData(pSafeArray, (void**)&data);
                        net.gateway = std::string(_bstr_t(data[0]));
                        SafeArrayUnaccessData(pSafeArray);
                    }
                }
            }
            VariantClear(&vtProp);

            if (SUCCEEDED(pclsObj->Get(L"MACAddress", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                net.mac = std::string(_bstr_t(vtProp.bstrVal));
            else
                net.mac = "Unknown";
            VariantClear(&vtProp);

            if (SUCCEEDED(pclsObj->Get(L"SettingID", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                net.identifier = std::string(_bstr_t(vtProp.bstrVal));
            else
                net.identifier = "Unknown";
            VariantClear(&vtProp);

            info.networkAdapters.push_back(net);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    // --------------------- CPU Info ---------------------
    pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_Processor"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"Name", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                info.cpuName = std::string(_bstr_t(vtProp.bstrVal));
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0)) && vtProp.vt == VT_I4)
                info.cpuCores = vtProp.intVal;
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0)) && vtProp.vt == VT_I4)
                info.cpuClockSpeed = vtProp.intVal;
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                info.cpuIdentifier = std::string(_bstr_t(vtProp.bstrVal));
            else
                info.cpuIdentifier = "Unknown";
            VariantClear(&vtProp);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    // --------------------- RAM Info ---------------------
    pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_PhysicalMemory"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        unsigned __int64 totalCapacityBytes = 0;
        int commonSpeed = 0;
        std::string manufacturer = "";
        std::string partNumber = "";
        std::string memTypeStr = "Unknown";
        while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
            VARIANT vtProp;
            if (SUCCEEDED(pclsObj->Get(L"Capacity", 0, &vtProp, 0, 0))) {
                if (vtProp.vt == VT_BSTR)
                    totalCapacityBytes += _wtoi64(vtProp.bstrVal);
                else if (vtProp.vt == VT_UI8)
                    totalCapacityBytes += vtProp.ullVal;
            }
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"Speed", 0, &vtProp, 0, 0)) && vtProp.vt == VT_I4)
                commonSpeed = vtProp.intVal;
            VariantClear(&vtProp);
            if (manufacturer.empty() && SUCCEEDED(pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                manufacturer = std::string(_bstr_t(vtProp.bstrVal));
            VariantClear(&vtProp);
            if (partNumber.empty() && SUCCEEDED(pclsObj->Get(L"PartNumber", 0, &vtProp, 0, 0)) && vtProp.vt == VT_BSTR)
                partNumber = std::string(_bstr_t(vtProp.bstrVal));
            VariantClear(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"MemoryType", 0, &vtProp, 0, 0)) && vtProp.vt == VT_I4) {
                int memType = vtProp.intVal;
                if (memType == 20) memTypeStr = "DDR";
                else if (memType == 21) memTypeStr = "DDR2";
                else if (memType == 24) memTypeStr = "DDR3";
                else if (memType == 26) memTypeStr = "DDR4";
                else memTypeStr = "Unknown";
            }
            VariantClear(&vtProp);
            pclsObj->Release();
        }
        pEnumerator->Release();
        info.ramAmount = static_cast<int>(totalCapacityBytes / (1024 * 1024));
        info.ramSpeed = commonSpeed;
        info.ramName = manufacturer;
        info.ramIdentifier = partNumber;
        info.ramVersion = memTypeStr;
    }

    // Cleanup WMI
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return info;
}