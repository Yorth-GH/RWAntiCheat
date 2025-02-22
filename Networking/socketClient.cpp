#include "../includes.h"

SOCKET socketClient::connection = NULL;
std::string socketClient::recvBuffer = "";
int socketClient::userId = -1;
BOOL socketClient::IsActive = FALSE;

bool socketClient::Connect(ULONG ip, USHORT port)
{
	WSADATA wsa_data;

	SOCKADDR_IN addr;

	int result = 0;

	result = WSAStartup(MAKEWORD(2, 0), &wsa_data);
	if (result != NO_ERROR) return false;

	connection = socket(AF_INET, SOCK_STREAM, 0);
	if (connection == INVALID_SOCKET) return false;

	addr.sin_addr.s_addr = ip;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);


	result = connect(connection, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));
	if (result == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	std::cout << "[AC] Connected to AC server!" << std::endl;

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)RecvData, 0, 0, 0);
	//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CallbackThread, 0, 0, 0);

	return true;
}

void socketClient::Close()
{
	closesocket(connection);
	WSACleanup();
	ExitProcess(0);
}

void socketClient::Send(std::string buffer)
{
	if (connection == NULL || buffer.length() == 0) return;
	send(connection, buffer.c_str(), (int)buffer.length(), 0);
}

void socketClient::Handle(std::string packet)
{
	auto p = new packetReader(packet);
	if (p->Valid)
	{
		switch (p->OpCode)
		{
			case 15001: // ACK_LOGIN
			{
				int code = p->GetInt(0);
				std::cout << "[AC] Got a ACK_LOGIN with code " << std::to_string(code) << std::endl;
				if (code == 1)
				{
					IsActive = TRUE;
				}
				else
				{
					Close();
				}
				break;
			}
			case 15011: // ACK_HEARTBEAT
			{
				break;
			}
			default:
				break;
		}
	}
}
  
DWORD __stdcall socketClient::RecvData(LPVOID arg)
{
	while (true)
	{
		char buffer[8192];
		int len = recv(connection, buffer, 8192, 0);
		if (len > 0)
		{
			for (int i = 0; i < len; i++)
			{
				recvBuffer += (char)(buffer[i] ^ 0x96);
			}

			int packetBegin = 0;
			for (int i = 0; i < recvBuffer.length(); i++)
			{
				if (recvBuffer.at(i) == '\n')
				{
					std::string packet = recvBuffer.substr(packetBegin, i - packetBegin);

					Handle(packet);

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
		else
			break;
	}
	Close();
	return 0;
}
