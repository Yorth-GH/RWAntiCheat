#include "../includes.h"

char buffer[64];

std::string get_message(std::string str)
{
	if (str.at(0) == '0')
		return "Connection accepted!";
	else if (str.at(0) == '1')
		return "Loaded from within another game! Game Path: " + str.substr(1);
	else if (str.at(0) == '2')
		return "Debugger detected!";
	else if (str.at(0) == '3')
		return "Forbidden process open!" + str.substr(1);
	else if (str.at(0) == '4')
		return "Unknown module loaded!" + str.substr(1);
	else if (str.at(0) == '5')
		return "Heartbeat from client received!";
	else if (str.at(0) == '6')
		return "Connection closing!";

	return "Incorrect message! Client code error or possible reverse engineering?";
}

void client_f(SOCKET cl, char* ip)
{	
	int count = 0;
	std::string logname = "LOG " + std::string(ip) + " " + std::to_string(count) + ".txt";

	if (std::ifstream(logname).good())
	{
		count++;
		logname = "LOG " + std::string(ip) + " " + std::to_string(count) + ".txt";
	}

	std::ofstream log(logname);

	while (true)
	{
		ZeroMemory(buffer, 64);
		if (recv(cl, buffer, 64, 0) <= 0)
			break;

		std::cout << get_message(std::string(buffer)) << " - " << ip << std::endl;
		log << get_message(std::string(buffer)) << "\n";
		log.flush();
	}
	closesocket(cl);
}

void main(int argc, char* argv[])
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	sockaddr_in server_socket;
	sockaddr_in client_socket;

	SOCKET client;
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

	server_socket.sin_family = AF_INET;
	server_socket.sin_port = htons(PORT);

	InetPton(AF_INET, IP, &server_socket.sin_addr);

	bind(server, (sockaddr*)&server_socket, sizeof(server_socket));

	listen(server, SOMAXCONN);

	int clientsize = sizeof(client_socket);

	while (true)
	{
		client = accept(server, (sockaddr*)&client_socket, &clientsize);

		std::cout << "Client connecting from IP: " << inet_ntoa(client_socket.sin_addr) << std::endl;

		std::thread client_thread(client_f, client, inet_ntoa(client_socket.sin_addr));
		client_thread.detach();
	}

	WSACleanup();
}