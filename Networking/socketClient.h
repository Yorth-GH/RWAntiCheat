#pragma once
  
class socketClient
{
public:
	static SOCKET connection;

	static bool Connect(ULONG ip, USHORT port);
	static void Close();
	static void Send(std::string buffer);

	static void Handle(std::string packet); 

	static void SetUserId(int i) { userId = i; }

	static DWORD WINAPI RecvData(LPVOID arg); 

	static BOOL IsActive;
private:
	static int userId;

	static std::string recvBuffer;
};