#include "deltaPage.hpp"

template <typename T, typename U, size_t S, size_t V>
DeltaPage<T, U, S, V>::DeltaPage()
{
	for (size_t i = 0; i < SIZE; i++)
	{
		newVal[i] = UNSET;
		oldVal[i] = UNSET;
	}
	return;
}

template <typename T, typename U, size_t S, size_t V>
T *DeltaPage<T, U, S, V>::at(size_t index, bool newVals)
{
	// Don't go out of bounds.
	if (index > Page<T, U, S>::SEG_SIZE)
	{
		return NULL;
	}
	// Used bits are any locations that are read from or written to.
	std::bitset<Page<T, U, S>::SEG_SIZE> usedBits =
		Page<T, U, S>::bitset.read | Page<T, U, S>::bitset.write;
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

template <class T, class U, size_t S, size_t V>
bool DeltaPage<T, U, S, V>::get(size_t index, bool newVals, T &val)
{
	T *pointer = at(index, newVals);
	if (pointer == NULL)
	{
		return false;
	}
	val = *pointer;
	return true;
}

template <class T, class U, size_t S, size_t V>
bool DeltaPage<T, U, S, V>::set(size_t index, bool newVals, T val)
{
	T *element = at(index, newVals);
	if (element == NULL)
	{
		return false;
	}
	*element = val;
	return true;
}

template <class T, class U, size_t S>
Page<T, U, S> *Page<T, U, S>::getNewPage(size_t size)
{
	switch (size)
	{
	case 1:
		return new DeltaPage<T, U, S, 1>;
	case 2:
		return new DeltaPage<T, U, S, 2>;
	case 3:
		return new DeltaPage<T, U, S, 3>;
	case 4:
		return new DeltaPage<T, U, S, 4>;
	case 5:
		return new DeltaPage<T, U, S, 5>;
	case 6:
		return new DeltaPage<T, U, S, 6>;
	case 7:
		return new DeltaPage<T, U, S, 7>;
	case 8:
		return new DeltaPage<T, U, S, 8>;
	default:
		printf("Invalid page size specified. Assuming size=%lu\n", S);
		return new DeltaPage<T, U, S, S>;
	}
}

template class Page<int, int, 8>;
template class DeltaPage<int, int, 8, 1>;
template class DeltaPage<int, int, 8, 2>;
template class DeltaPage<int, int, 8, 3>;
template class DeltaPage<int, int, 8, 4>;
template class DeltaPage<int, int, 8, 5>;
template class DeltaPage<int, int, 8, 6>;
template class DeltaPage<int, int, 8, 7>;
template class DeltaPage<int, int, 8, 8>;

template class Page<size_t, int, 1>;
template class DeltaPage<size_t, int, 1, 1>;