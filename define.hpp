#ifndef DEFINE_HPP
#define DEFINE_HPP

// These are used to switch between different vector implementations.
// Only uncomment one of them at a time. **Make sure they're all commented if using the test-all.sh script**
// **It's important to keep these comments as "//#define" instead of "// define"**
//#define SEGMENTVEC
//#define COMPACTVEC
//#define COARSEVEC
//#define STMVEC

// These are used to switch between different tests.
// Only uncomment one of them at a time.
#define PREDICATE_SEARCH
//#define RANDOM_RUN
#ifdef SEGMENTVEC
#define HELP_FREE_READS
#endif

// Change these to test different situations.
// Makes sense to make this cache line size times associativity (perhaps at L2 level, so 8*16)
// Divide by the size of the elements in the segment an by 2, so we can hold old and new values on the same cache line.
// NOTE: This may break if division makes this resolve to 0.
// TUNE
#define SGMT_SIZE ((8 * 16) / (sizeof(VAL) * 2))
// TUNE
#define NUM_TRANSACTIONS 10000
// TUNE
// #define TRANSACTION_SIZE 5
// TUNE
// #define THREAD_COUNT 2
// Define this to enable the helping scheme.
#define HELP

#ifdef SEGMENTVEC
// Define this to align SEGMENTVEC pages.
//#define ALIGNED
// Define this to make compact vector store only transaction pointers.
//#define HELP_FREE_READS
#endif

// TODO2: Maybe implement this.
//#define POINTER_ONLY

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
typedef unsigned int VAL;
// This reserved value indicates that a value cannot be set by a read or write here.
// Must differ depending on T.
const VAL UNSET = UINT32_MAX;
#endif

#endif
