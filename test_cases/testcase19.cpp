// TESTCASE 19
// MEMORY RESERVE
// MIX: NA
// We're not preinserting for this testcase

#include "main.hpp"

void createTransactions()
{
	for (size_t j = 0; j < THREAD_COUNT; j++)
	{
		Operation *ops = new Operation[1];

		// All operations are reserve.
		ops[0].type = Operation::OpType::reserve;

		// Reserve up to a large range.
		ops[0].index = 1000000;

		Desc *desc = new Desc(1, ops);
#ifdef CONFLICT_FREE_READS
		desc->isConflictFree = false;
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

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Create the transactions that are to be executed and timed below
	createTransactions();

	// Get start time.
	auto start = std::chrono::high_resolution_clock::now();

	// Execute the transactions
	threadRunner(threads, executeTransactions);

	// Get end time and count abort(s)
	auto finish = std::chrono::high_resolution_clock::now();

	auto preprocess = measurePreprocessTime(transactions);
	auto shared = measureSharedTime(transactions);
	auto total = measureTotalTime(transactions);

	std::cout << SGMT_SIZE << "\t" << NUM_TRANSACTIONS << "\t";
	std::cout << TRANSACTION_SIZE << "\t" << THREAD_COUNT << "\t";
	std::cout << std::chrono::duration_cast<std::chrono::TIME_UNIT>(finish - start).count();
	std::cout << "\t" << countAborts(transactions);
#ifdef METRICS
	std::cout << "\t" << preprocess.count();
	std::cout << "\t" << shared.count();
	std::cout << "\t" << total.count();
#endif
	std::cout << "\n";

	// Report on allocator issues.
	allocatorReport();

	return 0;
}