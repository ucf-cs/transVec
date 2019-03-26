#include "transVector.hpp"

template <typename T>
bool TransactionalVector<T>::reserve(size_t size)
{
    // Since we hold multiple elements per page, convert from a request for more elements to a request for more pages.

    // Reserve a page per SEG_SIZE elements.
    size_t reserveSize = size / Page<T, T, 8>::SEG_SIZE;
    // Since integer division always rounds down, add one more page to handle the remainder.
    if (size % Page<T, T, 8>::SEG_SIZE != 0)
    {
        reserveSize++;
    }
    // Perform the actual reservation.
    return array->reserve(reserveSize);
}

template <typename T>
bool TransactionalVector<T>::prependPage(size_t index, Page<T, T, 8> *page)
{
    // Create a bitset to keep track of all locations of interest.
    std::bitset<Page<T, T, 8>::SEG_SIZE> targetBits;
    // Set all bits we want to read or write.
    targetBits = page->bitset.read | page->bitset.write;

    Page<T, T, 8> *rootPage = NULL;
    do
    {

        // Quit if the transaction is no longer active.
        if (page->transaction->status.load() != Desc<T>::TxStatus::active)
        {
            return false;
        }

        // Get the head of the list of updates for this segment.
        rootPage = array->read(index);

        // Disallow prepending a page to itself.
        // TODO: Can this happen?
        if (rootPage == page)
        {
            assert(false);
            return false;
        }

        // Initialize the current page along the linked list of updates.
        Page<T, T, 8> *currentPage = rootPage;
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
            std::bitset<Page<T, T, 8>::SEG_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
            // If this page has said elements.
            if (!posessedBits.none())
            {
                // Check the status of the transaction.
                typename Desc<T>::TxStatus status = currentPage->transaction->status.load();
                // If the current page is part of an active transaction.
                if (status == Desc<T>::TxStatus::active)
                {
                    // TODO: Help the active transaction.
                    // For now, just busy wait.
                    while (currentPage->transaction->status.load() == Desc<T>::TxStatus::active)
                    {
                        continue;
                    }
                }
                // Go through the bits.
                for (size_t i = 0; i < Page<T, T, 8>::SEG_SIZE; i++)
                {
                    // We only care about the possessed bits.
                    if (posessedBits[i])
                    {
                        // We only get the new value if it was write comitted.
                        if (status == Desc<T>::TxStatus::committed && currentPage->bitset.write[i])
                        {
                            page->set(i, OLD_VAL, currentPage->get(i, NEW_VAL));
                        }
                        // Transaction was aborted or operation was a read.
                        // Grab the old page's old value.
                        else
                        {
                            page->set(i, OLD_VAL, currentPage->get(i, OLD_VAL));
                        }
                        // Abort if the operation fails our bounds check.
                        if (page->bitset.checkBounds[i] && page->get(i, OLD_VAL) == UNSET)
                        {
                            page->transaction->status.store(Desc<T>::TxStatus::aborted);
                            printf("Aborted!\n");
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
    }
    // Insert the page into the desired location.
    // Finish on success.
    // Retry on failure.
    while (!array->tryWrite(index, rootPage, page));

    return true;
}

template <typename T>
void TransactionalVector<T>::insertPages(std::map<size_t, Page<T, T, 8> *> pages, size_t startPage)
{
    // Get the start of the map.
    typename std::map<size_t, Page<T, T, 8> *>::iterator iter = pages.begin();
    // Advance part-way through.
    if (startPage > 0)
    {
        std::advance(iter, startPage);
    }
    for (auto i = iter; i != pages.end(); ++i)
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
    array = new SegmentedVector<Page<T, T, 8> *>();
    // Allocate a size descriptor.
    // Keep it seperated to avoid needless contention between it and low-indexed elements.
    // It also needs to hold a different type of element than the others, a size.
    // TODO: Set the bitset for the size page, and potentially other elements.
	size.store(NULL);

    // Allocate an end transaction, if it hasn't been already.
    if (endTransaction == NULL)
    {
        endTransaction = new Desc<T>(0, NULL);
        endTransaction->status.store(Desc<T>::TxStatus::committed);
    }

    // Allocate an end page, if it hasn't been already.
    if (endPage == NULL)
    {
        endPage = new Page<T, T, 8>();
        endPage->bitset.read.set();
        endPage->bitset.write.set();
        endPage->bitset.checkBounds.reset();
        endPage->transaction = endTransaction;
        endPage->next = NULL;
    }

    // Initialize the first size page.
    Page<size_t, T, 1> *sizePage = new Page<size_t, T, 1>(1);
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
void TransactionalVector<T>::executeTransaction(Desc<T> *descriptor)
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
    std::map<size_t, Page<T, T, 8> *> pages = set->setToPages(descriptor);

    // Insert the pages.
    insertPages(pages);

    auto active = Desc<T>::TxStatus::active;
    auto committed = Desc<T>::TxStatus::committed;
    // Commit the transaction.
    // If this fails, either we aborted or some other transaction committed, so no need to retry.
    if (!descriptor->status.compare_exchange_strong(active, committed))
    {
        // If the transaction aborted.
        if (descriptor->status.load() != Desc<T>::TxStatus::committed)
        {
            // Return immediately.
            return;
        }
    }

    // Set values for all operations that wanted to read from shared memory.
    set->setOperationVals(descriptor, &pages);

    return;
}

template <typename T>
void TransactionalVector<T>::printContents()
{
    for (size_t i = 0;; i++)
    {
        Page<T, T, 8> *rootPage = array->read(i);
        if (rootPage == NULL)
        {
            break;
        }
        // Initialize the current page along the linked list of updates.
        Page<T, T, 8> *currentPage = rootPage;
        T oldElements[Page<T, T, 8>::SEG_SIZE];
        T newElements[Page<T, T, 8>::SEG_SIZE];
        bool newElement = false;
        std::bitset<Page<T, T, 8>::SEG_SIZE> targetBits;
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
            std::bitset<Page<T, T, 8>::SEG_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
            // If this page has said elements.
            if (!posessedBits.none())
            {
                // Check the status of the transaction.
                typename Desc<T>::TxStatus status = currentPage->transaction->status.load();
                // If the current page is part of an active transaction.
                if (status == Desc<T>::TxStatus::active)
                {
                    // TODO: Help the active transaction.
                    // For now, just busy wait.
                    while (currentPage->transaction->status.load() == Desc<T>::TxStatus::active)
                    {
                        continue;
                    }
                }
                // Go through the bits.
                for (size_t j = 0; j < Page<T, T, 8>::SEG_SIZE; j++)
                {
                    // We only care about the possessed bits.
                    if (posessedBits[j])
                    {
                        oldElements[j] = currentPage->get(j, OLD_VAL);
                        newElements[j] = currentPage->get(j, NEW_VAL);

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
        for (size_t j = 0; j < Page<T, T, 8>::SEG_SIZE; j++)
        {
            printf("%ld:\t%d\t%d\n", i * Page<T, T, 8>::SEG_SIZE + j, oldElements[j], newElements[j]);
        }
    }
    printf("\n");
    return;
}

template class TransactionalVector<int>;
template class SegmentedVector<Page<size_t, int, 1> *>;
template class SegmentedVector<Page<int, int, 8> *>;