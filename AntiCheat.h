#pragma once
#include "includes.h"

class AC
{
private:
	std::vector<std::wstring> forbidden_processes; //TODO: add a list of processes
public:
	void process_scanner();
	void debugger_scanner();
};