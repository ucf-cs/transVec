// This file includes methods that will be shared between all test cases

#include "main.hpp"

// All global variables are initialized here
std::vector<Desc *> *transactions = new std::vector<Desc *>();
#ifdef SEGMENTVEC
TransactionalVector *transVector = new TransactionalVector();
#endif
#ifdef COMPACTVEC
CompactVector *transVector = new CompactVector();
#endif
#ifdef BOOSTEDVEC
BoostedVector *transVector = new BoostedVector();
std::atomic<size_t> abortCount(0);
#endif
#ifdef STMVEC
GCCSTMVector *transVector = new GCCSTMVector();
#endif
#ifdef COARSEVEC
CoarseTransVector *transVector = new CoarseTransVector();
#endif
#ifdef STOVEC
STOVector *transVector = new STOVector();
#endif

// Input: 1- Array of threads that will execute a fucntion.
//        2- A function pointer to that function.
void threadRunner(std::thread *threads, void function(int threadNum))
{
	// Start our threads.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i] = std::thread(function, i);
	}

	// Set thread affinity.
	for (unsigned i = 0; i < THREAD_COUNT; ++i)
	{
		// Create a cpu_set_t object representing a set of CPUs. Clear it and mark only CPU i as set.
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(i, &cpuset);
		int rc = pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
		if (rc != 0)
		{
			std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
		}
	}

	// Wait for all threads to complete.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i].join();
	}

	return;
}

// Goes through the vector of transactions and executes them.
void executeTransactions(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// Each thread is allocated an interval to work on
	int start = transactions->size() / THREAD_COUNT * threadNum;
	int end = transactions->size() / THREAD_COUNT * (threadNum + 1);

	for (int i = start; i < end; i++)
	{
		Desc *desc = transactions->at(i);
#ifndef BOOSTEDVEC
		transVector->executeTransaction(desc);
#else
		if (!transVector->executeTransaction(desc))
		{
			abortCount.fetch_add(1);
		}
#endif
	}
}

void executeRangedTransactions(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	Desc *desc = transactions->at(threadNum);
#ifndef BOOSTEDVEC
	transVector->executeTransaction(desc);
#else
	if (!transVector->executeTransaction(desc))
	{
		abortCount.fetch_add(1);
	}
#endif
}

// Prepushes a bunch of objects into the vector
void preinsert(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	int opsPerThread = NUM_TRANSACTIONS / THREAD_COUNT;

	// Ensure reserves have already completed to prevent them from affecting performance.
	Operation *reserveOp = new Operation[1];
	reserveOp->type = Operation::OpType::reserve;
	reserveOp->index = NUM_TRANSACTIONS;
	Desc *reserveDesc = new Desc(1, reserveOp);
	transVector->executeTransaction(reserveDesc);

	// A list of operations for the current thread.
	Operation *pushOps = new Operation[opsPerThread];

	// For each operation.
	for (int j = 0; j < opsPerThread; j++)
	{
		// All operations are pushes.
		pushOps[j].type = Operation::OpType::pushBack;

		// Push random values into the vector.
		VAL val = UNSET;
		// Ensure we never get an UNSET.
		while (val == UNSET)
		{
			val = rand() % std::numeric_limits<VAL>::max();
		}
		pushOps[j].val = val;
	}

	// Create a transaction containing the these operations.
	Desc *pushDesc = new Desc(opsPerThread, pushOps);

// Execute the transaction.
#ifndef BOOSTEDVEC
	transVector->executeTransaction(pushDesc);
	if (pushDesc->status.load() != Desc::TxStatus::committed)
	{
		printf("Preinsert failed.\n");
		return;
	}
#else
	if (!transVector->executeTransaction(pushDesc))
	{
		printf("Preinsert failed.\n");
		return;
	}
#endif
	return;
}

size_t countAborts([[maybe_unused]] std::vector<Desc *> *transactions)
{
#ifdef BOOSTEDVEC
	return abortCount.load();
#else
	size_t retVal = 0;
	for (size_t i = 0; i < transactions->size(); i++)
	{
		Desc *desc = transactions->at(i);

		if (desc->status.load() == Desc::TxStatus::aborted)
			retVal++;
	}
	return retVal;
#endif
}

#ifdef METRICS
std::chrono::high_resolution_clock::duration measurePreprocessTime([[maybe_unused]] std::vector<Desc *> *transactions)
{
	std::chrono::high_resolution_clock::duration retVal = std::chrono::high_resolution_clock::duration::zero();
	for (size_t i = 0; i < transactions->size(); i++)
	{
		Desc *desc = transactions->at(i);
		retVal += desc->preprocessTime - desc->startTime;
	}
	retVal /= transactions->size();
	return retVal;
}
std::chrono::high_resolution_clock::duration measureSharedTime([[maybe_unused]] std::vector<Desc *> *transactions)
{
	std::chrono::high_resolution_clock::duration retVal = std::chrono::high_resolution_clock::duration::zero();
	for (size_t i = 0; i < transactions->size(); i++)
	{
		Desc *desc = transactions->at(i);
		retVal += desc->endTime - desc->preprocessTime;
	}
	retVal /= transactions->size();
	return retVal;
}
std::chrono::high_resolution_clock::duration measureTotalTime([[maybe_unused]] std::vector<Desc *> *transactions)
{
	std::chrono::high_resolution_clock::duration retVal = std::chrono::high_resolution_clock::duration::zero();
	for (size_t i = 0; i < transactions->size(); i++)
	{
		Desc *desc = transactions->at(i);
		retVal += desc->endTime - desc->startTime;
	}
	retVal /= transactions->size();
	return retVal;
}
#endif

int setMaxPriority()
{
	int which = PRIO_PROCESS;
	id_t pid;
	int priority = -20;
	int ret;

	pid = getpid();
	ret = setpriority(which, pid, priority);
	return ret;
}