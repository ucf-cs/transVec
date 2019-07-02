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

// Initialize per-thread allocators.
void inline threadAllocatorInit(int threadNum)
{
#ifdef SEGMENTVEC
	Allocator<Page<VAL, SGMT_SIZE>>::threadInit(threadNum);
	Allocator<Page<size_t, 1>>::threadInit(threadNum);
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::threadInit(threadNum);
#endif
#ifdef COMPACTVEC
	Allocator<CompactElement>::threadInit(threadNum);
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC)
	Allocator<RWOperation>::threadInit(threadNum);
	Allocator<RWSet>::threadInit(threadNum);
#endif
	return;
}

void pushThread(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

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
	threadAllocatorInit(threadNum);

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
	threadAllocatorInit(threadNum);

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
	threadAllocatorInit(threadNum);

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

void randThreadInit(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// A vector containing all of a thread's transactions.
	std::vector<Desc *> threadTransactions;

	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// The transaction size is between 1 and TRANSACTION_SIZE, chosen at random.
		size_t transSize = (numPool->getNum(threadNum) % TRANSACTION_SIZE) + 1;
		// A list of operations for the current thread.
		Operation *ops = new Operation[transSize];
		// For each operation.
		for (size_t j = 0; j < transSize; j++)
		{
			ops[j].type = Operation::OpType(numPool->getNum(threadNum) % 6);
			ops[j].index = numPool->getNum(threadNum) % 1000;
			ops[j].val = numPool->getNum(threadNum) % 1000;
		}
		// Create a transaction containing the these operations.
		Desc *desc = new Desc(transSize, ops);
		// Add the transaction to the list.
		threadTransactions.push_back(desc);
	}
	// Place this thread's transactions in the list.
	transactions[threadNum] = threadTransactions;
}

void randThread(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// Execute the transaction.
		transVector->executeTransaction(transactions[threadNum][i]);
	}
	return;
}

void predicatePreinsert(int threadNum)
{
	// Initialize the allocators.
	threadAllocatorInit(threadNum);

	// A list of operations for the current thread.
	Operation *insertOps = new Operation[NUM_TRANSACTIONS];
	// For each operation.
	for (size_t j = 0; j < NUM_TRANSACTIONS; j++)
	{
		// All operations are pushes.
		insertOps[j].type = Operation::OpType::pushBack;
		// Push random values into the vector.
		insertOps[j].val = numPool->getNum(threadNum) % std::numeric_limits<VAL>::max();
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
	threadAllocatorInit(threadNum);

	// Get the transaction associated with the thread.
	Desc *desc = transactions[threadNum][0];
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
		std::vector<Desc *> threadTransactions;
		threadTransactions.push_back(desc);
		transactions.push_back(threadTransactions);
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

	printf("Total: %lu matched out of %lu\n", totalMatches.load(), (size_t)THREAD_COUNT * NUM_TRANSACTIONS);
}

// Perform several random operations.
void randomRun()
{
	// Create our threads.
	std::thread threads[THREAD_COUNT];

	// Pre-insertion step.
	printf("Preinserting.\n");
	threadRunner(threads, predicatePreinsert);

	// Generate all of the random transactions.
	printf("Generating transactions.\n");
	threadRunner(threads, randThreadInit);

	printf("Done!\n");

	// Get the current time.
	auto start = std::chrono::system_clock::now();

	threadRunner(threads, randThread);

	// Get total execution time.
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

	std::cout << NUM_TRANSACTIONS << " operations per thread with "
			  << THREAD_COUNT << " threads and "
			  << TRANSACTION_SIZE << " operations per transaction"
			  << std::endl;
	std::cout << total.count() << " milliseconds" << std::endl;
}

// Help-free test.
void helpFreeSearch()
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
			ops[j].type = Operation::OpType::hfRead;
			ops[j].index = i * NUM_TRANSACTIONS + j;
		}
		Desc *desc = new Desc(NUM_TRANSACTIONS, ops);
		std::vector<Desc *> threadTransactions;
		threadTransactions.push_back(desc);
		transactions.push_back(threadTransactions);
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

	printf("Total: %lu matched out of %lu\n", totalMatches.load(), (size_t)THREAD_COUNT * NUM_TRANSACTIONS);
}

void allocatorInit()
{
	printf("Initializing the memory allocators.\n");
	printf("Operation* allocator.\n");
	MemAllocator<Operation *>::init();
	// NOTE: Other MemAllocators are implicitly initialized.
	// Would be better to initialize them in advance for performance.
	// It's not a big deal if we pre-fill the vector first.
	printf("Finished initializing the memory allocators.\n\n");

#ifdef SEGMENTVEC
	// Preallocate the pages.
	printf("sizeof(Page<VAL, SGMT_SIZE>)=%lu\n", sizeof(Page<VAL, SGMT_SIZE>));
	Allocator<Page<VAL, SGMT_SIZE>>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
	// Preallocate the size pages.
	printf("sizeof(Page<VAL, SGMT_SIZE>)=%lu\n", sizeof(Page<size_t, 1>));
	Allocator<Page<size_t, 1>>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
	// Preallocate page maps.
	printf("sizeof(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>)=%lu\n", sizeof(std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>));
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
#ifdef COMPACTVEC
	// Preallocate compact elements.
	printf("sizeof(CompactElement)=%lu\n", sizeof(CompactElement));
	Allocator<CompactElement>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
#ifdef COMPACTVEC
	// Preallocate the RWOperation elements.
	printf("sizeof(RWOperation)=%lu\n", sizeof(RWOperation));
	Allocator<RWOperation>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT * THREAD_COUNT);
#elif defined(SEGMENTVEC)
	Allocator<RWOperation>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC)
	// Preallocate the RWSet elements.
	printf("sizeof(RWSet)=%lu\n", sizeof(RWSet));
	Allocator<RWSet>::init(NUM_TRANSACTIONS * TRANSACTION_SIZE * THREAD_COUNT);
#endif
	return;
}

void allocatorReport()
{
	// Report memory allocator usage.
	MemAllocator<Operation *>::report();

// Report object allocator usage.
#ifdef SEGMENTVEC
	Allocator<Page<VAL, SGMT_SIZE>>::report();
	Allocator<Page<size_t, 1>>::report();
	Allocator<std::map<size_t, Page<VAL, SGMT_SIZE> *, std::less<size_t>, MyPageAllocator>>::report();
#endif
#if defined(SEGMENTVEC) || defined(COMPACTVEC)
	Allocator<RWOperation>::report();
	Allocator<RWSet>::report();
#endif
	return;
}

int main(int argc, char *argv[])
{
	// Seed the random number generator.
	srand(time(NULL));

	// Pre-fill the allocators.
	allocatorInit();

	// Preallocate the random number generator.
	printf("Generating random numbers.\n");
	numPool = new RandomNumberPool(THREAD_COUNT, NUM_TRANSACTIONS * (2 + 3 * TRANSACTION_SIZE));

	// Reserve the transaction vector, for minor performance gains.
	transactions.reserve(THREAD_COUNT);

#ifdef SEGMENTVEC
	transVector = new TransactionalVector();
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

#ifdef PREDICATE_SEARCH
	predicateSearch();
#endif
#ifdef RANDOM_RUN
	randomRun();
#endif
#ifdef HF_SEARCH
	helpFreeSearch();
#endif

	// Report allocator usage.
	allocatorReport();

	// Print the final state of the vector.
	//transVector->printContents();

	return 0;
}