#include "../includes.h"

void applicationCore::Initialize(socketClient* connection)
{
	if (connection == NULL)
		connection = new socketClient();

	if (connection->Connect(inet_addr(IP), PORT) == false)
	{
		ExitProcess(0);
		return;
	} 

	// Send Handshake
}

void applicationCore::Uninitialize()
{
}

void applicationCore::Update(socketClient* connection)
{
}
