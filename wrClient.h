#pragma once 

typedef int(__stdcall* oConnect)(SOCKET socket, sockaddr* name, int namelen);
typedef int(__stdcall* oRecv)(SOCKET socket, CHAR* buf, int len, int flags);
typedef int(__stdcall* oSend)(SOCKET socket, const CHAR* buf, int len, int flags);
 
class wrClient
{
public:
	static void HookNetwork();

	static void HandleServerPacket(packetReader* packet);

	static int __stdcall Hooked_Recv(SOCKET s, CHAR* buf, int len, int flags);
	static int WINAPI Hooked_Connect(SOCKET s, sockaddr* name, int len);

	static bool isHooked;
	static bool isOnGameServer;

	static int userId;
	static std::string username;
	static std::string nickname;

	static void ResetClient();

	static SOCKET gameServerSocket;
	static std::string sendBuffer;
	static std::string recvBuffer;

	static oConnect pConnect;
	static oRecv pRecv;
	static oSend pSend;
};