#pragma once
#include "includes.h"

class AC
{
private:
public:
	static void process_scanner();
	static void debugger_scanner();
	static ULONG calculate_crc(const BYTE* data, size_t length);
	static void game_check();
	static void injection_scanner();
	static void socket_setup();
};