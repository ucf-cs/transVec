#include "transaction.hpp"

template <typename T>
Desc<T>::Desc(unsigned int size, Operation<T> *ops)
{
    this->size = size;
    this->ops = ops;
    // Transactions are always active at start.
    status.store(active);
    // Returned values are never safe to access until they have been explicitly set.
    returnedValues.store(false);
    return;
}

template <typename T>
Desc<T>::~Desc()
{
    //delete ops;
    return;
}

template <typename T>
T *Desc<T>::getResult(size_t index)
{
    if (index >= size)
    {
        return NULL;
    }
    if (status != committed)
    {
        return NULL;
    }
    return &(ops[index].ret);
}

template class Desc<int>;