#include "transVector.hpp"

template <typename T>
bool TransactionalVector<T>::reserve(size_t size)
{
	// Since we hold multiple elements per page, convert from a request for more elements to a request for more pages.

	// Reserve a page per SEG_SIZE elements.
	size_t reserveSize = size / Page<T, T, SGMT_SIZE>::SEG_SIZE;
	// Since integer division always rounds down, add one more page to handle the remainder.
	if (size % Page<T, T, SGMT_SIZE>::SEG_SIZE != 0)
	{
		reserveSize++;
	}
	// Perform the actual reservation.
	return array->reserve(reserveSize);
}

template <typename T>
bool TransactionalVector<T>::prependPage(size_t index, Page<T, T, SGMT_SIZE> *page)
{
	assert(page != NULL && "Invalid page passed in.");

	// Create a bitset to keep track of all locations of interest.
	std::bitset<Page<T, T, SGMT_SIZE>::SEG_SIZE> targetBits;
	// Set all bits we want to read or write.
	targetBits = page->bitset.read | page->bitset.write;

	// The head of the linkedlist of updates.
	Page<T, T, SGMT_SIZE> *rootPage = NULL;
	// The previous head. The new page has been updated up to this point.
	Page<T, T, SGMT_SIZE> *prevRoot = NULL;
	while (true)
	{
		// Quit if the transaction is no longer active.
		if (page->transaction->status.load() != Desc<T>::TxStatus::active)
		{
			return false;
		}

		// Get the head of the list of updates for this segment.
		if (!array->read(index, rootPage))
		{
			// Failure means the transaction attempted an invalid read or write, as the vector wasn't allocated to this point.
			page->transaction->status.store(Desc<T>::TxStatus::aborted);
			//printf("Aborted!\n");
			// No need to even try anymore. The whole transaction failed.
			return false;
		}

		// Disallow prepending a redundant page.
		// May occur during helping.
		if (rootPage != NULL && rootPage->transaction == page->transaction)
		{
			// DEBUG:
			//printf("Attempted to prepend a page to itself.\n");

			return false;
		}

		// Initialize the current page along the linked list of updates.
		Page<T, T, SGMT_SIZE> *currentPage = rootPage;
		// Traverse down the existing delta updates, collecting old values as we go.
		// We stop when we no longer need any more elements or when there are no pages left.
		while (!targetBits.none())
		{
			// If we reach the end before identifying all values, use a generic initializer page instead.
			if (currentPage == NULL)
			{
				currentPage = endPage;
			}
			// If we reach a page that we already traversed in a previous loop, then there's no reason to traverse again.
			if (currentPage == prevRoot)
			{
				break;
			}
			// Get the set of elements the current page has that we need.
			std::bitset<Page<T, T, SGMT_SIZE>::SEG_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
			// If this page has said elements.
			if (!posessedBits.none())
			{
				// Check the status of the transaction.
				typename Desc<T>::TxStatus status = currentPage->transaction->status.load();
				// If the current page is part of an active transaction.
				// No need to help reads, so check if there are any dependencies with writes.
				if (status == Desc<T>::TxStatus::active && (posessedBits & currentPage->bitset.write) != 0)
				{
					// TODO: Help the active transaction.
					// For now, just busy wait.
					while (currentPage->transaction->status.load() == Desc<T>::TxStatus::active)
					{
						continue;
						//insertPages(currentPage->transaction->pages);
					}
					// Update our status to its final (committed or aborted) state.
					status = currentPage->transaction->status.load();
				}
				// Go through the bits.
				for (size_t i = 0; i < Page<T, T, SGMT_SIZE>::SEG_SIZE; i++)
				{
					// We only care about the possessed bits.
					if (posessedBits[i])
					{
						// We only get the new value if it was write comitted.
						if (status == Desc<T>::TxStatus::committed && currentPage->bitset.write[i])
						{
							T val = UNSET;
							currentPage->get(i, NEW_VAL, val);
							page->set(i, OLD_VAL, val);
						}
						// Transaction was aborted or operation was a read.
						// Grab the old page's old value.
						else
						{
							T val = UNSET;
							currentPage->get(i, OLD_VAL, val);
							page->set(i, OLD_VAL, val);
						}
						T val = UNSET;
						// Abort if the operation fails our bounds check or we fail to get a value.
						if (!page->get(i, OLD_VAL, val) || (page->bitset.checkBounds[i] && val == UNSET))
						{
							page->transaction->status.store(Desc<T>::TxStatus::aborted);
							//printf("Aborted!\n");
							// No need to even try anymore. The whole transaction failed.
							return false;
						}
					}
				}
			}
			// Update our set of target bits.
			// We don't care about the elements we've just found anymore.
			targetBits &= posessedBits.flip();
			// Move on to the next delta update.
			currentPage = currentPage->next;
		}

		// Link our new page to the old root page.
		page->next = rootPage;

		// Insert the page into the desired location.
		if (array->tryWrite(index, rootPage, page))
		{
			// Finish on success.
			break;
		}
		else
		{
			// Retry on failure.
			prevRoot = rootPage;
		}
	}

	return true;
}

