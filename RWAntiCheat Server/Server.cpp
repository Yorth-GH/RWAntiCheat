#include <thread>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "27015"
#define IP "127.0.0.1"

char buffer[512];

void client_f(SOCKET cl)
{
	while (true)
	{
		ZeroMemory(buffer, 512);
		recv(cl, buffer, 512, 0);
		std::cout << buffer << std::endl;
	}
}

void main()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	sockaddr_in server_socket;
	sockaddr_in client_socket;

	SOCKET client;
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

	server_socket.sin_family = AF_INET;
	server_socket.sin_port = htons(27999);

	inet_pton(AF_INET, IP, &server_socket.sin_addr);

	bind(server, (sockaddr*)&server_socket, sizeof(server_socket));

	listen(server, SOMAXCONN);

	int clientsize = sizeof(client_socket);

	while (true)
	{
		client = accept(server, (sockaddr*)&client_socket, &clientsize);

		std::cout << "Accepted client: " << client_socket.sin_addr.S_un.S_addr << std::endl;

		std::thread client_thread(client_f, client);
		client_thread.detach();
	}
}