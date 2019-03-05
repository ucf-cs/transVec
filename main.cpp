#include "main.hpp"

void runThread(int threadNum)
{
	Operation<int> *ops = new Operation<int>[TRANSACTION_SIZE];
	for (int i = 0; i < TRANSACTION_SIZE; i++)
	{
		ops[i].type = Operation<int>::OpType::pushBack;
		ops[i].val = i;
	}
	Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);
	transVector->executeTransaction(desc);
}

int main(int argc, char *argv[])
{
	auto transVector = new TransactionalVector<VAL>();

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