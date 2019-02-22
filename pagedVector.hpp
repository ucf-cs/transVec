/*
This vector builds off of the segmented vector, adding support for transactions, push, pop, read, and write.
It does this by utilizing delta update pages.
*/
#ifndef PAGEDVECTOR_H
#define PAGEDVECTOR_H

#include <atomic>
#include <bitset>
#include <cstddef>
#include <map>
#include <vector>
#include "deltaPage.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

template <class T>
class PagedVector
{
  private:
    // A page holding our shared size variable.
    std::atomic<Page<size_t> *> size = NULL;
    // An array of page pointers.
    SegmentedVector<Page<T> *> *array;
    // A generic ending page, used to get values.
    static Page<T> *endPage = NULL;
    // A generic committed transaction.
    static Desc<T> *endTransaction = NULL;

  public:
    PagedVector()
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
            for (size_t i = 0; i < endPage->size; i++)
            {
                endPage->newVal[i] = UNSET;
                endPage->oldVal[i] = UNSET;
            }
            endPage->bitset.read.set();
            endPage->bitset.write.set();
            endPage->bitset.checkBounds.reset();
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

    bool reserve(size_t size)
    {
        // Since we hold multiple elements per page, convert from a request for more elements to a request for more pages.
        // +1 to hold size.
        // /SEG_SIZE to account for each segment holding multiple elements.
        array->reserve((size + 1) / Page<T>::SEG_SIZE);
    }

    // Creates and prepends a special delta update for size.
    // Note that we need to do this whether we just read or also write to page, to prevent conflicts.
    Page<size_t> *getSizePage(Desc<T> *transaction)
    {
        // Create a new size page.
        Page<size_t> *newPage = new Page<size_t>(1);
        // We will read and write to all bits for this page.
        newPage->bitset.read.set();
        newPage->bitset.write.set();
        // This is passed in by the calling transaction.
        newPage->transaction = transaction;
        // This new page will store the old size at oldVal[0].
        prependPage(SIZE_MAX, newPage);
        // Size is a special case. We don't keep a linkedlist of changes.
        Page<size_t> *oldPage = newPage->next;
        newPage->next = NULL;
        delete oldPage;
        // We return a reference to the new page, so the caller can get oldVal and later set newVal.
        return newPage;
    }
    // Tells us that we need to use size for a transaction, if we aren't already.
    // Stores values for size and sizeDesc.
    size_t getSize(Desc<T> *descriptor, RWSet *set)
    {
        if (set->sizeDesc == NULL)
        {
            // Get a new vector size descriptor. Note that this associates size with this active transaction, leading to contention.
            set->sizeDesc = getSizePage(descriptor);
        }
        // Size is always at index 0, in our implementation.
        set->size = set->sizeDesc->oldVal[0];
        return size;
    }
    // Assign the final size value.
    // Must always be called after getSizePage().
    // If we called getSizePage(), we cannot commit until this step has completed.
    bool setSize(Page<size_t> *sizePage, size_t newSize)
    {
        // If these aren't true, something really bad and unexpected happened.
        if (sizePage == NULL || sizePage->transaction->status != Desc<T>::TxStatus::active)
        {
            return false;
        }
        // Set the actual value.
        sizePage->newVal[0] = newSize;
        return true;
    }

