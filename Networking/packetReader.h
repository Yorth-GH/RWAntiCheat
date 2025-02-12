#pragma once
 
class packetReader
{
public:
	int TimeStamp = 0;
	int OpCode = 0;
	std::vector<std::string> blocks;

	bool Valid = false;

	packetReader(std::string data)
	{
		std::string _tempBlock = "";
		int _blockId = 0;
		for (int i = 0; i < data.length(); i++)
		{
			char character = data.at(i);
			if (character == '\n') break;
			if (character != ' ')
			{
				_tempBlock += character == '\x1D' ? ' ' : character;
			}
			else
			{
				if (_blockId == 0) { TimeStamp = (int)atoi(_tempBlock.c_str()); }
				else if (_blockId == 1) { OpCode = (int)atoi(_tempBlock.c_str()); Valid = true; }
				else { blocks.push_back(_tempBlock); }
				_blockId++;
				_tempBlock = "";
			}
		}
	}

	std::string GetString(int i) { return std::string(blocks[i]); }
	int GetInt(int i) { return(int)(atoi(GetString(i).c_str())); }
};