#include "main.hpp"

void pushThread(int threadNum)
{
	// For each transaction.
	for (size_t i = 0; i < NUM_TRANSACTIONS; i++)
	{
		// A list of operations for the current thread.
		Operation<int> *ops = new Operation<int>[TRANSACTION_SIZE];
		// For each operation.
		for (int j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation<int>::OpType::pushBack;
			ops[j].val =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);
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
		Operation<int> *ops = new Operation<int>[TRANSACTION_SIZE];
		// For each operation.
		for (int j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are reads.
			ops[j].type = Operation<int>::OpType::read;
			// DEBUG: This will always be an invalid read. Test invalid writes too.
			ops[j].index =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);
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
		Operation<int> *ops = new Operation<int>[TRANSACTION_SIZE];
		// For each operation.
		for (int j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are writes.
			ops[j].type = Operation<int>::OpType::write;
			ops[j].val = 0;
			ops[j].index =
				threadNum * NUM_TRANSACTIONS * TRANSACTION_SIZE +
				i * TRANSACTION_SIZE +
				j;
		}
		// Create a transaction containing the these operations.
		Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);
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
		Operation<int> *ops = new Operation<int>[TRANSACTION_SIZE];
		// For each operation.
		for (int j = 0; j < TRANSACTION_SIZE; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation<int>::OpType::popBack;
		}
		// Create a transaction containing the these operations.
		Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);
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
		Operation<int> *ops = new Operation<int>[transSize];
		// For each operation.
		for (int j = 0; j < transSize; j++)
		{
			// All operations are pushes.
			ops[j].type = Operation<int>::OpType(rand() % 6);
			ops[j].index = rand() % 1000;
			ops[j].val = rand() % 1000;
		}
		// Create a transaction containing the these operations.
		Desc<int> *desc = new Desc<int>(transSize, ops);
		// Execute the transaction.
		transVector->executeTransaction(desc);
	}
}

void threadRunner(thread threads[THREAD_COUNT], void function(int threadNum))
{
	// Start our threads.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i] = thread(function, i);
	}

	// Wait for all threads to complete.
	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		threads[i].join();
	}
	return;
}

int main(int argc, char *argv[])
{
	transVector = new TransactionalVector<int>();

	// Create our threads.
	thread threads[THREAD_COUNT];

	// Get the current time.
	auto start = chrono::system_clock::now();

	threadRunner(threads, randThread);

	// Get total execution time.
	auto total = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start);

	transVector->printContents();

	cout << "Ran with " << THREAD_COUNT << " threads and " << TRANSACTION_SIZE << " operations per transaction" << endl;
	cout << total.count() << " milliseconds" << endl;

	return 0;
}