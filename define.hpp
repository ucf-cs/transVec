#ifndef DEFINE_HPP
#define DEFINE_HPP

// TUNE (Compile time only)
static const size_t SGMT_SIZE = 16;

// Change these to test different situations.
// TUNE
static size_t NUM_TRANSACTIONS = 250000;
// TUNE
static size_t TRANSACTION_SIZE = 5;
// TUNE
static size_t THREAD_COUNT = 6;

// Use this typedef to quickly change what type of objects we're working with.
// TUNE (Compile time only)
typedef short VAL;
// This reserved value indicates that a value cannot be set by a read or write here.
// Must differ depending on T.
const VAL UNSET = __SHRT_MAX__;

#endif