#include "rwSet.hpp"

template <typename T>
RWSet<T>::~RWSet()
{
	operations.clear();
	return;
}

template <typename T>
std::pair<size_t, size_t> RWSet<T>::access(size_t pos)
{
	// All other cases.
	size_t first = pos / Page<T, T, SGMT_SIZE>::SEG_SIZE;
	size_t second = pos % Page<T, T, SGMT_SIZE>::SEG_SIZE;
	return std::make_pair(first, second);
}

template <typename T>
bool RWSet<T>::createSet(Desc<T> *descriptor, TransactionalVector<T> *vector)
{
	// DEBUG:
	//descriptor->print();

	// Go through each operation.
	for (size_t i = 0; i < descriptor->size; i++)
	{
		std::pair<size_t, size_t> indexes;
		RWOperation<T> *op = NULL;
		switch (descriptor->ops[i].type)
		{
		case Operation<T>::OpType::read:
			indexes = access(descriptor->ops[i].index);
			op = &operations[indexes.first][indexes.second];
			// If this location has already been written to, read its value.
			// This is done to handle operations that are totally internal to the transaction.
			if (op->lastWriteOp != NULL)
			{
				descriptor->ops[i].ret = op->lastWriteOp->val;
				// If the value was unset (shouldn't really be possible here), then our transaction fails.
				if (op->lastWriteOp->val == UNSET)
				{
					descriptor->status.store(Desc<T>::TxStatus::aborted);
					// DEBUG:
					//printf("Aborted!\n");
					return false;
				}
			}
			// We haven't written here before. Request a read from the shared structure.
			else
			{
				if (op->checkBounds == RWOperation<T>::Assigned::unset)
				{
					op->checkBounds = RWOperation<T>::Assigned::yes;
				}
				// Add ourselves to the read list.
				op->readList.push_back(&descriptor->ops[i]);
			}
			break;
		case Operation<T>::OpType::write:
			indexes = access(descriptor->ops[i].index);
			op = &operations[indexes.first][indexes.second];
			if (op->checkBounds == RWOperation<T>::Assigned::unset)
			{
				op->checkBounds = RWOperation<T>::Assigned::yes;
			}
			op->lastWriteOp = &descriptor->ops[i];
			break;
		case Operation<T>::OpType::pushBack:
			getSize(descriptor, vector);
			// This should never happen, but make sure we don't have an integer overflow.
			if (size == SIZE_MAX)
			{
				descriptor->status.store(Desc<T>::TxStatus::aborted);
				// DEBUG:
				//printf("Aborted!\n");
				return false;
			}
			indexes = access(size++);

			op = &operations[indexes.first][indexes.second];
			if (op->checkBounds == RWOperation<T>::Assigned::unset)
			{
				op->checkBounds = RWOperation<T>::Assigned::no;
			}
			op->lastWriteOp = &descriptor->ops[i];
			break;
		case Operation<T>::OpType::popBack:
			getSize(descriptor, vector);
			// Prevent popping past the bottom of the stack.
			if (size < 1)
			{
				descriptor->status.store(Desc<T>::TxStatus::aborted);
				// DEBUG:
				//printf("Aborted!\n");
				return false;
			}
			indexes = access(--size);

			op = &operations[indexes.first][indexes.second];
			// If this location has already been written to, read its value. This is done to handle operations that are totally internal to the transaction.
			if (op->lastWriteOp != NULL)
			{
				descriptor->ops[i].ret = op->lastWriteOp->val;
			}
			// We haven't written here before. Request a read from the shared structure.
			else
			{
				// Add ourselves to the read list.
				op->readList.push_back(&descriptor->ops[i]);
			}
			// We actually write an unset value here when we pop.
			// Make sure we explicitly mark as UNSET.
			// Don't leave this in the hands of the person creating the transactions.
			descriptor->ops[i].val = UNSET;
			if (op->checkBounds == RWOperation<T>::Assigned::unset)
			{
				op->checkBounds = RWOperation<T>::Assigned::no;
			}
			op->lastWriteOp = &descriptor->ops[i];
			break;
		case Operation<T>::OpType::size:
			getSize(descriptor, vector);

			// NOTE: Don't store in ret. Store in index, as a special case for size calls.
			descriptor->ops[i].index = size;
			break;
		case Operation<T>::OpType::reserve:
			// We only care about the largest reserve call.
			// All other reserve operations will consolidate into a single call at the beginning of the transaction.
			if ((size_t)descriptor->ops[i].index > maxReserveAbsolute)
			{
				maxReserveAbsolute = descriptor->ops[i].index;
			}
			break;
		default:
			// Unexpected operation type found.
			break;
		}
	}

	// If we accessed size, we need to report what we changed it to.
	if (sizeDesc != NULL)
	{
		sizeDesc->set(0, NEW_VAL, size);
	}
	return true;
}

