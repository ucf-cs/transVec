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
	// If our update is the same as the page size, just use the index directly.
	if (S == V)
	{
		pos = index;
	}
	else
	{
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
size_t Page<T, U, S>::getSize()
{
	return SIZE;
}

template <class T, class U, size_t S, size_t V>
size_t DeltaPage<T, U, S, V>::getSize()
{
	return SIZE;
}

template <class T, class U, size_t S, size_t V>
bool DeltaPage<T, U, S, V>::copyFrom(DeltaPage<T, U, S, V> *page)
{
	// DEBUG:
	//printf("\nOld page:\n");
	//this->print();

	// DEBUG:
	//printf("\nTarget page:\n");
	//page->print();

	// TODO: Make sure all of this copying works correctly.
	this->bitset.read = page->bitset.read;
	this->bitset.write = page->bitset.write;
	this->bitset.checkBounds = page->bitset.checkBounds;
	this->transaction = page->transaction;
	// This will be set later. No need to copy it.
	//next = page->next;
	for (size_t i = 0; i < V; i++)
	{
		this->newVal[i] = page->newVal[i];
		// This will be set later. No need to copy it.
		//this->oldVal[i] = page->oldVal[i];
	}

	// DEBUG:
	//printf("\nUpdated page:\n");
	//this->print();
}

template <class T, class U, size_t S>
void Page<T, U, S>::print()
{
	std::cout << "SEG_SIZE \t= " << SEG_SIZE << std::endl;
	std::cout << "SIZE \t\t= " << SIZE << std::endl;
	std::cout << "Bitset:" << std::endl;
	std::cout << "read \t\t= " << bitset.read.to_string() << std::endl;
	std::cout << "write \t\t= " << bitset.write.to_string() << std::endl;
	std::cout << "checkBounds \t= " << bitset.checkBounds.to_string() << std::endl;
	std::cout << "transaction \t= " << transaction << std::endl;
	std::cout << "next \t\t= " << next << std::endl;
	return;
}

template <class T, class U, size_t S, size_t V>
void DeltaPage<T, U, S, V>::print()
{
	Page<T, U, S>::print();
	std::cout << "SIZE \t\t= " << SIZE << std::endl;
	for (size_t i = 0; i < SIZE; i++)
	{
		std::cout << "oldVal[" << i << "] \t= " << std::setw(11) << oldVal[i] << "\t"
				  << "newVal[" << i << "] \t= " << std::setw(11) << newVal[i] << std::endl;
	}
	return;
}

template class Page<VAL, VAL, SGMT_SIZE>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 1>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 2>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 3>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 4>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 5>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 6>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 7>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 8>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 9>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 10>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 11>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 12>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 13>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 14>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 15>;
template class DeltaPage<VAL, VAL, SGMT_SIZE, 16>;

template class Page<size_t, VAL, 1>;