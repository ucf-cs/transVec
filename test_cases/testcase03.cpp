// THIRD TESTCASE - RANDOM TRANSACTION SIZE WITH RANDOM OPERATIONS
// transactions of random length (determined by the rand() functions)
// All operations are of equal chance to be chosen

#include "main.hpp"

void createTransactions()
{
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
			ops[k].index = rand() % NUM_TRANSACTIONS / 2;
			ops[k].val = rand() % std::numeric_limits<VAL>::max();;
		}

		// Create a transaction containing these operations.
		Desc *desc = new Desc(txnSize, ops);

		// Add the transaction to the vector.
		transactions->push_back(desc);
	}
}

int main(void)
{
	// Seed the random number generator.
	srand(time(NULL));

	// Pre-fill the allocators.
	allocatorInit();

	// Reserve the transaction vector, for minor performance gains.
	transactions->reserve(THREAD_COUNT);

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, preinsert);

	// Create the transactions that are to be executed and timed below
	createTransactions();

	// Get start time.
	auto start = std::chrono::high_resolution_clock::now();

	// Execute the transactions
	threadRunner(threads, executeTransactions);

	// Get end time and count abort(s)
	auto finish = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
	std::cout << "ns with " << countAborts(transactions) << " abort(s)\n";

	// Report on allocator issues.
	allocatorReport();

	return 0;
}