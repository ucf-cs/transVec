#include "deltaPage.hpp"

template <typename T, typename U, size_t S>
Page<T, U, S>::Page(size_t size)
{
    this->size = size;
    newVal = new T[size];
    oldVal = new T[size];
    for (size_t i = 0; i < size; i++)
    {
        newVal[i] = UNSET;
        oldVal[i] = UNSET;
    }

    return;
}

template <typename T, typename U, size_t S>
Page<T, U, S>::~Page()
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

template <typename T, typename U, size_t S>
T *Page<T, U, S>::at(size_t index, bool newVals)
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
    // Consider all bits in our bitset just short of the target.
    for (size_t i = 0; i < index; i++)
    {
        // If the value is in the segment.
        if (usedBits[i])
        {
            // Increment our target index by 1.
            pos++;
        }
    }
    // Now pos is the index of our value in the array.

    // Return the address of the position in question.
    // This way, we can change the value stored in the page, if needed.
    if (newVals)
    {
        return &newVal[pos];
    }
    else
    {
        return &oldVal[pos];
    }
}

template <typename T, typename U, size_t S>
T Page<T, U, S>::get(size_t index, bool newVals)
{
    T *pointer = at(index, newVals);
    if (pointer == NULL)
    {
        return UNSET;
    }
    return *pointer;
}

template <typename T, typename U, size_t S>
bool Page<T, U, S>::set(size_t index, bool newVals, T val)
{
    T *element = at(index, newVals);
    *element = val;
    return true;
}

template <typename T, typename U, size_t S>
void Page<T, U, S>::printAt(size_t index)
{
    T oldVal = get(index, OLD_VAL);
    T newVal = get(index, NEW_VAL);
    return;
}

template class Page<int, int, 8>;
template class Page<size_t, int, 1>;