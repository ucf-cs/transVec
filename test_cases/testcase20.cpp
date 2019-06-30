// TWENTITH TESTCASE - lots of small reserves with multiple threads
// Preallocate a bunch of nodes and then preform a ranged series of writes
// See transaction.cpp for more detail on "write"

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

	// Prepare to read the entire vector.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		Operation *ops = new Operation[TRANSACTION_SIZE];

		for (size_t k = 0; k < TRANSACTION_SIZE; k++)
		{
			// We'll get the 33-33-33 ratio by checking for mod 3
			if (rand() % 4 == 0)
			{
				// All operations are pushes.
				ops[k].type = Operation::OpType::pushBack;

				// Push random values into the vector.
				ops[k].val = rand();
			}
			else
			{
				ops[k].type = Operation::OpType::popBack;
			}
		}

		Desc *desc = new Desc(NUM_TRANSACTIONS, ops);
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