#include "rwSet.hpp"

#if defined SEGMENTVEC || defined COMPACTVEC || defined BOOSTEDVEC

RWSet::~RWSet()
{
    operations.clear();
#ifdef SEGMENTVEC
    sizeDesc = NULL;
#endif
    return;
}

#ifdef SEGMENTVEC
std::pair<size_t, size_t> RWSet::access(size_t pos)
{
    size_t first = pos / (size_t)SGMT_SIZE;
    size_t second = pos % (size_t)SGMT_SIZE;
    // DEBUG: Ensure we access the correct page and offset for a given segment size.
    //printf("SGMT_SIZE=%lu\tpos=%lu\tfirst=%lu\tsecond=%lu\n", SGMT_SIZE, pos, first, second);
    return std::make_pair(first, second);
}
#endif
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

#ifdef SEGMENTVEC
bool RWSet::createSet(Desc *descriptor, TransactionalVector *vector)
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
#ifdef SEGMENTVEC
        std::pair<size_t, size_t> indexes;
#endif
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
#ifdef HELP_FREE_READS
        case Operation::OpType::hfRead:
            assert(false && "Cannot use help-free reads in a standard transaction.");
            break;
#endif
        default:
            // Unexpected operation type found.
            break;
        }
    }

    // If we accessed size, we need to report what we changed it to.
#ifdef SEGMENTVEC
    if (sizeDesc != NULL)
    {
        sizeDesc->set(0, NEW_VAL, size);
    }
#endif
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

#ifdef SEGMENTVEC
void RWSet::setToPages(Desc *descriptor)
{
    // All of the pages we want to insert (except size), ordered from low to high.
    std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator> *pages = Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::alloc();

    // For each page to generate.
    // These are all independent of shared memory.
    for (auto i = operations.begin(); i != operations.end(); ++i)
    {
        // Create the initial page.
        Page<VAL, SGMT_SIZE> *page = Allocator<Page<VAL, SGMT_SIZE>>::alloc();
        // Link the page to the transaction descriptor.
        page->transaction = descriptor;

        for (size_t j = 0; j < SGMT_SIZE; j++)
        {
            RWOperation *op = i->second[j];
            // Ignore NULL elements in the array.
            if (op == NULL)
            {
                // DEBUG: NULL element in the array.
                //printf("No op at page %lu index %lu\n", i->first, j);
                continue;
            }
            // Infer a read based on the read list size.
            page->bitset.read[j] = (op->readList.size() > 0);
            // Infer a write based on the write op pointer.
            page->bitset.write[j] = (op->lastWriteOp != NULL);
            // Check bounds only if the first operation on this element needed to.
            page->bitset.checkBounds[j] = (op->checkBounds == RWOperation::Assigned::yes) ? true : false;
            // If a write occured, place the appropriate new value from it.
            if (op->lastWriteOp != NULL)
            {
                page->set(j, NEW_VAL, op->lastWriteOp->val);
            }
        }
        (*pages)[i->first] = page;
    }

    // Store a pointer to the pages in the descriptor.
    // Only the first thread to finish the job succeeds here.
    std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator> *nullVal = NULL;
    descriptor->pages.compare_exchange_strong(nullVal, pages);

    return;
}
#endif

#ifdef SEGMENTVEC
size_t RWSet::getSize(TransactionalVector *vector, Desc *descriptor)
{
    if (sizeDesc != NULL)
    {
        return size;
    }

    // Prepend a read page to size.
    // The size page is always of size 1.
    // Set all unchanging page values here.
    Page<size_t, 1> *tempSizeDesc = Allocator<Page<size_t, 1>>::alloc();
    tempSizeDesc->bitset.read.set();
    tempSizeDesc->bitset.write.set();
    tempSizeDesc->bitset.checkBounds.reset();
    tempSizeDesc->transaction = descriptor;
    tempSizeDesc->next = NULL;

    Page<size_t, 1> *rootPage = NULL;
    do
    {
        // Get the current head.
        rootPage = vector->size.load();

        // Quit if the transaction is no longer active.
        if (descriptor->status.load() != Desc::TxStatus::active)
        {
            return 0;
        }

        // If the root page does not exist.
        // Root page should never be NULL, ever.
        if (rootPage == NULL)
        {
            // Assume an initial size of 0.
            tempSizeDesc->set(0, OLD_VAL, 0);
        }
        // If a helper got here first.
        else if (rootPage->transaction == tempSizeDesc->transaction)
        {
            // Do not insert again.
            break;
        }
        else
        {
            enum Desc::TxStatus status = rootPage->transaction->status.load();
            while (status == Desc::TxStatus::active)
            {
#ifdef HELP
                // Help the active transaction.
                vector->sizeHelp(rootPage->transaction);
#endif
                status = rootPage->transaction->status.load();
            }

            // Store the root page's value as an old value in case we abort.
            // Get the appropriate value from the root page depending on whether or not it succeeded.
            size_t value = UNSET;
            if (status == Desc::TxStatus::committed)
            {
                rootPage->get(0, NEW_VAL, value);
            }
            else
            {
                rootPage->get(0, OLD_VAL, value);
            }
            tempSizeDesc->set(0, OLD_VAL, value);
        }
        // Append the old page onto the new page. Used to maintain history.
        tempSizeDesc->next = rootPage;
    }
    // Replace the page. Finish on success. Retry on failure.
    while (!vector->size.compare_exchange_weak(rootPage, tempSizeDesc));

    // Store the actual size locally.
    vector->size.load()->get(0, OLD_VAL, size);

    // Store the descriptor locally.
    sizeDesc = tempSizeDesc;

    return size;
}
#endif
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
                continue;
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

#ifdef SEGMENTVEC
void RWSet::getOp(RWOperation *&op, std::pair<size_t, size_t> indexes)
{
    op = operations[indexes.first][indexes.second];
    if (op == NULL)
    {
        op = Allocator<RWOperation>::alloc();
        assert(op != NULL);
        operations[indexes.first][indexes.second] = op;
    }
    return;
}
#endif
#if defined(COMPACTVEC) || defined(BOOSTEDVEC)
// Get an op node from a map. Allocate it if it doesn't already exist.
bool RWSet::getOp(RWOperation *&op, size_t index)
{
    try
    {
        op = operations.at(index);
    }
    catch (std::out_of_range)
    {
        op = Allocator<RWOperation>::alloc();
        assert(op != NULL);
        operations.insert(std::pair<size_t, RWOperation *>(index, op));
    }
    return true;
}
#endif

#ifdef SEGMENTVEC
void RWSet::printOps()
{
    for (auto it = operations.begin(); it != operations.end(); ++it)
    {
        std::cout << "Page " << it->first << std::endl;
        for (int i = SGMT_SIZE - 1; i >= 0; i--)
        {
            std::cout << (it->second[i] == NULL ? SGMT_SIZE + 1 : i) << " ";
        }
        std::cout << std::endl;
    }
    return;
}
#endif
#endif