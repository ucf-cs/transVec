#ifndef TRANSVECTOR_H
#define TRANSVECTOR_H

#include <atomic>
#include <cstddef>
#include <map>
#include <assert.h>
#include "deltaPage.hpp"
#include "rwSet.hpp"
#include "segmentedVector.hpp"
#include "transaction.hpp"

template <class T>
class TransactionalVector
{
  private:
    // A page holding our shared size variable.
    // TODO: Redo size logic.
    std::atomic<Page<size_t> *> size;
    // An array of page pointers.
    SegmentedVector<Page<T> *> *array = NULL;

    // A generic ending page, used to get values.
    Page<T> *endPage = NULL;
    // A generic committed transaction.
    Desc<T> *endTransaction = NULL;

    bool reserve(size_t size);

    // Prepends a delta update on an existing page.
    // Only sets oldVal values and the next pointer here.
    bool prependPage(size_t index, Page<T> *page);

    // Takes in a set of pages and inserts them into our vector.
    // startPage is used in the helping scheme to start inserting at later pages.
    void insertPages(std::map<size_t, Page<T> *> pages, size_t startPage = 0);

    // Assign values from the readList of each operation.
    // Once complete, we can freely read values from our operations.
    //void setOperationVals(Desc<T> *descriptor, std::map<size_t, Page<T> *> pages, std::map<size_t, std::map<size_t, typename RWSet<T>::RWOperation>> operations);
    
  public:
    TransactionalVector();

    void getSize(Desc<T> *descriptor);

    // Apply a transaction to a vector.
    void executeTransaction(Desc<T> *descriptor);
};

#endif