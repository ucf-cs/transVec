#ifndef __MAIN_HEADER__
#define __MAIN_HEADER__

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../define.hpp"
#include "../randomPool.hpp"
#include "../transaction.hpp"
#include "../allocator.hpp"

// ALL GLOBAL VARIABLES ARE INITIALIZED IN main.cpp
#ifdef SEGMENTVEC
#include "../transVector.hpp"
extern TransactionalVector *transVector;
#endif

#ifdef COMPACTVEC
#include "../compactVector.hpp"
extern CompactVector *transVector;
#endif

#ifdef BOOSTEDVEC
#include "../boostedVector.hpp"
extern BoostedVector *transVector;
extern std::atomic<size_t> abortCount;
#endif

#ifdef STMVEC
#include "../vector.hpp"
extern GCCSTMVector *transVector;
#endif

#ifdef COARSEVEC
#include "../vector.hpp"
extern CoarseTransVector *transVector;
#endif

extern std::vector<Desc *> *transactions;

extern RandomNumberPool *numPool;

void executeTransactions(int threadNum);

void threadRunner(std::thread *threads, void function(int threadNum));

void preinsert(int threadNum);

void createTransactions(int threadNum);

size_t countAborts(std::vector<Desc *> *transactions);

#endif