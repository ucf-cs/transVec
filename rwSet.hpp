#ifndef RWSET_H
#define RWSET_H

#include <atomic>
#include <cstddef>
#include <map>
#include <vector>
#include "deltaPage.hpp"
#include "transaction.hpp"

// All transactions are converted into a read/write set before modifying the vector.
template <class T>
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

#endif