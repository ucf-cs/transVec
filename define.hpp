#ifndef DEFINE_H
#define DEFINE_H

#define SGMT_SIZE 16

// Use this typedef to quickly change what type of objects we're working with.
typedef int VAL;

// This reserved value indicates that a value cannot be set by a read or write here.
// Must differ depending on T.
const VAL UNSET = INT32_MIN;

#endif