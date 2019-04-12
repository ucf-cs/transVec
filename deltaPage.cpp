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
	size_t index;
	switch (size)
	{
	case 1:
		index = DeltaPage<T, U, S, 1>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 1>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 1>;
		}
		break;
	case 2:
		index = DeltaPage<T, U, S, 2>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 2>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 2>;
		}
		break;
	case 3:
		index = DeltaPage<T, U, S, 3>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 3>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 3>;
		}
		break;
	case 4:
		index = DeltaPage<T, U, S, 4>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 4>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 4>;
		}
		break;
	case 5:
		index = DeltaPage<T, U, S, 5>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 5>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 5>;
		}
		break;
	case 6:
		index = DeltaPage<T, U, S, 6>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 6>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 6>;
		}
		break;
	case 7:
		index = DeltaPage<T, U, S, 7>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 7>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 7>;
		}
		break;
	case 8:
		index = DeltaPage<T, U, S, 8>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 8>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 8>;
		}
		break;
	case 9:
		index = DeltaPage<T, U, S, 9>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 9>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 9>;
		}
		break;
	case 10:
		index = DeltaPage<T, U, S, 10>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 10>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 10>;
		}
		break;
	case 11:
		index = DeltaPage<T, U, S, 11>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 11>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 11>;
		}
		break;
	case 12:
		index = DeltaPage<T, U, S, 12>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 12>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 12>;
		}
		break;
	case 13:
		index = DeltaPage<T, U, S, 13>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 13>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 13>;
		}
		break;
	case 14:
		index = DeltaPage<T, U, S, 14>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 14>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 14>;
		}
		break;
	case 15:
		index = DeltaPage<T, U, S, 15>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 15>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 15>;
		}
		break;
	case 16:
		index = DeltaPage<T, U, S, 16>::poolCounter.fetch_add(1);
		if (index < POOL_SIZE)
		{
			return DeltaPage<T, U, S, 16>::pool[index];
		}
		else
		{
			printf("Pool for update size %lu is full.\n", size);
			return new DeltaPage<T, U, S, 16>;
		}
		break;
	default:
		printf("Invalid page size specified (%lu). Assuming size=%lu\n", size, S);
		return new DeltaPage<T, U, S, S>;
	}
}

template <class T, class U, size_t S, size_t V>
void DeltaPage<T, U, S, V>::initPool()
{
	poolCounter.store(0);
	pool = new DeltaPage<T, U, S, V>*[POOL_SIZE];
	for(size_t i=0;i<POOL_SIZE;i++){
		pool[i] = new DeltaPage<T, U, S, V>;
	}
}

template <class T, class U, size_t S, size_t V>
std::atomic<size_t> DeltaPage<T, U, S, V>::poolCounter;

template <class T, class U, size_t S, size_t V>
DeltaPage<T, U, S, V> **DeltaPage<T, U, S, V>::pool;

template class Page<int, int, SGMT_SIZE>;
template class DeltaPage<int, int, SGMT_SIZE, 1>;
template class DeltaPage<int, int, SGMT_SIZE, 2>;
template class DeltaPage<int, int, SGMT_SIZE, 3>;
template class DeltaPage<int, int, SGMT_SIZE, 4>;
template class DeltaPage<int, int, SGMT_SIZE, 5>;
template class DeltaPage<int, int, SGMT_SIZE, 6>;
template class DeltaPage<int, int, SGMT_SIZE, 7>;
template class DeltaPage<int, int, SGMT_SIZE, 8>;
template class DeltaPage<int, int, SGMT_SIZE, 9>;
template class DeltaPage<int, int, SGMT_SIZE, 10>;
template class DeltaPage<int, int, SGMT_SIZE, 11>;
template class DeltaPage<int, int, SGMT_SIZE, 12>;
template class DeltaPage<int, int, SGMT_SIZE, 13>;
template class DeltaPage<int, int, SGMT_SIZE, 14>;
template class DeltaPage<int, int, SGMT_SIZE, 15>;
template class DeltaPage<int, int, SGMT_SIZE, 16>;

template class Page<size_t, int, 1>;
template class DeltaPage<size_t, int, 1, 1>;