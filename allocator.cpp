#include "allocator.hpp"

//template <class T, class U, size_t S>
//DeltaPage<T, U, S, V> **Allocator<T, U, S>::pool;

template <class T, class U, size_t S>
Page<T, U, S> *Allocator<T, U, S>::getNewPage(size_t size)
{
    Page<T, U, S> *page = NULL;
    switch (size)
    {
    case 1:
        if (pool1.size() > 0)
        {
            page = (Page<T, U, S> *)pool1.back();
            pool1.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 1>;
        }
        break;
    case 2:
        if (pool2.size() > 0)
        {
            page = (Page<T, U, S> *)pool2.back();
            pool2.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 2>;
        }
        break;
    case 3:
        if (pool3.size() > 0)
        {
            page = (Page<T, U, S> *)pool3.back();
            pool3.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 3>;
        }
        break;
    case 4:
        if (pool4.size() > 0)
        {
            page = (Page<T, U, S> *)pool4.back();
            pool4.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 4>;
        }
        break;
    case 5:
        if (pool5.size() > 0)
        {
            page = (Page<T, U, S> *)pool5.back();
            pool5.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 5>;
        }
        break;
    case 6:
        if (pool6.size() > 0)
        {
            page = (Page<T, U, S> *)pool6.back();
            pool6.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 6>;
        }
        break;
    case 7:
        if (pool7.size() > 0)
        {
            page = (Page<T, U, S> *)pool7.back();
            pool7.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 7>;
        }
        break;
    case 8:
        if (pool8.size() > 0)
        {
            page = (Page<T, U, S> *)pool8.back();
            pool8.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 8>;
        }
        break;
    case 9:
        if (pool9.size() > 0)
        {
            page = (Page<T, U, S> *)pool9.back();
            pool9.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 9>;
        }
        break;
    case 10:
        if (pool10.size() > 0)
        {
            page = (Page<T, U, S> *)pool10.back();
            pool10.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 10>;
        }
        break;
    case 11:
        if (pool11.size() > 0)
        {
            page = (Page<T, U, S> *)pool11.back();
            pool11.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 11>;
        }
        break;
    case 12:
        if (pool12.size() > 0)
        {
            page = (Page<T, U, S> *)pool12.back();
            pool12.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 12>;
        }
        break;
    case 13:
        if (pool13.size() > 0)
        {
            page = (Page<T, U, S> *)pool13.back();
            pool13.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 13>;
        }
        break;
    case 14:
        if (pool14.size() > 0)
        {
            page = (Page<T, U, S> *)pool14.back();
            pool14.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 14>;
        }
        break;
    case 15:
        if (pool15.size() > 0)
        {
            page = (Page<T, U, S> *)pool15.back();
            pool15.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 15>;
        }
        break;
    case 16:
        if (pool16.size() > 0)
        {
            page = (Page<T, U, S> *)pool16.back();
            pool16.pop_back();
        }
        else
        {
            printf("Pool for update size %lu is empty.\n", size);
            page = new DeltaPage<T, U, S, 16>;
        }
        break;
    default:
        printf("Invalid page size specified (%lu). Assuming size=%lu\n", size, S);
        page = new DeltaPage<T, U, S, S>;
    }
    return page;
}

template <class T, class U, size_t S>
void Allocator<T, U, S>::freePage(Page<T, U, S> *page)
{
    size_t size = page->getSize();
    switch (size)
    {
    case 1:
        pool1.push_back((DeltaPage<VAL, VAL, SGMT_SIZE, 1> *)page);
        break;
    case 2:
        pool2.push_back((DeltaPage<T, U, S, 2> *)page);
        break;
    case 3:
        pool3.push_back((DeltaPage<T, U, S, 3> *)page);
        break;
    case 4:
        pool4.push_back((DeltaPage<T, U, S, 4> *)page);
        break;
    case 5:
        pool5.push_back((DeltaPage<T, U, S, 5> *)page);
        break;
    case 6:
        pool6.push_back((DeltaPage<T, U, S, 6> *)page);
        break;
    case 7:
        pool7.push_back((DeltaPage<T, U, S, 7> *)page);
        break;
    case 8:
        pool8.push_back((DeltaPage<T, U, S, 8> *)page);
        break;
    case 9:
        pool9.push_back((DeltaPage<T, U, S, 9> *)page);
        break;
    case 10:
        pool10.push_back((DeltaPage<T, U, S, 10> *)page);
        break;
    case 11:
        pool11.push_back((DeltaPage<T, U, S, 11> *)page);
        break;
    case 12:
        pool12.push_back((DeltaPage<T, U, S, 12> *)page);
        break;
    case 13:
        pool13.push_back((DeltaPage<T, U, S, 13> *)page);
        break;
    case 14:
        pool14.push_back((DeltaPage<T, U, S, 14> *)page);
        break;
    case 15:
        pool15.push_back((DeltaPage<T, U, S, 15> *)page);
        break;
    case 16:
        pool16.push_back((DeltaPage<T, U, S, 16> *)page);
        break;
    default:
        printf("Invalid page size specified (%lu). Failed to reuse page\n", size);
    }
}

// Preallocation occurs here.
template <class T, class U, size_t S>
void Allocator<T, U, S>::initPool()
{
    for (size_t i = 0; i < POOL_SIZE; i++)
    {
        pool1.push_back(new DeltaPage<T, U, S, 1>());
        pool2.push_back(new DeltaPage<T, U, S, 2>());
        pool3.push_back(new DeltaPage<T, U, S, 3>());
        pool4.push_back(new DeltaPage<T, U, S, 4>());
        pool5.push_back(new DeltaPage<T, U, S, 5>());
        pool6.push_back(new DeltaPage<T, U, S, 6>());
        pool7.push_back(new DeltaPage<T, U, S, 7>());
        pool8.push_back(new DeltaPage<T, U, S, 8>());
        pool9.push_back(new DeltaPage<T, U, S, 9>());
        pool10.push_back(new DeltaPage<T, U, S, 10>());
        pool11.push_back(new DeltaPage<T, U, S, 11>());
        pool12.push_back(new DeltaPage<T, U, S, 12>());
        pool13.push_back(new DeltaPage<T, U, S, 13>());
        pool14.push_back(new DeltaPage<T, U, S, 14>());
        pool15.push_back(new DeltaPage<T, U, S, 15>());
        pool16.push_back(new DeltaPage<T, U, S, 16>());
    }
}

template class Allocator<VAL, VAL, SGMT_SIZE>;
template class Allocator<size_t, VAL, 1>;