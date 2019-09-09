// TODO: Unused. Consider removal.

#ifndef RANDOM_POOL_HPP
#define RANDOM_POOL_HPP

#include <atomic>
#include <random>
#include <vector>

#define RANGE SIZE_MAX - 1

// Generates random numbers in advance of actually needing them.
class RandomNumberPool
{
private:
    std::vector<std::vector<size_t>> pools;
    std::atomic<size_t> count;

public:
    RandomNumberPool(size_t numThreads = 1, size_t numElements = 0)
    {
        count.store(0);

        pools.reserve(numThreads);

        std::random_device rd;
        std::mt19937_64 e2(rd());
        std::uniform_int_distribution<size_t> dist(0, RANGE);

        for (size_t i = 0; i < numThreads; i++)
        {
            std::vector<size_t> elements;
            elements.reserve(numElements);
            for (size_t j = 0; j < numElements; j++)
            {
                elements.push_back(dist(e2));
            }
            pools.push_back(elements);
        }
        return;
    }
    size_t getNum(size_t threadNum)
    {
        size_t ret;
        if (pools[threadNum].size() > 0)
        {
            ret = pools[threadNum].back();
            pools[threadNum].pop_back();
        }
        else
        {
            // printf("Random number generator ran out of numbers.\n");
            // printf("Generated %lu more numbers\n", count.fetch_add(1));
            std::random_device rd;
            std::mt19937_64 e2(rd());
            std::uniform_int_distribution<size_t> dist(0, RANGE);
            ret = dist(e2);
        }
        return ret;
    }
};

#endif