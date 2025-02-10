#pragma once
#include "includes.h"

class AC
{
public:
	static void process_scanner();
	static void debugger_scanner();
	static ULONG calculate_crc(const BYTE* data, size_t length);
	static void game_check();
	static void injection_scanner();
	static void socket_setup();
	static void receive_processes();
	static void receive_modules();
	static void send_to_server(std::string str);
	static void close_socket();
	static bool verify_module(std::string module);
	static void dump_module(HMODULE module, std::string path);
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
*/