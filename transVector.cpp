#include "transVector.hpp"

#ifdef SEGMENTVEC

bool TransactionalVector::reserve(size_t size)
{
	// Since we hold multiple elements per page, convert from a request for more elements to a request for more pages.

	// Reserve a page per SEG_SIZE elements.
	size_t reserveSize = size / Page<VAL, SGMT_SIZE>::SEG_SIZE;
	// Since integer division always rounds down, add one more page to handle the remainder.
	if (size % Page<VAL, SGMT_SIZE>::SEG_SIZE != 0)
	{
		reserveSize++;
	}
	// Perform the actual reservation.
	return array->reserve(reserveSize);
}

bool TransactionalVector::prependPage(size_t index, Page<VAL, SGMT_SIZE> *page)
{
	assert(page != NULL && "Invalid page passed in.");

	// Create a bitset to keep track of all locations of interest.
	std::bitset<Page<VAL, SGMT_SIZE>::SEG_SIZE> targetBits;
	// Set all bits we want to read or write.
	targetBits = page->bitset.read | page->bitset.write;

	// The head of the linkedlist of updates.
	Page<VAL, SGMT_SIZE> *rootPage = NULL;
	// The previous head. The new page has already been updated to consider everything after this point.
	Page<VAL, SGMT_SIZE> *prevRoot = NULL;
	// Keep looping until page insertion suceeds or the transaction fails.
	while (true)
	{
		// Get the head of the list of updates for this segment.
		if (!array->read(index, rootPage))
		{
			// Failure means the transaction attempted an invalid read or write, as the vector wasn't allocated to this point.
			page->transaction->status.store(Desc::TxStatus::aborted);
			// DEBUG: Abort reporting.
			//printf("Aborted!\n");
			// No need to even try anymore. The whole transaction failed.
			return false;
		}

		// Quit if the transaction is no longer active.
		// Can happen if another thread helped the transaction complete.
		if (page->transaction->status.load() != Desc::TxStatus::active)
		{
			return false;
		}

		// Disallow prepending a redundant (duplicate) page.
		// May occur during helping.
		if (rootPage != NULL && rootPage->transaction == page->transaction)
		{
			// DEBUG: Duplicate page insertion catch.
			//printf("Attempted to prepend a page to itself.\n");

			// Insertion failed, but the transaction is incomplete, so keep trying.
			return true;
		}

		// Initialize the current page at the start of the linked list of updates.
		Page<VAL, SGMT_SIZE> *currentPage = rootPage;
		// Traverse down the existing delta updates, collecting old values as we go.
		// We stop when we have found all target elements or when there are no pages left.
		while (!targetBits.none())
		{
			// If we reach the end before identifying all values, use a generic initializer page instead.
			// In this page, all values are write UNSET, for proper abort handling.
			if (currentPage == NULL)
			{
				currentPage = endPage;
			}
			// If we reach a page that we already traversed in a previous loop, then there's no reason to traverse any further.
			if (currentPage == prevRoot)
			{
				break;
			}
			// Get the set of elements the current page has that we need.
			std::bitset<SGMT_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
			// If this page has said elements.
			if (!posessedBits.none())
			{
				// Check the status of the transaction.
				typename Desc::TxStatus status = currentPage->transaction->status.load();
				// No need to help reads, so check if there are any dependencies with writes.
				 // TODO: We must actually help reads!
				// If any of the posessed bits of the current page were writes.
				if ((posessedBits & currentPage->bitset.write) != 0)
				{
					// If the current page is part of an active transaction.
					if (status == Desc::TxStatus::active)
					{
						// Help the active transaction.
						while (currentPage->transaction->status.load() == Desc::TxStatus::active)
						{
#ifdef HELP
							completeTransaction(currentPage->transaction, true, index);
#endif
						}
						// Update our status to its final (committed or aborted) state.
						status = currentPage->transaction->status.load();
					}
				}
				// Go through the bits.
				for (size_t i = 0; i < Page<VAL, SGMT_SIZE>::SEG_SIZE; i++)
				{
					// We only care about the possessed bits.
					if (!posessedBits[i])
					{
						continue;
					}
					// Used to pass a value around by reference.
					VAL val = UNSET;
					// We only get the new value if it was write committed.
					if (status == Desc::TxStatus::committed && currentPage->bitset.write[i])
					{
						currentPage->get(i, NEW_VAL, val);
					}
					// Transaction was aborted or operation was a read.
					// Grab the old page's old value.
					else
					{
						currentPage->get(i, OLD_VAL, val);
					}
					// Set the old value for the page we want to insert, using the val pulled from the current page.
					page->set(i, OLD_VAL, val);

					// Abort if the operation fails our bounds check.
					if (page->bitset.checkBounds[i] && val == UNSET)
					{
						// DEBUG: Abort reporting.
						//printf("Aborted!\n");

						page->transaction->status.store(Desc::TxStatus::aborted);
						// No need to even try anymore. The whole transaction failed.
						return false;
					}
				}
			}
			// Update our set of target bits.
			// No longer look for elements associated with bits we've already found.
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
			// Keep track of the previous root to prevent redundant lookups.
			prevRoot = rootPage;
		}
	}

	// For each element in the page.
	for (size_t i = 0; i < page->SEG_SIZE; i++)
	{
		// If a read was not performed, skip it.
		if (!page->bitset.read[i])
		{
			continue;
		}
		// Get the RWOperation with the readList.
		RWOperation *op = NULL;
		page->transaction->set.load()->getOp(op, std::make_pair(index, i));
		// Get the old value from the page.
		VAL val = UNSET;
		page->get(i, OLD_VAL, val);
		// For each operation attempting to read the element.
		for (size_t j = 0; j < op->readList.size(); j++)
		{
			// Assign the return values.
			op->readList[j]->ret = val;
		}
	}

	return true;
}