    // Prepends a delta update on an existing page.
    // Only sets oldVal values and the next pointer here.
    void prependPage(size_t index, Page<T> *page)
    {
        // Size is a special case, always stored at index 0.
        if (index == SIZE_MAX)
        {
            index = 0;
        }
        // Offset all other operations by 1.
        else
        {
            index++;
        }

        /*
        // Attempt to create an initial page.
        // We now have a "logical" page defined at NULL, so we don't need to physically allocate this at all.
        Page<T> *initialPage = new Page<T>();
        // If we fail, it just means someone else initialized for us.
        if (!array->tryWrite(index, NULL, initialPage))
        {
            delete initialPage;
        }
        */

        Page<T> *oldPage = NULL;
        do
        {
            // Get the head of the list of updates for this segment.
            oldPage = array->read(index);

            // Create a bitset to keep track of the updates of interest.
            std::bitset<Page<T>::SEG_SIZE> targetBits;
            // Set all bits we want to read or write.
            targetBits = page->bitset.read | page->bitset.write;

            // Initialize the current page along the linked list of updates.
            Page<T> *currentPage = oldPage;
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
                            // We only get the new value if it was read comitted.
                            if (status == Desc<T>::TxStatus::committed && currentPage->bitset.write[i])
                            {
                                *(page->at(i, OLD_VALS)) = *(currentPage->at(i, NEW_VALS));
                            }
                            // Transaction was aborted, grab the old page's old value.
                            else
                            {
                                *(page->at(i, OLD_VALS)) = *(currentPage->at(i, OLD_VALS));
                            }
                            // Abort if the operation fails our bounds check.
                            if (page->bitset.checkBounds[i] && *(page->at(i, OLD_VALS)) == UNSET)
                            {
                                page->transaction->status.store(Desc<T>::TxStatus::aborted);
                            }
                            // TODO: Store new values in ret.
                        }
                    }
                }
                // Update our set of target bits.
                // We don't care about the elements we've already found.
                targetBits &= posessedBits.flip();
                // Move on to the next delta update.
                currentPage = currentPage->next;
            }

            // Link our new page to the lastest page.
            page->next = oldPage;
        }
        // Insert the page into the desired location.
        // Retry on failure.
        // Finish either on success or when the whole transaction has aborted.
        while (!array->tryWrite(index, oldPage, page) && page->transaction->status.load() != Desc<T>::TxStatus::aborted);

        return;
    }

    // Takes in a set of pages and inserts them into our vector.
    // startPage is used in the helping scheme to start inserting at later pages.
    void insertPages(std::map<size_t, Page<T> *> pages, size_t startPage = 0)
    {
        for (auto i = pages.begin() + startPage; i <= pages.end(); ++i)
        {
            // If some prepend produced a failure, don't insert any more pages for this transaction.
            if (pages->second->transaction->status.load() == Desc<T>::TxStatus::aborted)
            {
                break;
            }
            prependPage(pages->first, pages->second);
        }
    }

    // Apply a transaction to a vector.
    void executeTransaction(Desc<T> *descriptor)
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
        std::map<size_t, Page<T> *> pages = set->setToPages();

        // If our size changed.
        // Once we set size, there is no turning back. The transaction is now touching shared memory.
        if (set->sizeDesc != NULL)
        {
            // Now that we know the ending size, set it.
            setSize(set->sizeDesc, set->size);
        }

        // Insert the pages.
        insertPages(pages);

        // Commit the transaction.
        // If this fails, either we aborted or some other transaction committed, so no need to retry.
        descriptor->status.compare_exchange_strong(Desc<T>::TxStatus::active, Desc<T>::TxStatus::committed);

        return;
    }

    // All transactions are converted into a read/write set before modifying the vector.
    class RWSet
    {
      public:
        // An individual operation on a single element location.
        struct RWOperation
        {
            // Set to false only if we know the current size of our vector.
            // True means we need to check unset.
            bool checkBounds = false;

            // Keep track of the last write operation to handle internally matching pops and reads from this location.
            // If this isn't NULL, we can infer a write for our page's bitset.
            Operation<T> *lastWriteOp = NULL;
            // Keep a list of operations that want to read the old value.
            // If this isn't empty, we can infer a read for our page's bitset.
            std::vector<Operation<T> *> readList;
        };

        // Map vector locations to read/write operations.
        // Used for absolute reads/writes.
        std::map<size_t, std::map<size_t, RWOperation>> operations;
        // An absolute reserve position.
        size_t maxReserveAbsolute = 0;
        // Our size descriptor. After reading size, we use this to write a new size value later.
        Page<size_t> *sizeDesc = NULL;
        // Set this if size changes.
        size_t size;

        // Map vector operations to pushes and pops relative to size.
        // Read size, then they can be resolved to absolute indexes.
        //RWOperation *operationsList;
        // A relative reserve position.
        //signed long int maxReserveRelative = 0;

        ~RWSet()
        {
            operations.clear();
            return;
        }

        // Return the indexes associated with a RW operation access.
        // Note that page insertions will internally offset by 1 later.
        std::pair<size_t, size_t> access(size_t pos)
        {
            // Special case for size pages.
            if (pos == SIZE_MAX)
            {
                return std::make_pair(SIZE_MAX, 0);
            }
            // All other cases.
            return std::make_pair(pos / Page<T>::SEG_SIZE, pos % Page<T>::SEG_SIZE);
        }

        // Converts a transaction descriptor into a read/write set.
        void createSet(Desc<T> *descriptor)
        {
            // Go through each operation.
            for (size_t i = 0; i < descriptor->size; i++)
            {
                std::pair<size_t, size_t> indexes;
                RWOperation *op = NULL;
                switch (descriptor->ops[i].type)
                {
                case Operation<T>::OpType::read:
                    indexes = access(descriptor->ops[i].index);
                    op = operations[indexes.first][indexes.second];
                    // If this location has already been written to, read its value. This is done to handle operations that are totally internal to the transaction.
                    if (op->lastWriteOp != NULL)
                    {
                        descriptor->ops[i].ret = op->lastWriteOp->val;
                        // If the value was unset (shouldn't really be possible here), then our transaction fails.
                        if (op->lastWriteOp->val == UNSET)
                        {
                            descriptor->status = Desc<T>::TxStatus::aborted;
                            return;
                        }
                    }
                    // We haven't written here before. Request a read from the shared structure.
                    else
                    {
                        op->checkBounds = true;
                        // Add ourselves to the read list.
                        op->readList.push_back(descriptor->ops[i]);
                    }
                    break;
                case Operation<T>::OpType::write:
                    indexes = access(descriptor->ops[i].index);
                    op = operations[indexes.first][indexes.second];
                    op->checkBounds = true;
                    op->lastWriteOp = descriptor->ops[i];
                    break;
                case Operation<T>::OpType::pushBack:
                    getSize(descriptor);

                    indexes = access(size++);
                    op = operations[indexes.first][indexes.second];
                    op->checkBounds = false;
                    op->lastWriteOp = descriptor->ops[i];
                    break;
                case Operation<T>::OpType::popBack:
                    getSize(descriptor);

                    indexes = access(--size);
                    op = operations[indexes.first][indexes.second];
                    // If this location has already been written to, read its value. This is done to handle operations that are totally internal to the transaction.
                    if (op->lastWriteOp != NULL)
                    {
                        descriptor->ops[i].ret = op->lastWriteOp->val;
                    }
                    // We haven't written here before. Request a read from the shared structure.
                    else
                    {
                        // Add ourselves to the read list.
                        op->readList.push_back(descriptor->ops[i]);
                    }
                    // We actually write an unset value here when we pop.
                    // Make sure we explicitly mark as UNSET.
                    // Don't leave this in the hands of the person creating the transactions.
                    descriptor->ops[i].val = UNSET;
                    op->checkBounds = false;
                    op->lastWriteOp = descriptor->ops[i];
                    break;
                case Operation<T>::OpType::size:
                    getSize(descriptor);

                    // Note: Don't store in ret. Store in index, as a special case for size calls.
                    descriptor->ops[i].index = size;
                    break;
                case Operation<T>::OpType::reserve:
                    // We only care about the largest reserve call.
                    // All other reserve operations will consolidate into a single call at the beginning of the transaction.
                    if (descriptor->ops[i].val > maxReserveAbsolute)
                    {
                        maxReserveAbsolute = descriptor->ops[i].val;
                    }
                    break;
                default:
                    // Unexpected operation type found.
                    break;
                }
            }
            return;
        }

        // Convert from a set of reads and writes to a list of pages.
        std::map<size_t, Page<T> *> setToPages(Desc<T> *descriptor)
        {
            // All of the pages we want to insert (except size), ordered from low to high.
            std::map<size_t, Page<T> *> pages;

            // For each page to generate.
            // These are all independent of shared memory.
            for (auto i = operations.begin(); i != operations.end(); ++i)
            {
                // Determine how many elements are in this page.
                size_t elementCount = i->second.size();
                // Create the initial page.
                Page<T> *page = new Page<T>(elementCount);
                // Link the page to the transaction descriptor.
                page->transaction = descriptor;

                // For each element to place in the given page.
                for (auto j = i->second.begin(); j != i->second.end(); ++j)
                {
                    size_t index = j - i->second.begin();
                    page->newVal[index] = j->second->lastWriteOp->val;
                    page->bitset.read[index] = (j->second->readList.size() > 0);
                    page->bitset.write[index] = (j->second->lastWriteOp != NULL);
                    page->bitset.checkBounds[index] = j->second->checkBounds;
                }
                pages[i->first] = page;
            }

            return pages;
        }
    };
};

#endif