template <typename T>
void RWSet<T>::setToPages(Desc<T> *descriptor)
{
	// All of the pages we want to insert (except size), ordered from low to high.
	std::map<size_t, Page<T, T, SGMT_SIZE> *> *pages = new std::map<size_t, Page<T, T, SGMT_SIZE> *>();

	// For each page to generate.
	// These are all independent of shared memory.
	for (auto i = operations.begin(); i != operations.end(); ++i)
	{
		// Determine how many elements are in this page.
		size_t elementCount = i->second.size();
		// Create the initial page.
		Page<T, T, SGMT_SIZE> *page = Page<T, T, SGMT_SIZE>::getNewPage(elementCount);
		// Link the page to the transaction descriptor.
		page->transaction = descriptor;

		// For each element to place in the given page.
		for (auto j = i->second.begin(); j != i->second.end(); ++j)
		{
			size_t index = j->first;
			// Infer a read based on the read list size.
			page->bitset.read[index] = (j->second.readList.size() > 0);
			// Infer a write based on the write op pointer.
			page->bitset.write[index] = (j->second.lastWriteOp != NULL);
			page->bitset.checkBounds[index] = (j->second.checkBounds == RWOperation<T>::Assigned::yes) ? true : false;
			// If a write occured, place the appropriate new value from it.
			if (j->second.lastWriteOp != NULL)
			{
				page->set(index, NEW_VAL, j->second.lastWriteOp->val);
			}
		}
		(*pages)[i->first] = page;
	}

	// Store a pointer to the pages in the descriptor.
	// Only the first thread to finish the job succeeds here.
	std::map<size_t, Page<T, T, SGMT_SIZE> *> *nullVal = NULL;
	descriptor->pages.compare_exchange_strong(nullVal, pages);

	return;
}

template <typename T>
void RWSet<T>::setOperationVals(Desc<T> *descriptor, std::map<size_t, Page<T, T, SGMT_SIZE> *> pages)
{
	// If the returned values have already been copied over, do no more.
	if (descriptor->returnedValues.load() == true)
	{
		return;
	}

	// For each page.
	for (auto i = pages.begin(); i != pages.end(); ++i)
	{
		// Get the page.
		Page<T, T, SGMT_SIZE> *page = i->second;
		// Get a list of values in the page.
		std::bitset<page->SEG_SIZE> usedBits = page->bitset.read | page->bitset.write;
		// For each element.
		for (auto j = 0; j < page->SEG_SIZE; ++j)
		{
			// Skip elements that aren't represented in this page.
			if (!usedBits[j])
			{
				continue;
			}

			// Get the value.
			T value = UNSET;
			page->get(j, OLD_VAL, value);

			// Get the read list for the current element.
			std::vector<Operation<T> *> readList = operations.at(i->first).at(j).readList;
			// For each operation attempting to read the element.
			for (size_t k = 0; k < readList.size(); k++)
			{
				// Assign the return value.
				readList[k]->ret = value;
			}
		}
	}

	// Subsequent attempts to load values will not have to repeat this work.
	descriptor->returnedValues.store(true);

	return;
}

template <typename T>
size_t RWSet<T>::getSize(Desc<T> *descriptor, TransactionalVector<T> *vector)
{
	if (sizeDesc != NULL)
	{
		return size;
	}

	// Prepend a read page to size.
	// The size page is always of size 1.
	// Set all unchanging page values here.
	sizeDesc = new DeltaPage<size_t, T, 1, 1>;
	sizeDesc->bitset.read.set();
	sizeDesc->bitset.write.set();
	sizeDesc->bitset.checkBounds.reset();
	sizeDesc->transaction = descriptor;
	sizeDesc->next = NULL;

	Page<size_t, T, 1> *rootPage = NULL;
	do
	{
		// Get the current head.
		rootPage = vector->size.load();

		// Quit if the transaction is no longer active.
		if (descriptor->status.load() != Desc<T>::TxStatus::active)
		{
			return 0;
		}

		// If the root page does not exist.
		// Root page should never be NULL, ever.
		if (rootPage == NULL)
		{
			// Assume an initial size of 0.
			sizeDesc->set(0, OLD_VAL, 0);
		}
		// If a helper got here first.
		else if (rootPage->transaction == sizeDesc->transaction)
		{
			// Do not insert again.
			break;
		}
		else
		{
			enum Desc<T>::TxStatus status = rootPage->transaction->status.load();
			// TODO: Use helping scheme here.
			// Just busy wait for now.
			while (status == Desc<T>::TxStatus::active)
			{
				status = rootPage->transaction->status.load();
			}

			// Store the root page's value as an old value in case we abort.
			// Get the appropriate value from the root page depending on whether or not it succeeded.
			size_t value = UNSET;
			if (status == Desc<T>::TxStatus::committed)
			{
				rootPage->get(0, NEW_VAL, value);
			}
			else
			{
				rootPage->get(0, OLD_VAL, value);
			}
			sizeDesc->set(0, OLD_VAL, value);
		}
		// Append the old page onto the new page. Used to maintain history.
		sizeDesc->next = rootPage;
	}
	// Replace the page. Finish on success. Retry on failure.
	while (!vector->size.compare_exchange_strong(rootPage, sizeDesc));

	// Store the actual size locally.
	sizeDesc->get(0, OLD_VAL, size);

	return size;
}

template class RWSet<VAL>;