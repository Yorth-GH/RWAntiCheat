#include "includes.h"

extern "C" __declspec(dllexport) void start()
{
	//TODO: implement features

	uint64_t* heartbeat_timer;

	std::this_thread::sleep_for(std::chrono::seconds(60)); // let the game load files etc.

	while (true) // as long as the anticheat and game are open
	{

		(*heartbeat_timer)++;
	}
}
