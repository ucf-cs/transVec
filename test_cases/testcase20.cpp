// TESTCASE 20
// RANGED HELPFREE READS
// MIX: NA
// This test case inserts a bunch of random elements and then reads then reads the entire
// vector and counts for even numbers. See transaction.cpp for more detail on "read"

#include "main.hpp"

void createTransactions()
{
	// One transaction per thread.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		size_t numOps = NUM_TRANSACTIONS * TRANSACTION_SIZE;
		Operation *ops = new Operation[numOps];
		// Generate the operations.
		for (size_t j = 0; j < numOps; j++)
		{
			// Read all elements, split among threads.
			ops[j].type = Operation::OpType::hfRead;
			// Ensure each operation accesses a unique, contiguous index.
			ops[j].index = i * numOps + j;
		}

		Desc *desc = new Desc(numOps, ops);
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
	threadRunner(threads, executeRangedTransactions);

	// Get end time and count abort(s)
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout << SGMT_SIZE << "\t" << NUM_TRANSACTIONS << "\t";
	std::cout << TRANSACTION_SIZE << "\t" << THREAD_COUNT << "\t";
    std::cout << std::chrono::duration_cast<std::chrono::TIME_UNIT>(finish-start).count();
	std::cout << "\t" << countAborts(transactions) << "\n";

	// Report on allocator issues.
	allocatorReport();

	return 0;
}