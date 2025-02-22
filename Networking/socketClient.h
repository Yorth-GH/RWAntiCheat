#pragma once
  
class socketClient
{
public:
	static SOCKET connection;

	static bool Connect(ULONG ip, USHORT port);
	static void Close();
	static void Send(std::string buffer);

	static void Handle(std::string packet); 
	
	static std::vector<int> HandshakeTokens;
	static int TokenCount;

	static DWORD WINAPI RecvData(LPVOID arg);
	static DWORD WINAPI HandshakeThread(LPVOID arg);

	static BOOL IsActive;
private:  
	static std::string recvBuffer;
};