#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "wintrust")

#define PORT 27500
#define IP "188.246.93.212"

#include <iostream>
#include <windows.h>
#include <WinSock2.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <tlhelp32.h>
#include <vector>
#include <psapi.h>
#include <fstream>
#include <sstream>
#include <ws2tcpip.h>
#include "AntiCheat.h"
#include <wintrust.h>
#include <wincrypt.h>
#include <SoftPub.h>