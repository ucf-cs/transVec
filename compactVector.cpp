#include "compactVector.hpp"

#ifdef COMPACTVEC

CompactElement::CompactElement() noexcept
{
    // Ensure all values have appropriate defaults.
    oldVal = UNSET;
    newVal = UNSET;
    descriptor = NULL;
    return;
}

void CompactElement::print()
{
    printf("oldval: %du\tnewVal: %du\tdescriptor: %p", oldVal, newVal, descriptor);
    return;
}

bool CompactVector::reserve(size_t size)
{
    return array->reserve(size);
}

bool CompactVector::updateElement(size_t index, CompactElement &newElem)
{
    CompactElement oldElem;
    do
    {
        // Attempt to read the old element.
        if (!array->read(index, oldElem))
        {
            // Failure means the transaction attempted an invalid read or write, as the vector wasn't allocated to this point.
            newElem.descriptor->status.store(Desc::TxStatus::aborted);
            // DEBUG: Abort reporting.
            //printf("Aborted!\n");
            // No need to even try anymore. The whole transaction failed.
            return false;
        }

        // Ensure the oldElem doesn't point to NULL.
        Desc *oldDesc = (oldElem.descriptor == NULL)
                            ? endTransaction
                            : oldElem.descriptor;

        // Quit if the transaction is no longer active.
        // Can happen if another thread helped the transaction complete.
        if (newElem.descriptor->status.load() != Desc::TxStatus::active)
        {
            return false;
        }

        // Quit early if the element is already inserted.
        // May occur during helping.
        if (oldDesc == newElem.descriptor)
        {
            // Insertion failed here but succeeded elsewhere.
            // Act like we succeeded and continue to the next element.
            return true;
        }

        // Check the status of the transaction.
        typename Desc::TxStatus status = oldDesc->status.load();
        // If the old element is part of an active transaction.
        // No need to help reads, so check if there are any dependencies with writes.
        RWOperation *op = NULL;
        if (status == Desc::TxStatus::active && newElem.descriptor->set.load()->getOp(op, index) && op->lastWriteOp != NULL)
        {
            // Help the active transaction.
            while (oldDesc->status.load() == Desc::TxStatus::active)
            {
#ifdef HELP
                completeTransaction(oldDesc, true, index);
#endif
            }
            // Update our status to its final (committed or aborted) state.
            status = oldDesc->status.load();
        }

        // We only get the new value if it was write committed.
        // Also check if it matches the end transaction, as that is a special case where we should see a write, even though the set is empty.
        if (oldDesc == endTransaction || (status == Desc::TxStatus::committed && oldDesc->set.load()->getOp(op, index) && op->lastWriteOp != NULL))
        {
            newElem.oldVal = oldElem.newVal;
        }
        // Transaction was aborted or operation was a read.
        // Grab the old page's old value.
        else
        {
            // Copy over the previous new value to become the replacement's old value.
            newElem.oldVal = oldElem.oldVal;
        }

        // Abort if the operation fails our bounds check.
        newElem.descriptor->set.load()->getOp(op, index);
        if (op != NULL && op->checkBounds == RWOperation::Assigned::yes && newElem.oldVal == UNSET)
        {
            newElem.descriptor->status.store(Desc::TxStatus::aborted);
            // DEBUG: Abort reporting.
            //printf("Aborted!\n");
            // No need to even try anymore. The whole transaction failed.
            return false;
        }
    } while (!array->tryWrite(index, oldElem, newElem));

    // Store the old value in the associated operations.
    // We move this here because otherwise we have no index to reference.
    RWOperation *op = NULL;
    newElem.descriptor->set.load()->getOp(op, index);
    // For each operation attempting to read the element.
    for (size_t i = 0; i < op->readList.size(); i++)
    {
        // Assign the return values.
        op->readList[i]->ret = newElem.oldVal;
    }

    return true;
}