void TransactionalVector::insertPages(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MemAllocator<std::pair<size_t, Page<VAL, SGMT_SIZE> *>>> *pages, bool helping, size_t startPage)
{
	assert(pages != NULL);
	// Get the start of the map.
	typename std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MemAllocator<std::pair<size_t, Page<VAL, SGMT_SIZE> *>>>::reverse_iterator iter = pages->rbegin();
	// Advance to a target index, if specified.
	if (startPage < SIZE_MAX)
	{
		// Attempt to find a page at the target index.
		auto foundIter = pages->find(startPage);
		// If we found it.
		if (foundIter != pages->end())
		{
			// Reposition our iterator.
			//iter = make_reverse_iterator(foundIter);
			iter = std::reverse_iterator<std::_Rb_tree_iterator<std::pair<const size_t, Page<VAL, SGMT_SIZE> *>>>(foundIter);
		}
		// If the page is invalid, which should never happen.
		else
		{
			printf("Specified page %lu does not exist. Starting from the beginning.\n", startPage);
		}
	}
	for (auto i = iter; i != pages->rend(); ++i)
	{
		// If some prepend produced a failure, don't insert any more pages for this transaction.
		if (i->second->transaction->status.load() == Desc::TxStatus::aborted)
		{
			break;
		}

		size_t index = i->first;
		Page<VAL, SGMT_SIZE> *page;
		if (!helping)
		{
			page = i->second;
		}
		// If we are helping, get a page from our allocator and copy over the relevant data.
		else
		{
			page = Allocator<Page<VAL, SGMT_SIZE>>::alloc();
			page->copyFrom(i->second);
		}

		// Prepend the page.
		// Returns false if the transaction is no longer active.
		if (!prependPage(index, page))
		{
			break;
		}
	}
	return;
}

