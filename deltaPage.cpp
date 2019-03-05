#include "deltaPage.hpp"

template<typename T>
Page<T>::Page(size_t size)
{
    this->size = size;
    newVal = new T[size];
    oldVal = new T[size];

    newVal[5] = 5;

    next = NULL;
    this->transaction = NULL;

    return;
}

template<typename T>
Page<T>::~Page()
{
    if (newVal != NULL)
    {
        delete newVal;
    }
    if (oldVal != NULL)
    {
        delete oldVal;
    }
}

template<typename T>
T *Page<T>::at(size_t index, bool newVals)
{
    // Don't go out of bounds.
    if (index > SEG_SIZE)
    {
        return NULL;
    }
    // Used bits are any locations that are read from or written to.
    std::bitset<SEG_SIZE> usedBits = bitset.read | bitset.write;
    // If the bit isn't even in this page, we can't return a valid value.
    if (usedBits[index] != 1)
    {
        return NULL;
    }
    // Get the number of bits before the bit at our target index.
    size_t pos = 0;
    for (size_t i = 0; i < index; i++)
    {
        if (usedBits[i])
        {
            pos++;
        }
    }
    // Return the address of the position in question.
    // This way, we can change the value stored in the page, if needed.
    return newVals ? &newVal[pos] : &oldVal[pos];
}

template class Page<int>;
template class Page<size_t>;