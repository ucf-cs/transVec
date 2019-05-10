#include "deltaPage.hpp"

Page::Page()
{
	for (size_t i = 0; i < SGMT_SIZE; i++)
	{
		newVal[i] = UNSET;
		oldVal[i] = UNSET;
	}
	return;
}

VAL *Page::at(size_t index, bool newVals)
{
	// Don't go out of bounds.
	if (index > Page::SEG_SIZE)
	{
		return NULL;
	}
	// Used bits are any locations that are read from or written to.
	std::bitset<Page::SEG_SIZE> usedBits =
		Page::bitset.read | Page::bitset.write;
	// If the bit isn't even in this page, we can't return a valid value.
	if (usedBits[index] != 1)
	{
		return NULL;
	}

	// Return the address of the position in question.
	// This way, we can change the value stored in the page, if needed.
	if (newVals)
	{
		return &newVal[index];
	}
	else
	{
		return &oldVal[index];
	}
}

bool Page::get(size_t index, bool newVals, VAL &val)
{
	VAL *pointer = at(index, newVals);
	if (pointer == NULL)
	{
		return false;
	}
	val = *pointer;
	return true;
}

bool Page::set(size_t index, bool newVals, VAL val)
{
	VAL *element = at(index, newVals);
	if (element == NULL)
	{
		return false;
	}
	*element = val;
	return true;
}

bool Page::copyFrom(Page *page)
{
	this->bitset.read = page->bitset.read;
	this->bitset.write = page->bitset.write;
	this->bitset.checkBounds = page->bitset.checkBounds;
	this->transaction = page->transaction;
	// This will be set later. No need to copy it.
	//next = page->next;
	for (size_t i = 0; i < page->SEG_SIZE; i++)
	{
		this->newVal[i] = page->newVal[i];
		// This will be set later. No need to copy it.
		//this->oldVal[i] = page->oldVal[i];
	}
}

void Page::print()
{
	std::cout << "SEG_SIZE \t= " << SEG_SIZE << std::endl;
	std::cout << "Bitset:" << std::endl;
	std::cout << "read \t\t= " << bitset.read.to_string() << std::endl;
	std::cout << "write \t\t= " << bitset.write.to_string() << std::endl;
	std::cout << "checkBounds \t= " << bitset.checkBounds.to_string() << std::endl;
	std::cout << "transaction \t= " << transaction << std::endl;
	std::cout << "next \t\t= " << next << std::endl;
	for (size_t i = 0; i < this->SEG_SIZE; i++)
	{
		std::cout << "oldVal[" << i << "] \t= " << std::setw(11) << oldVal[i] << "\t"
				  << "newVal[" << i << "] \t= " << std::setw(11) << newVal[i] << std::endl;
	}
	return;
}