template <typename T>
void TransactionalVector<T>::insertPages(std::map<size_t, Page<T, T, SGMT_SIZE> *> pages, size_t startPage)
{
	// Get the start of the map.
	typename std::map<size_t, Page<T, T, SGMT_SIZE> *>::reverse_iterator iter = pages.rbegin();
	// Advance part-way through.
	if (startPage > 0)
	{
		std::advance(iter, startPage);
	}
	for (auto i = iter; i != pages.rend(); ++i)
	{
		// If some prepend produced a failure, don't insert any more pages for this transaction.
		if (i->second->transaction->status.load() == Desc<T>::TxStatus::aborted)
		{
			break;
		}

		// Prepend the page.
		// Returns false if it caused an abort.
		if (!prependPage(i->first, i->second))
		{
			break;
		}
	}
}

template <typename T>
TransactionalVector<T>::TransactionalVector()
{
	// Initialize our internal segmented array.
	array = new SegmentedVector<Page<T, T, SGMT_SIZE> *>();
	// Allocate a size descriptor.
	// Keep it seperated to avoid needless contention between it and low-indexed elements.
	// It also needs to hold a different type of element than the others, a size.
	size.store(NULL);

	// Allocate an end transaction, if it hasn't been already.
	if (endTransaction == NULL)
	{
		endTransaction = new Desc<T>(0, NULL);
		endTransaction->status.store(Desc<T>::TxStatus::committed);
	}

	// Create the pool of pages.
	DeltaPage<T, T, SGMT_SIZE, 1>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 2>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 3>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 4>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 5>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 6>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 7>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 8>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 9>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 10>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 11>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 12>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 13>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 14>::initPool();
	DeltaPage<T, T, SGMT_SIZE, 15>::initPool();
	DeltaPage<T, T, SGMT_SIZE, SGMT_SIZE>::initPool();

	// Allocate an end page, if it hasn't been already.
	if (endPage == NULL)
	{
		endPage = new DeltaPage<T, T, SGMT_SIZE, SGMT_SIZE>;
		endPage->bitset.read.set();
		endPage->bitset.write.set();
		endPage->bitset.checkBounds.reset();
		endPage->transaction = endTransaction;
		endPage->next = NULL;
	}

	// Initialize the first size page.
	Page<size_t, T, 1> *sizePage = new DeltaPage<size_t, T, 1, 1>;
	// To ensure we never try to go past the initial size page, claim all values have been set here.
	sizePage->bitset.read.set();
	sizePage->bitset.write.set();
	sizePage->bitset.checkBounds.reset();
	// There is initially nothing in the vector. Size is 0.
	sizePage->set(0, NEW_VAL, 0);
	sizePage->set(0, OLD_VAL, 0);
	// Just point to the generic end transaction to show this operation is committed.
	sizePage->transaction = endTransaction;
	size.store(sizePage);
}

/*
TODO: Helping scheme plan.
Help on size conflicts:
Set pointer should be stored atomically in transaction descriptor.
Help on partial insertion:
If the conflict isn't on size, then we can assume the transaction already has pages associated with it, so just use those.
Run insertPages, starting at some specific page number.
*/

template <typename T>
RWSet<T> *TransactionalVector<T>::prepareTransaction(Desc<T> *descriptor)
{
	// Initialize the RWSet object.
	RWSet<T> *set = new RWSet<T>();

	// Create the read/write set.
	// TODO: Getting size may happen here. How do we deal with this for the helping scheme?
	set->createSet(descriptor, this);

	// Ensure that we can fit all of the segments we plan to insert.
	reserve(set->maxReserveAbsolute > set->size ? set->maxReserveAbsolute : set->size);

	// Convert the set into pages.
	// TODO: Share these pages somewhere to enable helping scheme. Perhaps in the descriptor.
	set->setToPages(descriptor);

	return set;
}

