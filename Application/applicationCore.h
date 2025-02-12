#pragma once
 
class socketClient;

class applicationCore {
public:
	void Initialize(socketClient* connection);
	void Uninitialize();
	 
	void Update(socketClient* connection);
};