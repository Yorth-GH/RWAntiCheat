#pragma once
#include "../includes.h"

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO {
	USHORT ProcessId;
	USHORT CreatorBackTraceIndex;
	UCHAR ObjectTypeIndex;
	UCHAR HandleAttributes;
	USHORT HandleValue;
	PVOID Object;
	ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG NumberOfHandles;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

extern "C" NTSTATUS NTAPI NtQuerySystemInformation(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);

#define SystemHandleInformation 16

class socketClient;

class AC
{
public:
	static void update(socketClient* connection);

	//core modules
	static void process_scanner(socketClient* connection);
	static void debugger_scanner(socketClient* connection);
	static void game_check(socketClient* connection);
	static void injection_scanner(socketClient* connection);
	static void overlay_scanner(socketClient* connection);
	static void iat_scanner(socketClient* connection);
	static void hide_thread(HANDLE thread);
	 
	//net funcs
	static void SendReport(socketClient* connection, int type, std::string message = "");
	static bool SendFileToServer(const std::string& serverIp, int port, const std::string& name);

	//helper funcs -- TADO make helper classes
	static ULONG calculate_crc(const BYTE* data, size_t length);
	static bool system_process(std::string path);
	static bool system_module(HMODULE h_module);
	static bool verify_module(HMODULE moduleBase);
	static bool check_address_in_module(HMODULE module, FARPROC address);
	static std::string hash(std::string path);
};

/*
* Message Codes:
* 0 - Connected to server
* 1 - Loaded from within another game
* 2 - Debugger detected
* 3 - Forbidden process detected
* 4 - Unknown module detected
* 5 - Heartbeat callback
* 6 - Connection to server closing
* 7 - Unverified module
* 8 - Read access
* 9 - Potential overlay
* 10 - IAT Detection
* 11 - Process/Module Hash
*/

