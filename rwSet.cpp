#include "rwSet.hpp"

#if defined COMPACTVEC || defined BOOSTEDVEC

RWSet::~RWSet()
{
    operations.clear();
    return;
}

#ifdef COMPACTVEC
size_t RWSet::access(unsigned int pos)
{
    return pos;
}
#endif
#ifdef BOOSTEDVEC
size_t RWSet::access(size_t pos)
{
    return pos;
}
#endif

#ifdef COMPACTVEC
    bool RWSet::createSet(Desc *descriptor, CompactVector *vector)
#endif
#ifdef BOOSTEDVEC
        bool RWSet::createSet(Desc *descriptor, BoostedVector *vector)
#endif
{
#ifdef COMPACTVEC
    // Set the set's descriptor.
    this->descriptor = descriptor;
#endif
    // Go through each operation.
    for (size_t i = 0; i < descriptor->size; i++)
    {
#ifdef COMPACTVEC
        unsigned int indexes;
#endif
#ifdef BOOSTEDVEC
        size_t indexes;
#endif
        RWOperation *op = NULL;
        switch (descriptor->ops[i].type)
        {
        case Operation::OpType::read:
            indexes = access(descriptor->ops[i].index);
            getOp(op, indexes);
            // If this location has already been written to, read its value.
            // This is done to handle operations that are totally internal to the transaction.
            if (op->lastWriteOp != NULL)
            {
                descriptor->ops[i].ret = op->lastWriteOp->val;
                // If the value was unset (internal pop?), then our transaction fails.
                if (op->lastWriteOp->val == UNSET)
                {
#ifndef BOOSTEDVEC
                    descriptor->status.store(Desc::TxStatus::aborted);
                    // DEBUG: Abort reporting.
                    //fprintf(stderr, "Aborted!\n");
#endif
                    return false;
                }
            }
            // We haven't written here before. Request a read from the shared structure.
            else
            {
                if (op->checkBounds == RWOperation::Assigned::unset)
                {
                    op->checkBounds = RWOperation::Assigned::yes;
                }
                // Add ourselves to the read list.
                op->readList.push_back(&descriptor->ops[i]);
            }
            break;
        case Operation::OpType::write:
            indexes = access(descriptor->ops[i].index);
            getOp(op, indexes);
            // If this location has already been written to, read its value.
            // This is done to handle operations that are totally internal to the transaction.
            if (op->lastWriteOp != NULL)
            {
                // If the value was unset (internal pop?), then our transaction fails.
                if (op->lastWriteOp->val == UNSET)
                {
#ifndef BOOSTEDVEC
                    descriptor->status.store(Desc::TxStatus::aborted);
                    // DEBUG: Abort reporting.
                    //fprintf(stderr, "Aborted!\n");
#endif
                    return false;
                }
            }
            // We haven't written here before. Request a read from the shared structure.
            else
            {
                if (op->checkBounds == RWOperation::Assigned::unset)
                {
                    op->checkBounds = RWOperation::Assigned::yes;
                }
            }
            op->lastWriteOp = &descriptor->ops[i];
            break;
        case Operation::OpType::pushBack:
            getSize(vector, descriptor);
            // This should never happen, but make sure we don't have an integer overflow.
            if (size == std::numeric_limits<decltype(size)>::max())
            {
#ifndef BOOSTEDVEC
                descriptor->status.store(Desc::TxStatus::aborted);
                // DEBUG: Abort reporting.
                //fprintf(stderr, "Aborted!\n");
#endif
                return false;
            }
            indexes = access(size++);

            getOp(op, indexes);
            if (op->checkBounds == RWOperation::Assigned::unset)
            {
                op->checkBounds = RWOperation::Assigned::no;
            }
            op->lastWriteOp = &descriptor->ops[i];
            break;
        case Operation::OpType::popBack:
            getSize(vector, descriptor);
            // Prevent popping past the bottom of the stack.
            if (size < 1)
            {
#ifndef BOOSTEDVEC
                descriptor->status.store(Desc::TxStatus::aborted);
#endif
                // DEBUG: Abort reporting.
                //fprintf(stderr, "Aborted!\n");
                return false;
            }
            indexes = access(--size);

            getOp(op, indexes);
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
            if (op->checkBounds == RWOperation::Assigned::unset)
            {
                op->checkBounds = RWOperation::Assigned::no;
            }
            op->lastWriteOp = &descriptor->ops[i];
            break;
        case Operation::OpType::size:
            getSize(vector, descriptor);

            // NOTE: Don't store in ret. Store in index, as a special case for size calls.
            descriptor->ops[i].index = size;
            break;
        case Operation::OpType::reserve:
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
#ifdef COMPACTVEC
    // If size changed.
    if (sizeElement != NULL)
    {
        // Replace our size element in shared memory with a finalized size value.
        CompactElement newSizeElement;
        newSizeElement.newVal = size;
        newSizeElement.oldVal = sizeElement->oldVal;
        newSizeElement.descriptor = descriptor;
        // Attempt to update the vector's size to contain the new value.
        // If this CAS fails, then either the final size value was already set by a helper or a new transaction was associated with size and our transaction already completed long ago.
        vector->size.compare_exchange_strong(*sizeElement, newSizeElement);
    }
#endif
#ifdef BOOSTEDVEC
    if (hasSize)
    {
        vector->size = size;
        // DEBUG:
        //printf("to %lu\n", vector->size);
    }
#endif
    return true;
}

#ifdef COMPACTVEC
// Special way to retrieve the current size.
unsigned int RWSet::getSize(CompactVector *vector, Desc *descriptor)
{
    // If size has already been set.
    if (sizeElement != NULL)
    {
        // Just return that size.
        return size;
    }

    // Allocate the size element.
    sizeElement = Allocator<CompactElement>::alloc();

    CompactElement oldSizeElement;
    do
    {
    start:
        oldSizeElement = vector->size.load();

        // Quit if the transaction is no longer active.
        if (descriptor->status.load() != Desc::TxStatus::active)
        {
            return 0;
        }

        // If a helper got here first.
        if (oldSizeElement.descriptor == descriptor)
        {
            // We can just use the size of the existing element.
            size = oldSizeElement.oldVal;
            // Prepare sizeElement for a CAS later on.
            sizeElement->descriptor = descriptor;
            sizeElement->newVal = oldSizeElement.oldVal;
            sizeElement->oldVal = oldSizeElement.oldVal;
            // Do not insert again.
            return size;
        }

        if (oldSizeElement.descriptor == NULL)
        {
            sizeElement->descriptor = descriptor;
            sizeElement->newVal = 0;
            sizeElement->oldVal = 0;
        }
        else
        {
            Desc::TxStatus status = oldSizeElement.descriptor->status.load();
#ifdef HELP
            if (status == Desc::TxStatus::active)
            {
                // Help the active transaction.
                vector->sizeHelp(oldSizeElement.descriptor);
                // Start the loop over again.
                goto start;
            }
#else
            // Busy wait for the conflicting thread to complete.
            while (status == Desc::TxStatus::active)
            {
                status = oldSizeElement.descriptor->status.load();
            }
#endif

            // Create a new size element.
            // Set newVal so the whole object is known.
            // To keep things deterministic, newVal == oldVal when the final value has not been set.
            // This way, helpers can still perform a size CAS to complete the operation.
            sizeElement->descriptor = descriptor;
            if (status == Desc::TxStatus::committed)
            {
                sizeElement->newVal = oldSizeElement.newVal;
                sizeElement->oldVal = oldSizeElement.newVal;
            }
            else // Aborted.
            {
                sizeElement->newVal = oldSizeElement.oldVal;
                sizeElement->oldVal = oldSizeElement.oldVal;
            }
        }
    }
    // Replace the old size element.
    while (!vector->size.compare_exchange_weak(oldSizeElement, *sizeElement));

    // Set the size.
    size = sizeElement->oldVal;
    return size;
}
#endif
#ifdef BOOSTEDVEC
// Special way to retrieve the current size.
size_t RWSet::getSize(BoostedVector *vector, Desc *descriptor)
{
    if (hasSize)
    {
        return size;
    }
    vector->sizeLock.lock.lock();
    descriptor->locks.push_back(&vector->sizeLock);
    size = vector->size;
    // DEBUG:
    //printf("Size changed from %lu ", vector->size);
    hasSize = true;
    return size;
}
#endif

#if defined(COMPACTVEC) || defined(BOOSTEDVEC)
// Get an op node from a map. Allocate it if it doesn't already exist.
bool RWSet::getOp(RWOperation *&op, size_t index)
{
    op = operations[index];
    if (op == NULL)
    {
        op = Allocator<RWOperation>::alloc();
    }
    operations[index] = op;
    return true;
}
#endif

#endif