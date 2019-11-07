// TESTCASE 2
// RANGED WRITES
// MIX: NA

#include "main.hpp"

void createTransactions()
{
	// One transaction per thread.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		size_t numOps = NUM_TRANSACTIONS / THREAD_COUNT;
		Operation *ops = new Operation[numOps];
		// Generate the operations.
		for (size_t j = 0; j < numOps; j++)
		{
			// Read all elements, split among threads.
			ops[j].type = Operation::OpType::write;
			// Ensure each operation accesses a unique, contiguous index.
			ops[j].index = i * numOps + j;
			// Value doesn't really matter, but we might as well keep them unique.
			ops[j].val = i * numOps + j;
		}

		Desc *desc = new Desc(numOps, ops);
#ifdef CONFLICT_FREE_READS
		desc->isConflictFree = true;
#endif
		transactions->push_back(desc);
	}
}

int main(void)
{
	// Seed the random number generator.
	srand(time(NULL));

	// Ensure the test process runs at maximum priority.
	// Only works if run under sudo permissions.
	setMaxPriority();

	// Pre-fill the allocators.
	allocatorInit();

	// Reserve the transaction vector, for minor performance gains.
	transactions->reserve(THREAD_COUNT);

	// Create our threads.
	std::thread threads[THREAD_COUNT];

		// Pre-insertion step.
	//threadRunner(threads, preinsert);
	// Single-threaded alternative.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		preinsert(i);
	}

	// Create the transactions that are to be executed and timed below
	createTransactions();

	// Get start time.
	auto start = std::chrono::high_resolution_clock::now();

	// Execute the transactions
	threadRunner(threads, executeRangedTransactions);

	// Get end time and count abort(s)
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout << SGMT_SIZE << "\t" << NUM_TRANSACTIONS << "\t";
	std::cout << TRANSACTION_SIZE << "\t" << THREAD_COUNT << "\t";
	std::cout << std::chrono::duration_cast<std::chrono::TIME_UNIT>(finish - start).count();
	std::cout << "\t" << countAborts(transactions) << "\n";

	// Report on allocator issues.
	allocatorReport();

	return 0;
}