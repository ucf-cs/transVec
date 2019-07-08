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

// #ifdef SEGMENTVEC
// #include "../transVector.hpp"
// TransactionalVector *transVector;
// #endif

// #ifdef STMVEC
// #include "../vector.hpp"
// GCCSTMVector *transVector;
// #endif

// #ifdef COARSEVEC
// #include "../vector.hpp"
// CoarseTransVector *transVector;
// #endif

// #ifdef COMPACTVEC
// #include "../compactVector.hpp"
// CompactVector *transVector;
// #endif

void executeTransactions(int threadNum,
						 TransactionalVector *transVector,
						 std::vector<Desc *> *transactions,
						 RandomNumberPool *numPool);

void threadRunner(std::thread *threads,
				  void function(int threadNum,
								TransactionalVector *transVector,
								std::vector<Desc *> *transactions,
								RandomNumberPool *numPool),
				  TransactionalVector *transVector,
				  std::vector<Desc *> *transactions,
				  RandomNumberPool *numPool);

void preinsert(int threadNum,
			   TransactionalVector *transVector,
			   std::vector<Desc *> *transactions,
			   RandomNumberPool *numPool);

int countAborts(std::vector<Desc *> *transactions);

#endif