#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment (lib, "wintrust")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ntdll.lib")

#define PORT 1337
#define IP "127.0.0.1"

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
#include <wintrust.h>
#include <wincrypt.h>
#include <algorithm>
#include <SoftPub.h>
#include <wininet.h>
#include <bcrypt.h>
#include <iomanip>
 
#include "seededGen.h"

#include "Networking/packetBuilder.h"
#include "Networking/packetReader.h"
#include "Networking/socketClient.h"

#include "wrClient.h"

#include "RunTime/antiCheat.h"

#include "Application/applicationCore.h" 