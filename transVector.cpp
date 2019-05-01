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
		// Get the head of the list of updates for this segment.
		if (!array->read(index, rootPage))
		{
			// Failure means the transaction attempted an invalid read or write, as the vector wasn't allocated to this point.
			page->transaction->status.store(Desc<T>::TxStatus::aborted);
			//printf("Aborted!\n");
			// No need to even try anymore. The whole transaction failed.
			return false;
		}

		// Quit if the transaction is no longer active.
		if (page->transaction->status.load() != Desc<T>::TxStatus::active)
		{
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
					// Help the active transaction.
					// TODO: make sure this works as expected.
					while (currentPage->transaction->status.load() == Desc<T>::TxStatus::active)
					{
						//completeTransaction(currentPage->transaction, true, index);
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
							// DEBUG:
							//page->print();

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
void TransactionalVector<T>::insertPages(std::map<size_t, Page<T, T, SGMT_SIZE> *> pages, bool helping, size_t startPage)
{
	// Get the start of the map.
	typename std::map<size_t, Page<T, T, SGMT_SIZE> *>::reverse_iterator iter = pages.rbegin();
	// Advance to a specific index, if specified.
	if (startPage < SIZE_MAX)
	{
		// Attempt to find a page at the target index.
		auto foundIter = pages.find(startPage);
		// If we found it.
		if (foundIter != pages.end())
		{
			// Reposition our iterator.
			iter = make_reverse_iterator(foundIter);
		}
		// If the page is invalid, which should never happen.
		else
		{
			printf("Specified page %lu does not exist. Starting from the beginning.\n", startPage);
		}
	}
	for (auto i = iter; i != pages.rend(); ++i)
	{
		// If some prepend produced a failure, don't insert any more pages for this transaction.
		if (i->second->transaction->status.load() == Desc<T>::TxStatus::aborted)
		{
			break;
		}

		size_t index = i->first;
		Page<T, T, SGMT_SIZE> *page;
		if (!helping)
		{
			page = i->second;
		}
		// If we are helping, get a same-sized page from our allocator and copy over the relevant data.
		else
		{
			size_t size = i->second->getSize();
			// Get a page from the allocator.
			// TODO: Make sure size returns DeltaPage size instead of full Page size.
			page = Allocator<T, T, SGMT_SIZE>::getNewPage(size);
			// Copy the page over.
			DeltaPage<T, T, SGMT_SIZE, 1> *deltaPage1 = (DeltaPage<T, T, SGMT_SIZE, 1> *)page;
			DeltaPage<T, T, SGMT_SIZE, 2> *deltaPage2 = (DeltaPage<T, T, SGMT_SIZE, 2> *)page;
			DeltaPage<T, T, SGMT_SIZE, 3> *deltaPage3 = (DeltaPage<T, T, SGMT_SIZE, 3> *)page;
			DeltaPage<T, T, SGMT_SIZE, 4> *deltaPage4 = (DeltaPage<T, T, SGMT_SIZE, 4> *)page;
			DeltaPage<T, T, SGMT_SIZE, 5> *deltaPage5 = (DeltaPage<T, T, SGMT_SIZE, 5> *)page;
			DeltaPage<T, T, SGMT_SIZE, 6> *deltaPage6 = (DeltaPage<T, T, SGMT_SIZE, 6> *)page;
			DeltaPage<T, T, SGMT_SIZE, 7> *deltaPage7 = (DeltaPage<T, T, SGMT_SIZE, 7> *)page;
			DeltaPage<T, T, SGMT_SIZE, 9> *deltaPage9 = (DeltaPage<T, T, SGMT_SIZE, 9> *)page;
			DeltaPage<T, T, SGMT_SIZE, 8> *deltaPage8 = (DeltaPage<T, T, SGMT_SIZE, 8> *)page;
			DeltaPage<T, T, SGMT_SIZE, 10> *deltaPage10 = (DeltaPage<T, T, SGMT_SIZE, 10> *)page;
			DeltaPage<T, T, SGMT_SIZE, 11> *deltaPage11 = (DeltaPage<T, T, SGMT_SIZE, 11> *)page;
			DeltaPage<T, T, SGMT_SIZE, 12> *deltaPage12 = (DeltaPage<T, T, SGMT_SIZE, 12> *)page;
			DeltaPage<T, T, SGMT_SIZE, 13> *deltaPage13 = (DeltaPage<T, T, SGMT_SIZE, 13> *)page;
			DeltaPage<T, T, SGMT_SIZE, 14> *deltaPage14 = (DeltaPage<T, T, SGMT_SIZE, 14> *)page;
			DeltaPage<T, T, SGMT_SIZE, 15> *deltaPage15 = (DeltaPage<T, T, SGMT_SIZE, 15> *)page;
			DeltaPage<T, T, SGMT_SIZE, 16> *deltaPage16 = (DeltaPage<T, T, SGMT_SIZE, 16> *)page;
			DeltaPage<T, T, SGMT_SIZE, SGMT_SIZE> *deltaPage = (DeltaPage<T, T, SGMT_SIZE, SGMT_SIZE> *)page;
			switch (size)
			{
			case 1:
				deltaPage1->copyFrom((DeltaPage<T, T, SGMT_SIZE, 1> *)i->second);
				break;
			case 2:
				deltaPage2->copyFrom((DeltaPage<T, T, SGMT_SIZE, 2> *)i->second);
				break;
			case 3:
				deltaPage3->copyFrom((DeltaPage<T, T, SGMT_SIZE, 3> *)i->second);
				break;
			case 4:
				deltaPage4->copyFrom((DeltaPage<T, T, SGMT_SIZE, 4> *)i->second);
				break;
			case 5:
				deltaPage5->copyFrom((DeltaPage<T, T, SGMT_SIZE, 5> *)i->second);
				break;
			case 6:
				deltaPage6->copyFrom((DeltaPage<T, T, SGMT_SIZE, 6> *)i->second);
				break;
			case 7:
				deltaPage7->copyFrom((DeltaPage<T, T, SGMT_SIZE, 7> *)i->second);
				break;
			case 8:
				deltaPage8->copyFrom((DeltaPage<T, T, SGMT_SIZE, 8> *)i->second);
				break;
			case 9:
				deltaPage9->copyFrom((DeltaPage<T, T, SGMT_SIZE, 9> *)i->second);
				break;
			case 10:
				deltaPage10->copyFrom((DeltaPage<T, T, SGMT_SIZE, 10> *)i->second);
				break;
			case 11:
				deltaPage11->copyFrom((DeltaPage<T, T, SGMT_SIZE, 11> *)i->second);
				break;
			case 12:
				deltaPage12->copyFrom((DeltaPage<T, T, SGMT_SIZE, 12> *)i->second);
				break;
			case 13:
				deltaPage13->copyFrom((DeltaPage<T, T, SGMT_SIZE, 13> *)i->second);
				break;
			case 14:
				deltaPage14->copyFrom((DeltaPage<T, T, SGMT_SIZE, 14> *)i->second);
				break;
			case 15:
				deltaPage15->copyFrom((DeltaPage<T, T, SGMT_SIZE, 15> *)i->second);
				break;
			case 16:
				deltaPage16->copyFrom((DeltaPage<T, T, SGMT_SIZE, 16> *)i->second);
				break;
			default:
				printf("Invalid page size specified (%lu). Assuming size=%d\n", size, SGMT_SIZE);
				deltaPage->copyFrom((DeltaPage<T, T, SGMT_SIZE, SGMT_SIZE> *)i->second);
				break;
			}
		}

		// Prepend the page.
		// Returns false if it caused an abort.
		if (!prependPage(index, page))
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
	Allocator<T, T, SGMT_SIZE>::initPool();

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

template <typename T>
void TransactionalVector<T>::prepareTransaction(Desc<T> *descriptor)
{
	RWSet<T> *set = descriptor->set.load();
	if (set == NULL)
	{
		// Initialize the RWSet object.
		set = new RWSet<T>();

		// Create the read/write set.
		// NOTE: Getting size may happen here.
		// Requires special consideration for the helping scheme.
		// This will work because helpers will either insert size or find it already there.
		set->createSet(descriptor, this);

		RWSet<T> *nullVal = NULL;
		descriptor->set.compare_exchange_strong(nullVal, set);
	}
	// Make sure we only work with the set that succeeded first.
	set = descriptor->set.load();

	// Ensure that we can fit all of the segments we plan to insert.
	reserve(set->maxReserveAbsolute > set->size ? set->maxReserveAbsolute : set->size);

	if (descriptor->pages.load() == NULL)
	{
		// Convert the set into pages.
		set->setToPages(descriptor);
	}

	return;
}

template <typename T>
bool TransactionalVector<T>::completeTransaction(Desc<T> *descriptor, bool helping, size_t startPage)
{
	// Insert the pages.
	insertPages(*descriptor->pages.load(), helping, startPage);

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
	prepareTransaction(descriptor);

	// If the transaction committed.
	if (completeTransaction(descriptor))
	{
		// Set values for all operations that wanted to read from shared memory.
		// This only occurs on one thread. Helpers don't bother with this.
		descriptor->set.load()->setOperationVals(descriptor, *descriptor->pages.load());
	}
}

template <typename T>
void TransactionalVector<T>::sizeHelp(Desc<T> *descriptor)
{
	// Must actually start at the very beginning.
	prepareTransaction(descriptor);
	// Must help from the beginning of the list, since we didn't help part way through.
	completeTransaction(descriptor, true, SIZE_MAX);
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