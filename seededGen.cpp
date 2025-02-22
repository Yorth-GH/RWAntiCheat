#include "includes.h"

int SeededGenerator::next() 
{
    current = (A * current + C) % M;
    return current;
}

std::vector<int> SeededGenerator::generateSequence(int length)
{
    std::vector<int> sequence;
    for (int i = 0; i < length; i++)
    {
        sequence.push_back(next());
    }
    return sequence;
}
