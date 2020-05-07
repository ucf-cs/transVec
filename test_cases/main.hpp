#ifndef __MAIN_HEADER__
#define __MAIN_HEADER__

#define TIME_UNIT nanoseconds

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../define.hpp"
//#include "../randomPool.hpp"
#include "../transaction.hpp"
#include "../allocator.hpp"
#include "../threadLocalGlobals.hpp"

// Used to set process priority in Linux.
#include <sys/resource.h>
#include <unistd.h>

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

#ifdef STOVEC
#include "../STOVec.hpp"
extern STOVector *transVector;
#endif

#ifdef COARSEVEC
#include "../vector.hpp"
extern CoarseTransVector *transVector;
#endif

extern std::vector<Desc *> *transactions;

void executeTransactions(int threadNum);

void executeRangedTransactions(int threadNum);

void threadRunner(std::thread *threads, void function(int threadNum));

void preinsert(int threadNum);

void createTransactions(int threadNum);

size_t countAborts(std::vector<Desc *> *transactions);

#ifdef METRICS
std::chrono::TIME_UNIT measurePreprocessTime([[maybe_unused]] std::vector<Desc *> *transactions);
std::chrono::TIME_UNIT measureSharedTime([[maybe_unused]] std::vector<Desc *> *transactions);
std::chrono::TIME_UNIT measureTotalTime([[maybe_unused]] std::vector<Desc *> *transactions);
#endif

int setMaxPriority();

#endif