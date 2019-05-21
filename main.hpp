#ifndef TRANSVEC_HPP
#define TRANSVEC_HPP

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "randomPool.hpp"
#include "transVector.hpp"
#include "vector.hpp"

TransactionalVector *transVector;
// TODO: Make the compact vector functional.
//CompactVector *transVector;
//CoarseTransVector *transVector;
//GCCSTMVector *transVector;

std::vector<std::vector<Desc *>> transactions;

std::atomic<size_t> totalMatches;

RandomNumberPool *numPool;

int main(int argc, char *argv[]);

#endif