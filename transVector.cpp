#include "transVector.hpp"

template <typename T>
bool TransactionalVector<T>::reserve(size_t size)
{
    // Since we hold multiple elements per page, convert from a request for more elements to a request for more pages.

    // Reserve a page per SEG_SIZE elements.
    size_t reserveSize = size / Page<T>::SEG_SIZE;
    // Since integer division always rounds down, add one more page to handle the remainder.
    if (size % Page<T>::SEG_SIZE != 0)
    {
        reserveSize++;
    }
    // Perform the actual reservation.
    return array->reserve(reserveSize);
}

template <typename T>
void TransactionalVector<T>::getSize(Desc<T> *descriptor) {
    // TODO: Implement this.
    return;
}

template <typename T>
bool TransactionalVector<T>::prependPage(size_t index, Page<T> *page)
{
    Page<T> *rootPage = NULL;
    do
    {

        // Quit if the transaction is no longer active.
        if (page->transaction->status.load() != Desc<T>::TxStatus::active)
        {
            return false;
        }

        // Get the head of the list of updates for this segment.
        rootPage = array->read(index);

        // Quit if some other thread already inserted a page for this transaction.
        // TODO: Is this realistically even reachable? May never happen.
        if (rootPage->transaction == page->transaction)
        {
            assert(false);
            return true;
        }

        // Create a bitset to keep track of all locations of interest.
        std::bitset<Page<T>::SEG_SIZE> targetBits;
        // Set all bits we want to read or write.
        targetBits = page->bitset.read | page->bitset.write;

        // Initialize the current page along the linked list of updates.
        Page<T> *currentPage = rootPage;
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
            std::bitset<Page<T>::SEG_SIZE> posessedBits = targetBits & (currentPage->bitset.read | currentPage->bitset.write);
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
                    while (status != Desc<T>::TxStatus::committed)
                    {
                        continue;
                    }
                }
                // Go through the bits.
                for (size_t i = 0; i < Page<T>::SEG_SIZE; i++)
                {
                    // We only care about the possessed bits.
                    if (posessedBits[i])
                    {
                        // We only get the new value if it was write comitted.
                        if (status == Desc<T>::TxStatus::committed && currentPage->bitset.write[i])
                        {
                            *(page->at(i, OLD_VAL)) = *(currentPage->at(i, NEW_VAL));
                        }
                        // Transaction was aborted or operation was a read.
                        // Grab the old page's old value.
                        else
                        {
                            *(page->at(i, OLD_VAL)) = *(currentPage->at(i, OLD_VAL));
                        }
                        // Abort if the operation fails our bounds check.
                        if (page->bitset.checkBounds[i] && *(page->at(i, OLD_VAL)) == UNSET)
                        {
                            page->transaction->status.store(Desc<T>::TxStatus::aborted);
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

        // Link our new page to the lastest page.
        page->next = rootPage;
    }
    // Insert the page into the desired location.
    // Finish on success.
    // Retry on failure.
    while (!array->tryWrite(index, rootPage, page));

    return true;
}

template <typename T>
void TransactionalVector<T>::insertPages(std::map<size_t, Page<T> *> pages, size_t startPage)
{
    // Get the start of the map.
    typename std::map<size_t, Page<T> *>::iterator iter = pages.begin();
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

/*
// TODO: Fix this.
template <typename T>
void TransactionalVector<T>::setOperationVals(Desc<T> *descriptor, std::map<size_t, Page<T> *> pages, std::map<size_t, std::map<size_t, typename RWSet<T>::RWOperation>> operations)
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
        Page<T> *page = i->second;
        // For each element.
        for (auto j = 0; j < pages.size(); ++j)
        {
            // Get the value.
            T value = *(page->at(j, OLD_VAL));
            // For each operation attempting to read the element.
            for (size_t k = 0; k < operations.at(i->first).at(j)->readList.size(); k++)
            {
                // Assign the return value.
                operations.at(i->first).at(j)->readList[k] = value;
            }
        }
    }

    // Subsequent attempts to load values will not have to repeat this work.
    descriptor->returnedValues.store(true);

    return;
}
*/

template <typename T>
TransactionalVector<T>::TransactionalVector()
{
    // Initialize our internal segmented array.
    array = new SegmentedVector<Page<T> *>();
    // Allocate a size descriptor.
    // Keep it seperated to avoid needless contention between it and low-indexed elements.
    // It also needs to hold a different type of element than the others, a size.
    size.store(new Page<size_t>(1));
    // Allocate an end page, if it hasn't been already.
    if (endPage == NULL)
    {
        endPage = new Page<T>();
        endPage->bitset.read.set();
        endPage->bitset.write.set();
        endPage->bitset.checkBounds.reset();
        for (size_t i = 0; i < endPage->size; i++)
        {
            *(endPage->at(i, NEW_VAL)) = UNSET;
            *(endPage->at(i, OLD_VAL)) = UNSET;
        }
        // Allocate an end transaction, if it hasn't been already.
        if (endTransaction == NULL)
        {
            endTransaction = new Desc<T>(0, NULL);
            endTransaction->status = Desc<T>::TxStatus::committed;
        }
        endPage->transaction = endTransaction;
        endPage->next = NULL;
    }
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
    std::map<size_t, Page<T> *> pages = set->setToPages(descriptor);

    // If our size changed.
    // Once we set size, there is no turning back. The transaction is now touching shared memory.
    if (set->sizeDesc != NULL)
    {
        // Now that we know the ending size, set it.
        // TODO: Implement this.
        //setSize(set->sizeDesc, set->size);
    }

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
    // TODO: Fix this function.
    //setOperationVals(descriptor, pages);

    return;
}

template class TransactionalVector<int>;
template class SegmentedVector<Page<int>*>;