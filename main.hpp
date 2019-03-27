#ifndef TRANSVEC_H
#define TRANSVEC_H

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include "deltaPage.hpp"
#include "transaction.hpp"
#include "transVector.hpp"

using namespace std;

// Change these to test different situations.
// TUNE
const size_t THREAD_COUNT = 12;
// TUNE
const size_t TRANSACTION_SIZE = 5;
// TUNE
const size_t NUM_TRANSACTIONS = 5;

TransactionalVector<int> *transVector;

int main(int argc, char *argv[]);

#endif