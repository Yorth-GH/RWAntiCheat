#pragma once

class SeededGenerator {
private:
    int seed;
    int current;
    static constexpr int A = 1664525;
    static constexpr int C = 1013904223;
    static constexpr int M = 2147483647; // int max

public:
    SeededGenerator(int seed) : seed(seed), current(seed) {}

    int next();

    std::vector<int> generateSequence(int length);
};
//
//int main()
//{
//    SeededGenerator generator(12345);
//    std::vector<int> sequence = generator.generateSequence(10);
//
//    for (int num : sequence)
//    {
//        std::cout << num << " ";
//    }
//    std::cout << std::endl;
//
//    return 0;
//}
