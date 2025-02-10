#include "../includes.h"

#pragma comment(lib, "Ws2_32.lib")

char buffer[64];

void client_f(SOCKET cl)
{
	while (true)
	{
		ZeroMemory(buffer, 64);
		recv(cl, buffer, 64, 0);
		
		
	}
}

void print_message(std::string str)
{
	if (str.at(0) == '0')
		std::cout << "Connection accepted!" << std::endl;
	else if (str.at(0) == '1')
		std::cout << "Loaded from within another game! Game Path: " << str.substr(1) << std::endl;
	else if (str.at(0) == '2')
		std::cout << "Debugger detected!" << std::endl;
	else if (str.at(0) == '3')
		std::cout << "Forbidden process open!" << str.substr(1) << std::endl;
	else if (str.at(0) == '4')
		std::cout << "Unknown module loaded!" << str.substr(1) << std::endl;
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

	InetPton(AF_INET, IP, &server_socket.sin_addr);

	bind(server, (sockaddr*)&server_socket, sizeof(server_socket));

	listen(server, SOMAXCONN);

	int clientsize = sizeof(client_socket);

	while (true)
	{
		client = accept(server, (sockaddr*)&client_socket, &clientsize);

		std::cout << "Client connecting from IP: " << inet_ntoa(client_socket.sin_addr) << std::endl;

		std::thread client_thread(client_f, client);
		client_thread.detach();
	}
}