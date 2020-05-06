// TESTCASE 17
// PUSH-POP
// MIX: 75-25

#include "main.hpp"

void createTransactions()
{
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		Operation *ops = new Operation[TRANSACTION_SIZE];

		for (size_t k = 0; k < TRANSACTION_SIZE; k++)
		{
			if (rand() % 4 != 0)
			{
				// All operations are pushes.
				ops[k].type = Operation::OpType::pushBack;

				// Push random values into the vector.
				ops[k].val = rand() % std::numeric_limits<VAL>::max();
			}
			else
			{
				ops[k].type = Operation::OpType::popBack;
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
	threadRunner(threads, executeTransactions);

	// Get end time and count abort(s)
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout << SGMT_SIZE << "\t" << NUM_TRANSACTIONS << "\t";
	std::cout << TRANSACTION_SIZE << "\t" << THREAD_COUNT << "\t";
    std::cout << std::chrono::duration_cast<std::chrono::TIME_UNIT>(finish-start).count();
#ifdef METRICS
	std::cout << "Average preprocessing time" + std::chrono::duration_cast<std::chrono::TIME_UNIT>(measurePreprocessTime(transactions)).count();
	std::cout << "Average shared memory time" + std::chrono::duration_cast<std::chrono::TIME_UNIT>(measureSharedTime(transactions)).count();
	std::cout << "Average transaction time" + std::chrono::duration_cast<std::chrono::TIME_UNIT>(measureTotalTime(transactions)).count();
#endif
	std::cout << "\t" << countAborts(transactions) << "\n";

	// Report on allocator issues.
	allocatorReport();

	return 0;
}