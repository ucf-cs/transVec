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
const size_t THREAD_COUNT = 8;
// TUNE
const size_t TRANSACTION_SIZE = 5;
// TUNE
const size_t NUM_TRANSACTIONS = 5000;

TransactionalVector<int> *transVector;

std::vector<Desc<int> *> transactions;

std::atomic<size_t> totalMatches;

int main(int argc, char *argv[]);

#endif