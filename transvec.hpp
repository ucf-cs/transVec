#ifndef TRANSVEC_H
#define TRANSVEC_H

#include <chrono>
#include <iostream>
#include <thread>
#include "deltaPage.hpp"
#include "pagedVector.hpp"
#include "rwSet.hpp"
#include "transaction.hpp"
using namespace std;

// Change these to test different situations.
// TUNE
size_t THREAD_COUNT = 24;
// TUNE
size_t TRANSACTION_SIZE = 1;
// TUNE
size_t NUM_TRANSACTIONS = 10000;

#endif