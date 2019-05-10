#ifndef RANDOM_POOL_H
#define RANDOM_POOL_H

#include <random>
#include <vector>

#define RANGE SIZE_MAX - 1

// Generates random numbers in advance of actually needing them.
class RandomNumberPool
{
private:
    std::vector<size_t> elements;

public:
    RandomNumberPool(size_t numElements = 0)
    {
        std::random_device rd;
        std::mt19937_64 e2(rd());
        std::uniform_int_distribution<size_t> dist(0, RANGE);
        for (size_t i = 0; i < numElements; i++)
        {
            elements.push_back(dist(e2));
        }
        return;
    }
    size_t getNum()
    {
        if (elements.size() <= 0)
        {
            printf("Random number generator ran out of numbers.\n");
            std::random_device rd;
            std::mt19937_64 e2(rd());
            std::uniform_int_distribution<size_t> dist(0, RANGE);
            return dist(e2);
        }
        size_t ret = elements.back();
        elements.pop_back();
    }
};

#endif