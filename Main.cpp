#include "includes.h"

extern "C" __declspec(dllexport) void start()
{
	//TODO: implement features

	uint64_t* heartbeat_timer = 0;

	AC::game_check(); // check if its even running in RetroWar

	std::this_thread::sleep_for(std::chrono::seconds(5)); // let the game load files etc.
	
	AC::socket_setup();
	AC::receive_processes();
	AC::receive_modules();

	while (true) // as long as the anticheat and game are open
	{
		AC::process_scanner();
		AC::debugger_scanner();
		AC::injection_scanner();
		(*heartbeat_timer)++;

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}