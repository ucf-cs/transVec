#include "boostedVector.hpp"

#ifdef BOOSTEDVEC

void BoostedElement::print()
{
    printf("val: %du", val);
    return;
}

bool BoostedVector::reserve(size_t size)
{
    return array->reserve(size);
}

BoostedVector::BoostedVector()
{
    // Initialize our internal segmented array.
    array = new SegmentedVector<BoostedElement>();
    // Allocate an end transaction.
    if (endTransaction == NULL)
    {
        endTransaction = new Desc(0, NULL);
    }
    // Initialize size.
    sizeLock.lock.lock();
    size = 0;
    sizeLock.lock.unlock();
    return;
}

// TODO: Rework this.
bool BoostedVector::insertElements(Desc *descriptor)
{
    RWSet *set = descriptor->set;
    // Get the start of the map.
    typename std::map<size_t, RWOperation *, std::equal_to<size_t>, MemAllocator<std::pair<size_t, RWOperation *>>>::reverse_iterator iter = set->operations.rbegin();

    for (; iter != set->operations.rend(); ++iter)
    {
        BoostedElement *elem;
        if (!array->read(iter->first, elem))
        {
            return false;
        }
        // Lock the element.
        elem->lock.lock();
        // Push the lock to the list so we can unlock it when the transaction completes.
        descriptor->locks.push_back(elem);
        // If any reads are pending.
        if (!iter->second->readList.empty())
        {
            for (size_t i = 0; i < iter->second->readList.size(); i++)
            {
                iter->second->readList[i]->ret = elem->val;
            }
        }
        // If a write is pending.
        if (iter->second->lastWriteOp != NULL)
        {
            elem->val = iter->second->lastWriteOp->val;
        }
    }
    return true;
}

bool BoostedVector::prepareTransaction(Desc *descriptor)
{
    RWSet *set = descriptor->set;
    set = Allocator<RWSet>::alloc();
    // Create the read/write set.
    set->createSet(descriptor, this);
    // Ensure that we can fit all of the elements we plan to insert.
    return reserve(set->maxReserveAbsolute > set->size ? set->maxReserveAbsolute : set->size);
}

bool BoostedVector::executeTransaction(Desc *descriptor)
{
    return prepareTransaction(descriptor) && insertElements(descriptor);
    // As the transaction has completed, release all locks in the order obtained.
    for (size_t i = 0; i < descriptor->locks.size(); i++)
    {
        descriptor->locks[i]->lock.unlock();
    }
    return true;
}

void BoostedVector::printContents()
{
    for (size_t i = 0;; i++)
    {
        BoostedElement *elem;
        if (!array->read(i, elem))
        {
            break;
        }
        elem->print();
    }
    printf("\n");
    return;
}

#endif