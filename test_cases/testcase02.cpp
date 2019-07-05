// SECOND TESTCASE - RANGED WRITES (predicateFind)
// Preallocate a bunch of nodes and then preform a ranged series of writes
// See transaction.cpp for more detail on "write"

#include "main.hpp"

void createTransactions(TransactionalVector *transVector,
						std::vector<Desc *> *transactions,
						RandomNumberPool *numPool)
{
	// Total number of transactions
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		Operation *ops = new Operation[TRANSACTION_SIZE];

		// Each transaction will be of this size and only made of reads
		for (size_t k = 0; k < TRANSACTION_SIZE; k++)
		{
			// All operations are writes.
			ops[k].type  = Operation::OpType::write;
			ops[k].val   = 0;
			ops[k].index = (j + k) % NUM_TRANSACTIONS;
		}

		Desc *desc = new Desc(TRANSACTION_SIZE, ops);
		transactions->push_back(desc);
	}
}

int main(void)
{
	// Seed the random number generator.
	srand(time(NULL));

	// Pre-fill the allocators.
	allocatorInit();

	// Preallocate the random number generator.
	RandomNumberPool *numPool;
	numPool = new RandomNumberPool(THREAD_COUNT, NUM_TRANSACTIONS * (2 + 3 * TRANSACTION_SIZE));

	// Reserve the transaction vector, for minor performance gains.
	std::vector<Desc *> transactions;
	transactions.reserve(THREAD_COUNT);

#ifdef SEGMENTVEC
	TransactionalVector *transVector = new TransactionalVector();
#endif
#ifdef COMPACTVEC
	transVector = new CompactVector();
#endif
#ifdef COARSEVEC
	transVector = new CoarseTransVector();
#endif
#ifdef STMVEC
	transVector = new GCCSTMVector();
#endif

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, preinsert, transVector, &transactions, numPool);

	// Create the transactions that are to be executed and timed below
	createTransactions(transVector, &transactions, numPool);

	// Get start time.
	auto start = std::chrono::high_resolution_clock::now();

	// Execute the transactions
	threadRunner(threads, executeTransactions, transVector, &transactions, numPool);

	// Get end time.
	auto finish = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() << " nanoseconds\n";

	return 0;
}