#ifndef TRANSVEC_HPP
#define TRANSVEC_HPP

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../define.hpp"
#include "../randomPool.hpp"
#include "../transaction.hpp"
#include "../allocator.hpp"

#ifdef SEGMENTVEC
#include "../transVector.hpp"
TransactionalVector *transVector;
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

std::vector<Desc *> transactions;

std::atomic<size_t> totalMatches;

RandomNumberPool *numPool;

std::atomic<int> counter;

#endif