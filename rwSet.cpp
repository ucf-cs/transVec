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
    return std::make_pair(pos / Page<T,T>::SEG_SIZE, pos % Page<T,T>::SEG_SIZE);
}

template <typename T>
bool RWSet<T>::createSet(Desc<T> *descriptor, TransactionalVector<T> *vector)
{
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
            // If this location has already been written to, read its value. This is done to handle operations that are totally internal to the transaction.
            if (op->lastWriteOp != NULL)
            {
                descriptor->ops[i].ret = op->lastWriteOp->val;
                // If the value was unset (shouldn't really be possible here), then our transaction fails.
                if (op->lastWriteOp->val == UNSET)
                {
                    descriptor->status = Desc<T>::TxStatus::aborted;
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
            vector->getSize(descriptor);

            indexes = access(size++);
            op = &operations[indexes.first][indexes.second];
            if (op->checkBounds == RWOperation<T>::Assigned::unset)
            {
                op->checkBounds = RWOperation<T>::Assigned::no;
            }
            op->lastWriteOp = &descriptor->ops[i];
            break;
        case Operation<T>::OpType::popBack:
            vector->getSize(descriptor);

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
            vector->getSize(descriptor);

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
    return true;
}

template <typename T>
std::map<size_t, Page<T, T> *> RWSet<T>::setToPages(Desc<T> *descriptor)
{
    // All of the pages we want to insert (except size), ordered from low to high.
    std::map<size_t, Page<T, T> *> pages;

    // For each page to generate.
    // These are all independent of shared memory.
    for (auto i = operations.begin(); i != operations.end(); ++i)
    {
        // Determine how many elements are in this page.
        size_t elementCount = i->second.size();
        // Create the initial page.
        Page<T, T> *page = new Page<T, T>(elementCount);
        // Link the page to the transaction descriptor.
        page->transaction = descriptor;

        // For each element to place in the given page.
        for (auto j = i->second.begin(); j != i->second.end(); ++j)
        {
            size_t index = j->first;
            *page->at(index, NEW_VAL) = j->second.lastWriteOp->val;
            page->bitset.read[index] = (j->second.readList.size() > 0);
            page->bitset.write[index] = (j->second.lastWriteOp != NULL);
            page->bitset.checkBounds[index] = (j->second.checkBounds == RWOperation<T>::Assigned::yes) ? true : false;
        }
        pages[i->first] = page;
    }

    return pages;
}

template class RWSet<int>;