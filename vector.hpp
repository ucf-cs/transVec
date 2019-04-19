#include <cstdlib>
#include <mutex>

#include "transaction.hpp"

template <class T>
class Vector
{
  private:
    size_t size = 0;
    size_t capacity = 0;
    T *array = NULL;

    size_t highestBit(size_t val)
    {
        // Subtract 1 so the rightmost position is 0 instead of 1.
        //return (sizeof(val) * 8) - __builtin_clz(val | 1) - 1;

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
    bool pushBack(T val)
    {
        if (size + 1 > capacity)
        {
            if (!reserve(size + 1, true))
            {
                return false;
            }
        }
        array[size++] = val;
        return true;
    }
    bool popBack(T &val)
    {
        if (size - 1 < 0)
        {
            return false;
        }
        val = array[--size];
        return true;
    }
    bool read(size_t index, T &val)
    {
        if (index > size)
        {
            return false;
        }
        val = array[index];
        return true;
    }
    bool write(size_t index, T val)
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
    bool reserve(size_t size, bool lock = true)
    {
        size_t reserveSize = 1 << (highestBit(size) + 1);
        T *newArray = (T *)malloc(reserveSize * sizeof(T));
        if (newArray == NULL)
        {
            return false;
        }
        if (array != NULL)
        {
            for (size_t i = 0; i < size; i++)
            {
                newArray[i] = array[i];
            }
            free(array);
        }
        array = newArray;
        capacity = reserveSize;
        return true;
    }
    void printContents()
    {
        for (size_t i = 0; i < size; i++)
        {
            printf("%ul\n", array[size]);
        }
        return;
    }
};

template <class T>
class CoarseTransVector
{
  private:
    Vector<T> vector;
    std::mutex mtx;

  public:
    void executeTransaction(Desc<T> *desc)
    {
        bool ret = true;
        mtx.lock();
        for (size_t i = 0; i < desc->size; i++)
        {
            Operation<T> *op = &desc->ops[i];
            switch (op->type)
            {
            case Operation<T>::OpType::read:
                ret = vector.read(op->index, op->ret);
                break;
            case Operation<T>::OpType::write:
                ret = vector.write(op->index, op->val);
                break;
            case Operation<T>::OpType::pushBack:
                ret = vector.pushBack(op->val);
                break;
            case Operation<T>::OpType::popBack:
                ret = vector.popBack(op->ret);
                break;
            case Operation<T>::OpType::size:
                op->ret = vector.getSize();
                break;
            case Operation<T>::OpType::reserve:
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
            desc->status.store(Desc<T>::TxStatus::committed);
            desc->returnedValues.store(true);
        }
        else
        {
            desc->status.store(Desc<T>::TxStatus::aborted);
        }
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

template <class T>
class GCCSTMVector
{
  private:
    Vector<T> vector;

  public:
    void executeTransaction(Desc<T> *desc)
    {
        bool ret = true;
        __transaction_atomic
        {
            for (size_t i = 0; i < desc->size; i++)
            {
                Operation<T> *op = &desc->ops[i];
                switch (op->type)
                {
                case Operation<T>::OpType::read:
                    ret = vector.read(op->index, op->ret);
                    break;
                case Operation<T>::OpType::write:
                    ret = vector.write(op->index, op->val);
                    break;
                case Operation<T>::OpType::pushBack:
                    ret = vector.pushBack(op->val);
                    break;
                case Operation<T>::OpType::popBack:
                    ret = vector.popBack(op->ret);
                    break;
                case Operation<T>::OpType::size:
                    op->ret = vector.getSize();
                    break;
                case Operation<T>::OpType::reserve:
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
            desc->status.store(Desc<T>::TxStatus::committed);
            desc->returnedValues.store(true);
        }
        else
        {
            desc->status.store(Desc<T>::TxStatus::aborted);
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