#include "includes.h"

void server_thread()
{
	AC::socket_setup();

	AC::receive_processes();
	AC::receive_modules();
}

void heartbeat_thread()
{
	AC::heartbeat_timer++;

	std::string heartbeat_string = "5" + AC::heartbeat_timer;
	AC::send_to_server(heartbeat_string);

	std::this_thread::sleep_for(std::chrono::seconds(15));
}

void scanner_thread()
{
	AC::process_scanner();
	AC::debugger_scanner();
	AC::injection_scanner();

	std::this_thread::sleep_for(std::chrono::seconds(60));
}

extern "C" __declspec(dllexport) void start()
{
	AC::game_check(); // check if its even running in RetroWar

	std::this_thread::sleep_for(std::chrono::seconds(10)); // let the game load files etc.
	
	std::thread server_communication(server_thread); // open socket to server
	server_communication.join(); // not continuing until its finished

	std::thread heartbeat_communication(heartbeat_thread); // heartbeat sender

	AC::close_socket();
}