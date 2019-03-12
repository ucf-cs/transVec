#include "main.hpp"

void runThread(int threadNum)
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

int main(int argc, char *argv[])
{
	transVector = new TransactionalVector<int>();

	// Create our threads.
	thread threads[THREAD_COUNT];

	// Get the current time.
	auto start = chrono::system_clock::now();

	// Start our threads.
	for (long i = 0; i < THREAD_COUNT; i++)
	{
		threads[i] = thread(runThread, i);
	}

	// Wait for all threads to complete.
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		threads[i].join();
	}

	// Get total execution time.
	auto total = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start);

	transVector->printContents();

	cout << "Ran with " << THREAD_COUNT << " threads and " << TRANSACTION_SIZE << " operations per transaction" << endl;
	cout << total.count() << " milliseconds" << endl;

	return 0;
}