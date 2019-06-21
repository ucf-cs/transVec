#ifndef DEFINE_HPP
#define DEFINE_HPP

// These are used to switch between different vector implementations.
// Only uncomment one of them at a time.
#define SEGMENTVEC
//#define COMPACTVEC
//#define COARSEVEC
//#define STMVEC

// These are used to switch between different tests.
// Only uncomment one of them at a time.
#define PREDICATE_SEARCH
//#define RANDOM_RUN

// Change these to test different situations.
// TUNE
#define SGMT_SIZE 16
// TUNE
#define NUM_TRANSACTIONS 2500
// TUNE
#define TRANSACTION_SIZE 5
// TUNE
#define THREAD_COUNT 2
// Define this to align SEGMENTVEC pages.
//#define ALIGNED

// Compact vector requires 32-bit or smaller value types.
#ifdef COMPACTVEC
// Use this typedef to quickly change what type of objects we're working with.
// TUNE (Compile time only)
typedef int VAL;
// This reserved value indicates that a value cannot be set by a read or write here.
// Must differ depending on T.
const int UNSET = INT32_MAX;
#else
// Use this typedef to quickly change what type of objects we're working with.
// TUNE (Compile time only)
typedef short VAL;
// This reserved value indicates that a value cannot be set by a read or write here.
// Must differ depending on T.
const VAL UNSET = __SHRT_MAX__;
#endif

#endif