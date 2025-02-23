#include "includes.h"

SOCKET wrClient::gameServerSocket = NULL;
std::string wrClient::sendBuffer = "";
std::string wrClient::recvBuffer = "";

oConnect wrClient::pConnect;
oRecv wrClient::pRecv;
oSend wrClient::pSend;

bool wrClient::isHooked = false;
bool wrClient::isOnGameServer = false;

int wrClient::userId = -1;
std::string wrClient::username = "NULL";
std::string wrClient::nickname = "NULL";

void wrClient::HandleServerPacket(packetReader* packet)
{
	if (packet == NULL) return;
	if (packet->OpCode == 25088) // server_join
	{
		if (packet->GetInt(0) == 1) 
		{
			isOnGameServer = true;

			userId = packet->GetInt(3);
			nickname = packet->GetString(5);

			auto gsHandshake = new packetBuilder(25115); // unused wr opc 
			gsHandshake->AddInt(1); // subtype 1 handshake?
			gsHandshake->AddString("+++HWID+++"); // unique identifier ?
			gsHandshake->Send(gameServerSocket, 0xC3);
			// i add this so the wr game server also knows the client has AC running
		}
	}

	if (packet->OpCode == 24576) // event_disconnect
	{ 
		ResetClient();
	}
} 

int __stdcall wrClient::Hooked_Recv(SOCKET s, CHAR* buf, int len, int flags)
{
	int length = pRecv(s, buf, len, flags); 
	if (s == gameServerSocket)
	{
		if (length > 0)
		{
			for (int i = 0; i < length; i++)
			{
				recvBuffer += (char)(buf[i] ^ 0x96);
			}

			int packetBegin = 0;
			for (int i = 0; i < recvBuffer.length(); i++)
			{
				if (recvBuffer.at(i) == '\n')
				{
					std::string packet = recvBuffer.substr(packetBegin, i - packetBegin);
 
					auto reader = new packetReader(packet);
					HandleServerPacket(reader);

					packetBegin = i + 1;
				}
			}

			if (packetBegin < recvBuffer.length())
			{
				recvBuffer = recvBuffer.substr(packetBegin, recvBuffer.length() - packetBegin);
			}
			else
			{
				recvBuffer = "";
			}
		}
	}
	return length;
}

int WINAPI wrClient::Hooked_Connect(SOCKET s, sockaddr* name, int len)
{
	SOCKADDR_IN* pSockAddr = (SOCKADDR_IN*)name; // This will manipulate name directly since it is a pointer. 
	u_short port = ntohs(pSockAddr->sin_port); // Convert the port so we can read it.

	if (port == 5340) // if the port is our game server
	{
		gameServerSocket = SOCKET(s); // snatch the socket
	}
	else if (port == 5330) // if its auth server / new connection
	{
		gameServerSocket = NULL; // make sure to prune the socket
		ResetClient(); // prune our client data aswell
	}

	return pConnect(s, name, len); // return to client
}

void wrClient::ResetClient()
{
	isOnGameServer = false;
	userId = -1;
	username = nickname = "NULL";
}


void* hook_func(BYTE* src, const BYTE* dst, const int len)
{
	BYTE* jmp = (BYTE*)malloc(len + 5);
	DWORD dwBack;

	VirtualProtect(src, len, PAGE_READWRITE, &dwBack);
	memcpy(jmp, src, len);
	jmp += len;
	jmp[0] = 0xE9;
	*(DWORD*)(jmp + 1) = (DWORD)(src + len - jmp) - 5;

	src[0] = 0xE9;
	*(DWORD*)(src + 1) = (DWORD)(dst - src) - 5;
	for (int i = 5; i < len; i++) src[i] = 0x90;
	VirtualProtect(src, len, dwBack, &dwBack);

	return(jmp - len);
}

void wrClient::HookNetwork()
{
	HMODULE winSock = NULL;
	while (winSock == NULL)
		winSock = GetModuleHandleA("ws2_32.dll");

	pConnect = (oConnect)hook_func((BYTE*)GetProcAddress(winSock, "connect"), (BYTE*)wrClient::Hooked_Connect, 5);
	pRecv = (oRecv)hook_func((BYTE*)GetProcAddress(winSock, "recv"), (BYTE*)wrClient::Hooked_Recv, 5);

	isHooked = true;
}
