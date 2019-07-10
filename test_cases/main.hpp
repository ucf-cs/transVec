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

#ifdef STMVEC
#include "../vector.hpp"
GCCSTMVector *transVector;
#endif

#ifdef COARSEVEC
#include "../vector.hpp"
CoarseTransVector *transVector;
#endif

#ifdef COMPACTVEC
#include "../compactVector.hpp"
CompactVector *transVector;
#endif

extern std::vector<Desc *> *transactions;

extern RandomNumberPool *numPool;

void executeTransactions(int threadNum);

void threadRunner(std::thread *threads, void function(int threadNum));

void preinsert(int threadNum);

void createTransactions(int threadNum);

int countAborts(std::vector<Desc *> *transactions);

#endif