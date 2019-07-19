#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstdlib>
#include <mutex>
#include <ostream>

#include "transaction.hpp"

class Vector
{
private:
    size_t size = 0;
    size_t capacity = 0;
    VAL *array = NULL;

    size_t highestBit(size_t val)
    {
        // Subtract 1 so the rightmost position is 0 instead of 1.
        return (sizeof(val) * 8) - __builtin_clz(val | 1) - 1;

        // Slower alternate approach.
        size_t onePos = 0;
        const size_t max = 8 * sizeof(size_t) - 1;
        for (size_t i = 0; i <= 8 * sizeof(size_t); i++)
        {
            // Special case for zero.
            if (i == 8 * sizeof(size_t))
            {
                return 0;
            }

            size_t mask = (size_t)1 << (max - i);
            if ((val & mask) != 0)
            {
                onePos = max - i;
                break;
            }
        }
        return onePos;
    }

public:
    Vector(size_t capacity = 1)
    {
        reserve(capacity);
        return;
    }
    bool pushBack(VAL val)
    {
        if (size + 1 > capacity)
        {
            if (!reserve(size + 1))
            {
                return false;
            }
        }
        array[size++] = val;
        return true;
    }
    bool popBack(VAL &val)
    {
        if (size <= 0)
        {
            return false;
        }
        val = array[--size];
        return true;
    }
    bool read(size_t index, VAL &val)
    {
        if (index > size)
        {
            return false;
        }
        val = array[index];
        return true;
    }
    bool write(size_t index, VAL val)
    {
        if (index > size)
        {
            return false;
        }
        array[index] = val;
        return true;
    }
    size_t getSize()
    {
        return size;
    }
    bool reserve(size_t capacity)
    {
        // No need to do anything if our capacity is already sufficient.
        if (capacity <= this->capacity)
        {
            return true;
        }
        // Compute the smallest power of 2 >= to the requested capacity.
        size_t reserveCapacity = 1 << (highestBit(capacity) + 1);
        // Allocate an array with a sufficient capacity.
        VAL *newArray = (VAL *)malloc(reserveCapacity * sizeof(VAL));
        // If we failed to allocate, then this reserve call fails.
        if (newArray == NULL)
        {
            return false;
        }
        // If an old array had been allocated.
        if (array != NULL)
        {
            // Copy over the relevant old values.
            for (size_t i = 0; i < size; i++)
            {
                VAL tmp = array[i];
                newArray[i] = tmp;
            }
            // Free the old array. Otherwise, we have a memory leak.
            free(array);
        }
        // Replace the old array.
        array = newArray;
        // Update our new capacity.
        this->capacity = reserveCapacity;
        return true;
    }
    void printContents()
    {
        for (size_t i = 0; i < size; i++)
        {
            std::cout << array[size] << std::endl;
        }
        return;
    }
};

class CoarseTransVector
{
private:
    Vector vector;
    std::mutex mtx;

public:
    void executeTransaction(Desc *desc)
    {
        bool ret = true;
        mtx.lock();
        //printf("%lu got lock.\n", std::hash<std::thread::id>()(std::this_thread::get_id()));
        for (size_t i = 0; i < desc->size; i++)
        {
            Operation *op = &desc->ops[i];
            switch (op->type)
            {
            case Operation::OpType::read:
                ret = vector.read(op->index, op->ret);
                break;
            case Operation::OpType::write:
                ret = vector.write(op->index, op->val);
                break;
            case Operation::OpType::pushBack:
                ret = vector.pushBack(op->val);
                break;
            case Operation::OpType::popBack:
                ret = vector.popBack(op->ret);
                break;
            case Operation::OpType::size:
                op->ret = vector.getSize();
                break;
            case Operation::OpType::reserve:
                ret = vector.reserve(op->index);
            default:
                ret = false;
                break;
            }

            if (!ret)
            {
                break;
            }
        }
        if (ret)
        {
            desc->status.store(Desc::TxStatus::committed);
        }
        else
        {
            desc->status.store(Desc::TxStatus::aborted);
        }
        //printf("%lu releasing lock.\n", std::hash<std::thread::id>()(std::this_thread::get_id()));
        mtx.unlock();
        return;
    }
    void printContents()
    {
        mtx.lock();
        vector.printContents();
        mtx.unlock();
    }
};

class GCCSTMVector
{
private:
    Vector vector;

public:
    void executeTransaction(Desc *desc)
    {
        bool ret = true;
        __transaction_atomic
        {
            for (size_t i = 0; i < desc->size; i++)
            {
                Operation *op = &desc->ops[i];
                switch (op->type)
                {
                case Operation::OpType::read:
                    ret = vector.read(op->index, op->ret);
                    break;
                case Operation::OpType::write:
                    ret = vector.write(op->index, op->val);
                    break;
                case Operation::OpType::pushBack:
                    ret = vector.pushBack(op->val);
                    break;
                case Operation::OpType::popBack:
                    ret = vector.popBack(op->ret);
                    break;
                case Operation::OpType::size:
                    op->ret = vector.getSize();
                    break;
                case Operation::OpType::reserve:
                    ret = vector.reserve(op->index);
                default:
                    ret = false;
                    break;
                }

                if (!ret)
                {
                    break;
                }
            }
        }
        if (ret)
        {
            desc->status.store(Desc::TxStatus::committed);
        }
        else
        {
            desc->status.store(Desc::TxStatus::aborted);
        }
        return;
    }
    void printContents()
    {
        __transaction_atomic
        {
            vector.printContents();
        }
    }
};

#endif