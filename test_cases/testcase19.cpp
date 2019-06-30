// NINETEENTH TESTCASE - One major txn with a huge reserve. Each thread gets a txn
// We're not preinserting for this testcase

#include "main.hpp"

// Insert random elements into the vector then preforms a bunch of reads
void createTransactions(TransactionalVector *transVector,
						std::vector<Desc *> *transactions,
						RandomNumberPool *numPool)
{
	// Prepare a txn size of 1. 1 txn for each thread.
	for (size_t j = 0; j < THREAD_COUNT; j++)
	{
		Operation *ops = new Operation[1];

		// All operations are reserve.
		ops[0].type = Operation::OpType::reserve;

		// Push random values into the vector.
		ops[0].index = NUM_TRANSACTIONS / THREAD_COUNT;

		Desc *desc = new Desc(NUM_TRANSACTIONS, ops);
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
	threadRunner(threads, preinsert, transVector, transactions, numPool);

	// Create the transactions that are to be executed and timed below
	createTransactions(transVector, &transactions, numPool);

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	// Execute the transactions
	threadRunner(threads, executeTransactions, transVector, transactions, numPool);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	std::cout << total.count() << " milliseconds" << std::endl;

	return 0;
}