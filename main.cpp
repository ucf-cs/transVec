#include "main.hpp"

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

void pushThread(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation *ops = new Operation[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation::OpType::pushBack;
			ops[j].val =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc *desc = new Desc(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void readThread(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation *ops = new Operation[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are reads.
			ops[j].type = Operation::OpType::read;
			// DEBUG: This will always be an invalid read. Test invalid writes too.
			ops[j].index =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc *desc = new Desc(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void writeThread(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation *ops = new Operation[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are writes.
			ops[j].type = Operation::OpType::write;
			ops[j].val = 0;
			ops[j].index =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc *desc = new Desc(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void popThread(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation *ops = new Operation[TRANSACTION_SIZE];
		// For each operation.
		for (size_t j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation::OpType::popBack;
		}
		// Create a transaction containing the these operations.
		Desc *desc = new Desc(TRANSACTION_SIZE, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void randThread(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);

	RandomNumberPool *numPool = new RandomNumberPool(NUM_TRANSACTIONS * (1 + (3 * TRANSACTION_SIZE)));

	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		size_t transSize = (rand() % TRANSACTION_SIZE) + 1;
		// A list of operations for the current thread.
		Operation *ops = new Operation[transSize];
		// For each operation.
		for (size_t j = 0; j < transSize; j++)
		{
			ops[j].type = Operation::OpType(rand() % 6);
			ops[j].index = rand() % 1000;
			ops[j].val = rand() % 1000;
		}
		// Create a transaction containing the these operations.
		Desc *desc = new Desc(transSize, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void predicatePreinsert(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);
	// A list of operations for the current thread.
	Operation *insertOps = new Operation[NUM_TRANSACTIONS];
	// For each operation.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		// All operations are pushes.
		insertOps[j].type = Operation::OpType::pushBack;
		// Push random values into the vector.
		insertOps[j].val = rand() % INT32_MAX;
	}
	// Create a transaction containing the these operations.
	Desc *insertDesc = new Desc(NUM_TRANSACTIONS, insertOps);
	// Execute the transaction.
	transVector->executeTransaction(insertDesc);
	return;
}

void predicateFind(int threadNum)
{
	// Initialize the allocators.
	Allocator<Page>::threadInit(threadNum);
	Allocator<RWOperation>::threadInit(threadNum);
	// Get the transaction associated with the thread.
	Desc *desc = transactions[threadNum];
	// Execute the transaction.
	transVector->executeTransaction(desc);
	if (desc->status.load() != Desc::TxStatus::committed)
	{
		printf("Error on thread %d. Transaction failed.\n", threadNum);
		return;
	}
	// Get the results.
	// Busy wait until they are ready. Should never happen, but we need to be safe.
	while (desc->returnedValues.load() == false)
	{
		printf("Thread %d had to wait on returned values.\n", threadNum);
		continue;
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
	printf("Completed preinsertion!\n\n\n");

	// Prepare read transactions for each thread.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		Operation *ops = new Operation[NUM_TRANSACTIONS];
		// Prepare to read the entire vector.
		for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
		{
			// Read all elements, split among threads.
			ops[j].type = Operation::OpType::read;
			ops[j].index = i * NUM_TRANSACTIONS + j;
		}
		Desc *desc = new Desc(NUM_TRANSACTIONS, ops);
		transactions.push_back(desc);
	}

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
	// Seed the random number generator.
	srand(time(NULL));

	// Preallocate the pages.
	Allocator<Page>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE);
	// Preallocate the RWSet elements.
	Allocator<RWOperation>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE*2);

	transVector = new TransactionalVector();
	//transVector = new CoarseTransVector();
	//transVector = new GCCSTMVector();

	printf("Pages are %lu bytes.\n", sizeof(Page));

	//predicateSearch();
	//return 0;

	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	threadRunner(threads, predicatePreinsert);
	printf("Completed preinsertion!\n\n\n");

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	threadRunner(threads, randThread);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	//transVector->printContents();

	std::cout << "" << THREAD_COUNT << " threads and " << TRANSACTION_SIZE << " operations per transaction" << std::endl;
	std::cout << total.count() << " milliseconds" << std::endl;

	return 0;
}