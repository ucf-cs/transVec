#include "compactVector.hpp"

#ifdef COMPACTVEC

// A thread-local help stack, used to prevent cyclic dependencies.
// The key is the descriptor pointer.
// The value is the current opid.
thread_local std::unordered_map<Desc *, size_t> helpstack;

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
    printf("old: %10u\tnew: %10u\tselection: %s\tdesc: %p\n",
           oldVal,
           newVal,
           descriptor == NULL ? "NULL  " : (descriptor->status.load() == Desc::TxStatus::committed ? "commit" : "abort "),
           descriptor);
    return;
}

bool CompactVector::reserve(size_t size)
{
    return array->reserve(size);
}

// TODO: Decide what we need from this. Some will need to be removed.
// New element may need to be modified.
// Only includes the descriptor pointer and perhaps the new value.
// By the end of this function, newVal should store the value already in newElem in writes, or a copy of the existing value.
// oldVal should always contain the value represented by the previous transaction.
// Fails on seeing UNSET, except for push.
VAL CompactVector::updateElement(size_t index, CompactElement &newElem, Operation::OpType type, bool checkSize = false)
{
    Desc *newDesc = newElem.descriptor;

    CompactElement oldElem;
    bool success = false;
    do
    {
        // Check the returnVal array first.
        VAL returnVal = newDesc->returnValues[helpstack[newDesc]].load();
        // TODO: Use a specially reserved value instead of NULL.
        if (returnVal != NULL)
        {
            return returnVal;
        }

        // Attempt to read the old element.
        if (checkSize || type == Operation::OpType::size)
        {
            oldElem = size.load();
        }
        else
        {
            if (!array->read(index, oldElem))
            {
                // Failure means the transaction attempted an invalid read or write, as the vector wasn't allocated to this point.
                return UNSET;
            }
        }

        // Ensure the oldElem doesn't point to NULL.
        Desc *oldDesc = (oldElem.descriptor == NULL)
                            ? endTransaction
                            : oldElem.descriptor;

        // Quit if the transaction is no longer active.
        // Can happen if another thread helped the transaction complete.
        if (newDesc->status.load() != Desc::TxStatus::active)
        {
            returnVal = newDesc->returnValues[helpstack[newDesc]].load();
            // TODO: Use a specially reserved value instead of NULL.
            assert(returnVal != NULL);
            return returnVal;
        }

        // If helping occurred.
        // The descriptor and values match.
        if (oldDesc == newDesc && oldElem.newVal == newElem.newVal)
        {
            // Get and try to set the return value ourselves.
            // The helper may not have set it yet.
            // TODO: Use a specially reserved value instead of NULL.
            VAL null = NULL;
            newDesc->returnValues[helpstack[newDesc]].compare_exchange_strong(null, newElem.newVal);

            // Whether we succeed or fail, we need to make sure we retrieve a consistent value to maintain determinism.
            returnVal = newDesc->returnValues[helpstack[newDesc]].load();

            // If it doesn't match, something is probably wrong.
            assert(newElem.newVal == returnVal);
            // TODO: Use a specially reserved value instead of NULL.
            assert(returnVal != NULL);
            return returnVal;
        }

        // Check the status of the transaction.
        typename Desc::TxStatus status = oldDesc->status.load();
        // If the old element is part of another active transaction.
        if (status == Desc::TxStatus::active && oldDesc != newDesc)
        {
            // Help the active transaction.
            while (oldDesc->status.load() == Desc::TxStatus::active)
            {
#ifdef HELP
                helpTransaction(oldDesc);
#endif
            }
            // Update our status to its final (committed or aborted) state.
            status = oldDesc->status.load();
        }

        // Get the current value logically stored in the vector.
        VAL logicalVal;
        switch (status)
        {
        case Desc::TxStatus::committed:
            // We only get the new value if it was committed.
            logicalVal = oldElem.newVal;
            break;
        case Desc::TxStatus::aborted:
            // Transaction was aborted.
            // Grab the old page's old value.
            logicalVal = oldElem.oldVal;
            break;
        case Desc::TxStatus::active:
            // Another operation in the same transaction modified this value.
            assert(oldDesc == newDesc);
            // Preserve the oldVal.
            logicalVal = oldElem.oldVal;
            break;
        default:
            assert(false);
            break;
        }

        // Now finalize the values we want to use to replace oldElem.
        // Also determine the return value.
        VAL ret = UNSET;
        //newElem.descriptor = newElem.descriptor; // Unchanged.
        newElem.oldVal = logicalVal;
        // If this is a checkSize update, we follow different rules.
        if (checkSize)
        {
            switch (type)
            {
            case Operation::OpType::read:
            case Operation::OpType::write:
            case Operation::OpType::size:
                newElem.newVal = logicalVal;
                ret = logicalVal;
                break;
            case Operation::OpType::pushBack:
                newElem.newVal = logicalVal + 1;
                ret = logicalVal;
                break;
            case Operation::OpType::popBack:
                newElem.newVal = logicalVal - 1;
                ret = logicalVal - 1;
                break;
            default:
                assert(false);
                break;
            }
        }
        else
        {
            switch (type)
            {
            case Operation::OpType::read:
            case Operation::OpType::size:
                newElem.newVal = logicalVal;
                ret = logicalVal;
                break;
            case Operation::OpType::write:
            case Operation::OpType::pushBack:
                // newVal is unchanged.
                ret = newElem.newVal;
                break;
            case Operation::OpType::popBack:
                newElem.newVal = UNSET;
                ret = UNSET;
                break;
            default:
                assert(false);
                break;
            }
        }

        // Attempt to CAS the new element.
        if (checkSize || type == Operation::OpType::size)
        {
            success = size.compare_exchange_strong(oldElem, newElem);
        }
        else
        {
            success = array->tryWrite(index, oldElem, newElem);
        }
    } while (!success);

    return true;
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
        endTransaction = new Desc(NULL, NULL, NULL, 0);
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

bool CompactVector::helpTransaction(Desc *desc)
{
    // Place the transaction descriptor on the help stack.
    helpstack[desc] = 0;
    // A local map, used by the transaction function.
    valMap *localMap;
    // Copy the inputMap to the localMap.
    *localMap = *desc->inputMap;
    // Run the transaction function.
    bool ret = desc->func(desc, localMap);
    // Remove the transaction descriptor from the help stack.
    helpstack.erase(desc);

    auto active = Desc::TxStatus::active;
    auto committed = Desc::TxStatus::committed;
    auto aborted = Desc::TxStatus::aborted;
    // If the function executed successfully.
    if (ret)
    {
        // The transaction commits.
        desc->status.compare_exchange_strong(active, committed);
    }
    else
    {
        // The transaction aborts.
        // This induces logical rollback.
        desc->status.compare_exchange_strong(active, aborted);
    }
    // Copy the localMap to the outputMap.
    *desc->outputMap = *localMap;
    return ret;
}

// The transaction execution function. Executes a single transaction.
bool CompactVector::executeTransaction(bool (*func)(Desc *desc, valMap *localMap), valMap *inMap, valMap *outMap, size_t size)
{
    // Create a transaction descriptor to associate with the transaction function.
    Desc *desc = new Desc(func, inMap, outMap, size);
    // Perform the transaction, via the helping scheme.
    helpTransaction(desc);
    // Execution is successful only if the transaction commits.
    return desc->status.load() == Desc::TxStatus::committed;
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
        printf("%5lu:\t", i);
        elem.print();
    }
    printf("\n");
    return;
}

