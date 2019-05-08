#ifndef DEFINE_H
#define DEFINE_H

// TUNE
#define SGMT_SIZE 16

// Change these to test different situations.
// TUNE
static size_t THREAD_COUNT = 12;
// TUNE
static const size_t TRANSACTION_SIZE = 5;
// TUNE
static size_t NUM_TRANSACTIONS = 2500000;

// Use this typedef to quickly change what type of objects we're working with.
typedef size_t VAL;

// This reserved value indicates that a value cannot be set by a read or write here.
// Must differ depending on T.
const VAL UNSET = SIZE_MAX;

#endif