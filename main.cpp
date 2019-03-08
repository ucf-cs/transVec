#include "main.hpp"

void runThread(int threadNum)
{
	// A list of operations for the current thread.
	Operation<int> *ops = new Operation<int>[TRANSACTION_SIZE];
	// For each operation.
	for (int i = 0; i < TRANSACTION_SIZE; i++)
	{
		// All operations are pushes.
		ops[i].type = Operation<int>::OpType::pushBack;
		// All operations push the transaction number so they're unique to the thread.
		ops[i].val = i;
	}
	// Create a transaction containing the these operations.
	Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);
	// Execute the transaction.
	transVector->executeTransaction(desc);
}

int main(int argc, char *argv[])
{
	auto transVector = new TransactionalVector<int>();

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

	cout << "Ran with " << THREAD_COUNT << " threads and " << TRANSACTION_SIZE << " operations per transaction" << endl;
	cout << total.count() << " milliseconds" << endl;

	return 0;
}