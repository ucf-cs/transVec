// TESTCASE 14
// SLOW-FAST
// MIX: 25-75

#include "main.hpp"

void createTransactions()
{
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		Operation *ops = new Operation[TRANSACTION_SIZE];

		for (size_t k = 0; k < TRANSACTION_SIZE; k++)
		{
			// 75 slow, 25 fast
			if (rand() % 4 == 0)
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
					ops[k].val   = rand() % std::numeric_limits<VAL>::max();
					ops[k].index = rand() % (NUM_TRANSACTIONS / 2);
				}
				else
				{
					// Read all elements, split among threads.
					ops[k].type = Operation::OpType::read;
					ops[k].index = rand() % (NUM_TRANSACTIONS / 2);
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

	return 0;
}