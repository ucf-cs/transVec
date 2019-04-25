#include "main.hpp"

void pushThread(int threadNum)
{
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation<VAL> *ops = new Operation<VAL>[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation<VAL>::OpType::pushBack;
			ops[j].val =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc<VAL> *desc = new Desc<VAL>(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void readThread(int threadNum)
{
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation<VAL> *ops = new Operation<VAL>[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are reads.
			ops[j].type = Operation<VAL>::OpType::read;
			// DEBUG: This will always be an invalid read. Test invalid writes too.
			ops[j].index =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc<VAL> *desc = new Desc<VAL>(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void writeThread(int threadNum)
{
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation<VAL> *ops = new Operation<VAL>[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are writes.
			ops[j].type = Operation<VAL>::OpType::write;
			ops[j].val = 0;
			ops[j].index =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc<VAL> *desc = new Desc<VAL>(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void popThread(int threadNum)
{
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation<VAL> *ops = new Operation<VAL>[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation<VAL>::OpType::popBack;
		}
		// Create a transaction containing the these operations.
		Desc<VAL> *desc = new Desc<VAL>(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void randThread(int threadNum)
{
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		size_t transSize = (rand() % TRANSACTION_SIZE) + 1;
		// A list of operations for the current thread.
		Operation<VAL> *ops = new Operation<VAL>[transSize];
		// For each operation.
		for (size_t j = 0; j < transSize; j++)
		{
			ops[j].type = Operation<VAL>::OpType(rand() % 6);
			ops[j].index = rand() % 1000;
			ops[j].val = rand() % 1000;
		}
		// Create a transaction containing the these operations.
		Desc<VAL> *desc = new Desc<VAL>(transSize, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void threadRunner(std::thread *threads, void function(int threadNum))
{
	// Start our threads.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i] = std::thread(function, i);
	}

	// Wait for all threads to complete.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i].join();
	}
	return;
}

void predicatePreinsert(int threadNum)
{
	// A list of operations for the current thread.
	Operation<VAL> *insertOps = new Operation<VAL>[NUM_TRANSACTIONS];
	// For each operation.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		// All operations are pushes.
		insertOps[j].type = Operation<VAL>::OpType::pushBack;
		// Push random values into the vector.
		insertOps[j].val = rand() % INT32_MAX;
	}
	// Create a transaction containing the these operations.
	Desc<VAL> *insertDesc = new Desc<VAL>(NUM_TRANSACTIONS, insertOps);
	// Execute the transaction.
	transVector->executeTransaction(insertDesc);
}

void predicateFind(int threadNum)
{
	// Get the transaction associated with the thread.
	Desc<VAL> *desc = transactions[threadNum];
	// Execute the transaction.
	transVector->executeTransaction(desc);
	// Get the results.
	// Busy wait until they are ready. Should never happen, but we need to be safe.
	while (desc->returnedValues.load() == false)
	{
		printf("Thread %d had to wait on returned values.\n", threadNum);
		continue;
	}
	if (desc->status.load() != Desc<VAL>::TxStatus::committed)
	{
		printf("Error on thread %d. Transaction failed.\n", threadNum);
		return;
	}
	// Check for predicate matches.
	size_t matchCount = 0;
	for (size_t i = 0; i < desc->size; i++)
	{
		// Our simple predicate: the value is even.
		if (desc->ops[i].ret % 2 == 0)
		{
			matchCount++;
		}
	}
	// Add to the global total.
	totalMatches.fetch_add(matchCount);
	return;
}

// Insert random elements into the vector and count the number of elements that satisfy the predicate.
void predicateSearch()
{
	// Ensure we start with no matches.
	totalMatches.store(0);

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, predicatePreinsert);

	// Prepare read transactions for each thread.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		Operation<VAL> *ops = new Operation<VAL>[NUM_TRANSACTIONS];
		// Prepare to read the entire vector.
		for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
		{
			// Read all elements, split among threads.
			ops[j].type = Operation<VAL>::OpType::read;
			ops[j].index = i * NUM_TRANSACTIONS + j;
		}
		Desc<VAL> *desc = new Desc<VAL>(NUM_TRANSACTIONS, ops);
		transactions.push_back(desc);
	}

	printf("Completed preinsertion!\n\n\n");

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	// Run the threads.
	threadRunner(threads, predicateFind);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	//transVector->printContents();

	std::cout << "" << THREAD_COUNT << " threads and " << NUM_TRANSACTIONS << " locations per thread" << std::endl;
	std::cout << total.count() << " milliseconds" << std::endl;

	printf("Total: %lu matched out of %lu\n", totalMatches.load(), THREAD_COUNT * NUM_TRANSACTIONS);
}

int main(int argc, char *argv[])
{
	// Use command line arguments to quickly test the vector in a variety of scenarios.
	THREAD_COUNT = atol(argv[1]);
	NUM_TRANSACTIONS = atol(argv[2]);

	// Seed the random number generator.
	srand(time(NULL));

	transVector = new TransactionalVector<VAL>();
	//transVector = new CoarseTransVector<VAL>();
	//transVector = new GCCSTMVector<VAL>();

	printf("Largest page is %lu bytes.\n", sizeof(DeltaPage<VAL, VAL, SGMT_SIZE, SGMT_SIZE>));

	//predicateSearch();
	//return 0;

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	threadRunner(threads, randThread);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	transVector->printContents();

	std::cout << "" << THREAD_COUNT << " threads and " << TRANSACTION_SIZE << " operations per transaction" << std::endl;
	std::cout << total.count() << " milliseconds" << std::endl;

	return 0;
}