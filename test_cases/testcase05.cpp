// FIRST TESTCASE - RANDOM WRITES (predicateFind)
// Preallocate a bunch of nodes and then preform random writes
// See transaction.cpp for more detail on "write"

#include "main.hpp"

// Insert random elements into the vector then preforms a bunch of reads
void createTransactions(TransactionalVector *transVector,
						std::vector<Desc *> *transactions,
						RandomNumberPool *numPool)
{
	// Prepare write transactions for each thread.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		Operation *ops = new Operation[TRANSACTION_SIZE];

		for (size_t k = 0; k < TRANSACTION_SIZE; k++)
		{
			// All operations are writes.
			ops[k].type  = Operation::OpType::write;
			ops[k].val   = rand() % MAX_VAL;
			ops[k].index = rand() % MAX_VAL;
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