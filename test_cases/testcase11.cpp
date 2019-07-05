// ELEVENTH TESTCASE - RANDOM "SLOW" (push, pop, write) and "FAST" (read, write) operations
// There will be a 50-50 ratio of slow to fast operations
// The slow operations will be 33-33-33 ratio
// the fast operations will be 50-50

#include "main.hpp"

void createTransactions(TransactionalVector *transVector,
						std::vector<Desc *> *transactions,
						RandomNumberPool *numPool)
{
	// Prepare to read the entire vector.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		Operation *ops = new Operation[TRANSACTION_SIZE];

		for (size_t k = 0; k < TRANSACTION_SIZE; k++)
		{
			if (rand() % 2 == 0)
			{
				int r = rand();

				// We'll get the 33-33-33 ratio by checking for mod 3
				if (r % 3 == 0)
				{
					// All operations are pushes.
					ops[k].type = Operation::OpType::pushBack;

					// Push random values into the vector.
					ops[k].val = rand();
				}
				else if (r % 3 == 1)
				{
					ops[k].type = Operation::OpType::popBack;
				}
				else
				{
					ops[k].type = Operation::OpType::size;
				}
			}
			else
			{
				// We'll get the 50/50 ratio by checking for even or odd
				// If even, make a write operation, else make a read operation
				if (rand() % 2 == 0)
				{
					// All operations are writes.
					ops[k].type  = Operation::OpType::write;
					ops[k].val   = 0;
					ops[k].index = rand() % (NUM_TRANSACTIONS / 10);
				}
				else
				{
					// Read all elements, split among threads.
					ops[k].type = Operation::OpType::read;
					ops[k].index = rand() % (NUM_TRANSACTIONS / 10);
				}
			}
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