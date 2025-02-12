#pragma once 
 
class packetBuilder
{
public:
	int OpCode = 0;
	std::vector<std::string> blocks;

	packetBuilder(int opc) { OpCode = opc; }

	void AddInt(int i) { AddString(std::to_string(i)); }
	void AddString(std::string s)
	{
		std::string input = "";
		for (int i = 0; i < s.length(); i++) {
			input += (char)(s.at(i) == 0x20 ? 0x1D : s.at(i));
		}
		blocks.push_back(input);
	}

	std::string Build()
	{
		std::string output = "";

		output += std::to_string((__int64)std::time(nullptr)) + " ";
		output += std::to_string(OpCode) + " ";

		for (auto& block : blocks)
		{
			output += block + " ";
		}

		output += "\n";

		return output;
	}

	void Send(SOCKET s, BYTE key)
	{
		std::string printPacket = Build();
		for (int i = 0; i < printPacket.length(); i++)
		{
			char character = (char)(printPacket.at(i) ^ key);
			printPacket.at(i) = character;
		}
		send(s, printPacket.c_str(), printPacket.length(), 0);
	}
};