void CompactVector::insertElements(RWSet *set, bool helping, unsigned int startElement)
{
    // Get the start of the map.
    typename std::map<size_t, RWOperation *, std::equal_to<size_t>, MemAllocator<std::pair<size_t, RWOperation *>>>::reverse_iterator iter = set->operations.rbegin();

    // Advance to a starting index, if specified.
    if (startElement < INT32_MAX)
    {
        // Attempt to find an operation at the target index.
        auto foundIter = set->operations.find(startElement);
        // If we found it.
        if (foundIter != set->operations.end())
        {
            // Reposition our iterator.
            //iter = make_reverse_iterator(foundIter);
            iter = std::reverse_iterator<std::_Rb_tree_iterator<std::pair<const size_t, RWOperation *>>>(foundIter);
        }
        // If the element is invalid, which should never happen.
        else
        {
            printf("Specified element %du does not exist. Starting from the beginning.\n", startElement);
        }
    }

    for (; iter != set->operations.rend(); ++iter)
    {
        // If something caused a failure, don't insert any more elements for this transaction.
        if (set->descriptor->status.load() == Desc::TxStatus::aborted)
        {
            break;
        }

        // The index comes straight from the map.
        size_t index = iter->first;

        // The element is generated
        CompactElement element;
        // NOTE: oldVal is automatically set upon insertion, so don't worry about setting it yet.
        // Only assign a new value if we actually have one to assign.
        if (iter->second != NULL && iter->second->lastWriteOp != NULL)
        {
            element.newVal = iter->second->lastWriteOp->val;
        }
        element.descriptor = set->descriptor;

        // Replace the element.
        // Returns false if the transaction is no longer active.
        if (!updateElement(index, element))
        {
            break;
        }
    }
    return;
}

CompactVector::CompactVector()
{
    // Ensure that our elements are the size our bitfield perscribes.
    assert(sizeof(CompactElement) <= 16);
    // Initialize our internal segmented array.
    array = new SegmentedVector<CompactElement>();
    // Allocate an end transaction.
    if (endTransaction == NULL)
    {
        endTransaction = new Desc(0, NULL);
        endTransaction->status.store(Desc::TxStatus::committed);
    }
    // Initialize size.
    CompactElement sizeElem;
    sizeElem.oldVal = 0;
    sizeElem.newVal = 0;
    sizeElem.descriptor = endTransaction;
    size.store(sizeElem);
    return;
}

bool CompactVector::prepareTransaction(Desc *descriptor)
{
    RWSet *set = descriptor->set.load();
    if (set == NULL)
    {
        // Initialize the RWSet object.
        set = Allocator<RWSet>::alloc();

        // Create the read/write set.
        // NOTE: Getting size may happen here.
        // Requires special consideration for the helping scheme.
        // This will work because helpers will either replace size or find and help an active transaction.
        set->createSet(descriptor, this);

        RWSet *nullVal = NULL;
        descriptor->set.compare_exchange_strong(nullVal, set);
        // TODO2: Preferably deallocate if we fail to CAS.
    }
    // Make sure we only work with the set that succeeded first.
    set = descriptor->set.load();
    // Ensure that we can fit all of the elements we plan to insert.
    if (!reserve(set->maxReserveAbsolute > set->size ? set->maxReserveAbsolute : set->size))
    {
        descriptor->status.store(Desc::TxStatus::aborted);
        // DEBUG: Abort reporting.
        //printf("Aborted!\n");
        return false;
    }
    return true;
}

bool CompactVector::completeTransaction(Desc *descriptor, bool helping, unsigned int startElement)
{
    // Insert the elements.
    insertElements(descriptor->set.load(), helping, startElement);

    auto active = Desc::TxStatus::active;
    auto committed = Desc::TxStatus::committed;
    // Commit the transaction.
    // If this fails, either we aborted or some other transaction committed, so no need to retry.
    if (!descriptor->status.compare_exchange_strong(active, committed))
    {
        // At this point, the transaction either committed or aborted.

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

void CompactVector::executeTransaction(Desc *descriptor)
{
    // Initialize the set for the descriptor.
    prepareTransaction(descriptor);
    // Complete the transaction.
    completeTransaction(descriptor);
}

void CompactVector::sizeHelp(Desc *descriptor)
{
    // DEBUG: Print the thread id that is helping.
    //printf("Thread %lu is helping descriptor %p\n", std::hash<std::thread::id>{}(std::this_thread::get_id()), descriptor);

    // Must actually start at the very beginning.
    prepareTransaction(descriptor);
    // Must help from the beginning of the list, since we didn't help part way through.
    completeTransaction(descriptor, true, UINT32_MAX);
}

void CompactVector::printContents()
{
    for (size_t i = 0;; i++)
    {
        CompactElement elem;
        if (!array->read(i, elem))
        {
            break;
        }
        printf("%5lu:\told: %10u\tnew: %10u\tselection: %s\tdesc: %p\n",
               i,
               elem.oldVal,
               elem.newVal,
               elem.descriptor == NULL ? "NULL  " : (elem.descriptor->status.load() == Desc::TxStatus::committed ? "commit" : "abort "),
               elem.descriptor);
    }
    printf("\n");
    return;
}

#endif