// TODO: Figure out exactly how this works. This is the most confusing part of the code.
// Run an operation on the data structure.
int CompactVector::callOp(Desc *desc, Operation::OpType type, void *fmt...)
{
    // Prepare to read the variadic arguments.
    // For simplcity, we will assume that all arguments were valid for the OpType.
    va_list args;
    va_start(args, fmt);

    // Do not execute if the transaction already aborted.
    if (desc->status.load() == Desc::TxStatus::aborted)
    {
        return NULL;
    }
    // Acquire the operation's ID.
    // TODO: How does one get this from the help stack?
    int opid = helpstack[desc]++;
    // If the return value was already set.
    if (desc->returnValues[opid] != NULL)
    {
        // Just return that value.
        return desc->returnValues[opid];
    }
    // The operation's return value.
    // NOTE: Invalid reads and writes should return UNSET for the vector.
    int ret = NULL;
    // Run the appropriate function type.
    switch (type)
    {
    case Operation::OpType::read:
        // Check size.
        // TODO: Get and validate size, or keep using UNSET.

        // Get the operation index.
        size_t index = va_arg(args, size_t);
        // Create a replacement element.
        CompactElement newElem;
        newElem.descriptor = desc;
        if (updateElement(index, newElem))
        {
            // Return new value on success.
            ret = newElem.newVal;
        }
        else
        {
            // TODO: Is the the proper thing to return?
            ret = UNSET;
        }
        break;
    case Operation::OpType::write:
        // TODO: Check size.
        // Check element at target index.
        size_t index = va_arg(args, size_t);
        size_t val = va_arg(args, VAL);
        // Replace element with our descriptor pointer and new value.
        // Create a replacement element.
        CompactElement newElem;
        newElem.descriptor = desc;
        newElem.newVal = val;
        if (updateElement(index, newElem))
        {
            // Return new value on success.
            ret = newElem.newVal;
        }
        else
        {
            // TODO: Is the the proper thing to return?
            ret = UNSET;
        }
        break;
    case Operation::OpType::pushBack:
        // Check size.
        size_t index = 0; // TODO: Actually check size.
        // Increment size.

        size_t val = va_arg(args, VAL);
        // Replace element with our descriptor pointer and new value.
        // Create a replacement element.
        CompactElement newElem;
        newElem.descriptor = desc;
        newElem.newVal = val;
        // Place element at target index (size).
        if (updateElement(index, newElem, type))
        {
            // Return new value on success.
            ret = newElem.newVal;
        }
        else
        {
            // TODO: Is the the proper thing to return?
            ret = UNSET;
        }
        break;
    case Operation::OpType::popBack:
        // Check size.
        // Decrement size.

        // Place element at target index (decremented size).

        // Replace element with our descriptor pointer and new value.
        // Create a replacement element.
        CompactElement newElem;
        newElem.descriptor = desc;
        newElem.newVal = UNSET;
        // Place element at target index (size).
        if (updateElement(index, newElem, type))
        {
            // Return old value on success.
            ret = newElem.oldVal;
        }
        else
        {
            // TODO: Is the the proper thing to return?
            ret = UNSET;
        }
        break;
    case Operation::OpType::size:
        // Check size.
        // Replace element with our descriptor pointer.
        CompactElement newElem;
        newElem.descriptor = desc;
        updateElement(0, newElem, type);
        break;
    case Operation::OpType::reserve:
        // Just pass through vector reserve.
        array->reserve((size_t)args);
        break;
    default:
        break;
    }
    // Set the return value.
    // TODO: Is this safe without atomics? Only if the individual operations are as well. The individual operations must return the same value to all threads if the opid matches.
    // TODO: Modify my vector to support multiple operations on the same element in the same transaction.
    desc->returnValues[opid].store(ret);
    // Advance the help stack?
    //helpstack.nextOp();
    // Return the result of the operation.
    return ret;
}

#endif