template <typename T>
bool TransactionalVector<T>::completeTransaction(Desc<T> *descriptor, size_t startPage, bool helping)
{
	// Insert the pages.
	insertPages(*descriptor->pages.load(), startPage);

	auto active = Desc<T>::TxStatus::active;
	auto committed = Desc<T>::TxStatus::committed;
	// Commit the transaction.
	// If this fails, either we aborted or some other transaction committed, so no need to retry.
	if (!descriptor->status.compare_exchange_strong(active, committed))
	{
		// At this point, either we committed, or some other thread committed or aborted the transaction.

		// If the transaction aborted.
		if (descriptor->status.load() != Desc<T>::TxStatus::committed)
		{
			// Return immediately.
			return false;
		}
	}
	// Commit succeeded.
	return true;
}

template <typename T>
void TransactionalVector<T>::executeTransaction(Desc<T> *descriptor)
{
	// Initialize the set for the descriptor.
	RWSet<T> *set = prepareTransaction(descriptor);

	// If the transaction committed.
	if (completeTransaction(descriptor, 0))
	{
		// Set values for all operations that wanted to read from shared memory.
		set->setOperationVals(descriptor, *descriptor->pages.load());
	}
}

template <typename T>
void TransactionalVector<T>::printContents()
{
	for (size_t i = 0;; i++)
	{
		Page<T, T, SGMT_SIZE> *rootPage = NULL;
		if (!array->read(i, rootPage))
		{
			break;
		}
		if (rootPage == NULL)
		{
			break;
		}
		// Initialize the current page along the linked list of updates.
		Page<T, T, SGMT_SIZE> *currentPage = rootPage;
		T oldElements[Page<T, T, SGMT_SIZE>::SEG_SIZE];
		T newElements[Page<T, T, SGMT_SIZE>::SEG_SIZE];
		bool newElement = false;
		std::bitset<Page<T, T, SGMT_SIZE>::SEG_SIZE> targetBits;
		targetBits.set();
		// Traverse down the existing delta updates, collecting old values as we go.
		// We stop when we no longer need any more elements or when there are no pages left.
		while (!targetBits.none())
		{
			// If we reach the end before identifying all values, use a generic initializer page instead.
			if (currentPage == NULL)
			{
				currentPage = endPage;
			}
			// Get the set of elements the current page has that we need.
			std::bitset<Page<T, T, SGMT_SIZE>::SEG_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
			// If this page has said elements.
			if (!posessedBits.none())
			{
				// Check the status of the transaction.
				typename Desc<T>::TxStatus status = currentPage->transaction->status.load();
				// If the current page is part of an active transaction.
				if (status == Desc<T>::TxStatus::active)
				{
					// Busy wait for the transaction to complete.
					while (currentPage->transaction->status.load() == Desc<T>::TxStatus::active)
					{
						continue;
					}
				}
				// Go through the bits.
				for (size_t j = 0; j < Page<T, T, SGMT_SIZE>::SEG_SIZE; j++)
				{
					// We only care about the possessed bits.
					if (posessedBits[j])
					{
						currentPage->get(j, OLD_VAL, oldElements[j]);
						currentPage->get(j, NEW_VAL, newElements[j]);

						// We only get the new value if it was write comitted.
						if (status == Desc<T>::TxStatus::committed && currentPage->bitset.write[j])
						{
							newElement = true;
						}
						// Transaction was aborted or operation was a read.
						// Grab the old page's old value.
						else
						{
							newElement = false;
						}
					}
				}
			}
			// Update our set of target bits.
			// We don't care about the elements we've just found anymore.
			targetBits &= posessedBits.flip();
			// Move on to the next delta update.
			currentPage = currentPage->next;
		}
		for (size_t j = 0; j < Page<T, T, SGMT_SIZE>::SEG_SIZE; j++)
		{
			printf("%ld:\t%d\t%d\n", i * Page<T, T, SGMT_SIZE>::SEG_SIZE + j, oldElements[j], newElements[j]);
		}
	}
	printf("\n");
	return;
}

template class TransactionalVector<VAL>;
template class SegmentedVector<Page<size_t, VAL, 1> *>;
template class SegmentedVector<Page<VAL, VAL, SGMT_SIZE> *>;