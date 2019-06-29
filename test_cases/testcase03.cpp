// THIRD TESTCASE - RANDOM TRANSACTION SIZE WITH RANDOM OPERATIONS
// transactions of random length (determined by the rand() functions)
// All operations are of equal chance to be chosen
#include "main.hpp"

// Insert random elements into the vector then preforms a bunch of reads
void createTransactions(TransactionalVector *transVector,
						std::vector<Desc *> transactions,
						RandomNumberPool *numPool)
{
	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, preinsert, transVector, transactions, numPool);
	printf("Completed preinsertion!\n\n");

	// Create a random transactions of random operations and random size
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		// The transaction size is between 1 and TRANSACTION_SIZE, chosen at random.
		size_t txnSize = (rand() % TRANSACTION_SIZE) + 1;

		// A list of operations for the current thread.
		Operation *ops = new Operation[txnSize];

		// For each operation.
		for (size_t k = 0; k < txnSize; k++)
		{
			ops[k].type = Operation::OpType(rand() % 6);
			ops[k].index = rand() % NUM_TRANSACTIONS;
			ops[k].val = rand() % 1000;
		}

		// Create a transaction containing the these operations.
		Desc *desc = new Desc(txnSize, ops);

		// Add the transaction to the list.
		transactions.push_back(desc);
	}

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	// Run the threads.
	threadRunner(threads, executeTransactions, transVector, transactions, numPool);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	std::cout << "" << THREAD_COUNT << " threads and " << NUM_TRANSACTIONS << " locations per thread" << std::endl;
	std::cout << total.count() << " milliseconds" << std::endl;
}

int main(void)
{
	// Seed the random number generator.
	srand(time(NULL));

	// Pre-fill the allocators.
	allocatorInit();

	// Preallocate the random number generator.
	printf("Generating random numbers.\n");
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

	createTransactions(transVector, transactions, numPool);

	return 0;
}