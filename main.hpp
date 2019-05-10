#ifndef TRANSVEC_H
#define TRANSVEC_H

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "randomPool.hpp"
#include "transVector.hpp"
#include "vector.hpp"

TransactionalVector *transVector;
//CoarseTransVector *transVector;
//GCCSTMVector *transVector;

std::vector<Desc *> transactions;

std::atomic<size_t> totalMatches;

int main(int argc, char *argv[]);

#endif