TransactionalVector::TransactionalVector()
{
	// Initialize our internal segmented array.
	array = new SegmentedVector<Page<VAL, SGMT_SIZE> *>();
	// Allocate a size descriptor.
	// Keep it seperated to avoid needless contention between it and low-indexed elements.
	// It also needs to hold a different type of element than the others, a size.
	size.store(NULL);

	// Allocate an end transaction, if it hasn't been already.
	if (endTransaction == NULL)
	{
		endTransaction = new Desc(0, NULL);
		endTransaction->status.store(Desc::TxStatus::committed);
#ifdef HELP_FREE_READS
		size_t zero = 0;
		endTransaction->version.compare_exchange_strong(zero, globalVersionCounter.fetch_add(1));
#endif
	}

	// Allocate an end page, if it hasn't been already.
	if (endPage == NULL)
	{
		endPage = new Page<VAL, SGMT_SIZE>();
		endPage->bitset.read.set();
		endPage->bitset.write.set();
		endPage->bitset.checkBounds.reset();
		endPage->transaction = endTransaction;
		endPage->next = NULL;
	}

	// Initialize the first size page.
	Page<size_t, 1> *sizePage = new Page<size_t, 1>();
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

bool TransactionalVector::prepareTransaction(Desc *descriptor)
{
	RWSet *set = descriptor->set.load();
	if (set == NULL)
	{
		// Initialize the RWSet object.
		set = Allocator<RWSet>::alloc();

		// Create the read/write set.
		// NOTE: Getting size may happen here.
		// Requires special consideration for the helping scheme.
		// This will work because helpers will either insert size or find it already there.
		set->createSet(descriptor, this);

		RWSet *nullVal = NULL;
		descriptor->set.compare_exchange_strong(nullVal, set);
	}
	// Make sure we only work with the set that succeeded first.
	set = descriptor->set.load();

	// Ensure that we can fit all of the segments we plan to insert.
	if (!reserve(set->maxReserveAbsolute > set->size ? set->maxReserveAbsolute : set->size))
	{
		descriptor->status.store(Desc::TxStatus::aborted);
		return false;
	}

	if (descriptor->pages.load() == NULL)
	{
		// Convert the set into pages.
		set->setToPages(descriptor);
	}

	return true;
}

bool TransactionalVector::completeTransaction(Desc *descriptor, bool helping, size_t startPage)
{
	// Insert the pages.
	insertPages(descriptor->pages.load(), helping, startPage);

	auto active = Desc::TxStatus::active;
	auto committed = Desc::TxStatus::committed;
#ifdef HELP_FREE_READS
	// Always set the version number before committing.
	size_t zero = 0;
	descriptor->version.compare_exchange_strong(zero, globalVersionCounter.fetch_add(1));
#endif
	// Commit the transaction.
	// If this fails, either we aborted or some other transaction committed, so no need to retry.
	if (!descriptor->status.compare_exchange_strong(active, committed))
	{
		// At this point, either we committed, or some other thread committed or aborted the transaction.

		// If the transaction aborted.
		if (descriptor->status.load() != Desc::TxStatus::committed)
		{
			// Return immediately.
			return false;
		}
	}
	// Commit succeeded.
	return true;
}

#ifdef HELP_FREE_READS
void TransactionalVector::executeHelpFreeReads(Desc *descriptor)
{
	// Get the time now.
	// Any transactions after this will not be considered by these reads.
	size_t zero = 0;
	descriptor->version.compare_exchange_strong(zero, globalVersionCounter.fetch_add(1));

	// Validate that this is actually a purely help-free read transaction.
	// This step can be skipped for performance, but should happen for validation.
	// We start at 1 since executeTransaction already validated the operation at index 0.
	/*
	for (size_t i = 1; i < descriptor->size; i++)
	{
		assert(descriptor->ops[i].type == Operation::OpType::hfRead);
	}
	*/

	// The set of all transactions ignored by this help-free read transaction.
	// Used to interpret transactions consistently in a rare edge case.
	std::set<Desc *> ignoredTransactions;

	// Perform the reads.
	for (size_t i = 0; i < descriptor->size; i++)
	{
		// The head of the linkedlist of updates.
		Page<VAL, SGMT_SIZE> *rootPage = NULL;
		// Get the bucket and index of the read.
		std::pair<size_t, size_t> indexes = RWSet::access(descriptor->ops[i].index);

		// Get the root page.
		if (!array->read(indexes.first, rootPage))
		{
			// Failure means the transaction attempted an invalid read or write, as the vector wasn't allocated to this point.
			descriptor->status.store(Desc::TxStatus::aborted);
			// DEBUG: Abort reporting.
			//printf("Aborted!\n");

			// No need to even try anymore. The whole transaction failed.
			return;
		}

		// Initialize the current page at the start of the linked list of updates.
		Page<VAL, SGMT_SIZE> *currentPage = rootPage;
		bool traverse = true;
		while (traverse)
		{
			// If we reach the end before identifying a value, use a generic initializer page instead.
			// In this page, all values are write UNSET, for proper abort handling.
			if (currentPage == NULL)
			{
				currentPage = endPage;
			}

			// If the associated transaction is in the ignore list, ignore this page.
			if (ignoredTransactions.count(currentPage->transaction))
			{
				// Go to the next page.
				currentPage = currentPage->next;
				continue;
			}

			// If this page did not read or write to this location.
			if ((currentPage->bitset.read[indexes.second] || currentPage->bitset.write[indexes.second]) == 0)
			{
				// Go to the next page.
				currentPage = currentPage->next;
				continue;
			}

			// NOTE: Continue traversal only if the page is active or the timestamp is too late.
			// Otherwise, we have found a value to return.

			// If this page too new.
			if (currentPage->transaction->version.load() > descriptor->version.load())
			{
				// Go to the next page.
				currentPage = currentPage->next;
				continue;
			}

			// The element's logical status is based on the page's transaction status.
			switch (currentPage->transaction->status.load())
			{
			case Desc::TxStatus::active:
				// Add this transaction to the ignore list, so we can interpret its state consistently.
				ignoredTransactions.insert(currentPage->transaction);
				break;
			case Desc::TxStatus::committed:
				// If a write committed.
				if (currentPage->bitset.write[indexes.second] != 0)
				{
					currentPage->get(indexes.second, NEW_VAL, descriptor->ops[i].ret);
				}
				// If a read committed.
				else
				{
					currentPage->get(indexes.second, OLD_VAL, descriptor->ops[i].ret);
				}
				traverse = false;
				break;
			case Desc::TxStatus::aborted:
				currentPage->get(indexes.second, OLD_VAL, descriptor->ops[i].ret);
				traverse = false;
				break;
			default:
				assert(false);
				break;
			}

			// Go to the next page.
			currentPage = currentPage->next;
		}

		// Abort if an UNSET is read.
		if (descriptor->ops[i].ret == UNSET)
		{
			descriptor->status.store(Desc::TxStatus::aborted);
			return;
		}
	}
	// Complete the reads.
	descriptor->status.store(Desc::TxStatus::committed);
	return;
}
#endif

void TransactionalVector::executeTransaction(Desc *descriptor)
{
#ifdef HELP_FREE_READS
	// Determine if this is a help-free read transaction.
	if (descriptor->size > 0 && descriptor->ops[0].type == Operation::OpType::hfRead)
	{
		// Call the specialized transaction function to handle it.
		executeHelpFreeReads(descriptor);
		return;
	}
#endif
	// Initialize the set for the descriptor.
	prepareTransaction(descriptor);
	// If the transaction committed.
	completeTransaction(descriptor);
}

void TransactionalVector::sizeHelp(Desc *descriptor)
{
	// Must actually start at the very beginning.
	prepareTransaction(descriptor);
	// Must help from the beginning of the list, since we didn't help part way through.
	completeTransaction(descriptor, true);
}

void TransactionalVector::printContents()
{
	for (size_t i = 0;; i++)
	{
		Page<VAL, SGMT_SIZE> *rootPage = NULL;
		if (!array->read(i, rootPage))
		{
			break;
		}
		if (rootPage == NULL)
		{
			break;
		}
		// Initialize the current page along the linked list of updates.
		Page<VAL, SGMT_SIZE> *currentPage = rootPage;
		VAL oldElements[SGMT_SIZE];
		VAL newElements[SGMT_SIZE];
		std::bitset<SGMT_SIZE> targetBits;
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
			std::bitset<SGMT_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
			// If this page has said elements.
			if (!posessedBits.none())
			{
				// Check the status of the transaction.
				typename Desc::TxStatus status = currentPage->transaction->status.load();
				// If the current page is part of an active transaction.
				if (status == Desc::TxStatus::active)
				{
					// Busy wait for the transaction to complete.
					while (currentPage->transaction->status.load() == Desc::TxStatus::active)
					{
						continue;
					}
				}
				// Go through the bits.
				for (size_t j = 0; j < SGMT_SIZE; j++)
				{
					// We only care about the possessed bits.
					if (posessedBits[j])
					{
						currentPage->get(j, OLD_VAL, oldElements[j]);
						currentPage->get(j, NEW_VAL, newElements[j]);
					}
				}
			}
			// Update our set of target bits.
			// We don't care about the elements we've just found anymore.
			targetBits &= posessedBits.flip();
			// Move on to the next delta update.
			currentPage = currentPage->next;
		}
		for (size_t j = 0; j < SGMT_SIZE; j++)
		{
			std::cout << i * SGMT_SIZE + j << "\t"
					  << oldElements[j] << "\t"
					  << newElements[j] << std::endl;
		}
	}
	printf("\n");
	return;
}

#endif