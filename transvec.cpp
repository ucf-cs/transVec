#include "transvec.hpp"

SegmentedVector<Page<VAL> *> *array;

void runThread(int threadNum)
{
	for (int i = 0; i < NUM_TRANSACTIONS / THREAD_COUNT; i++)
	{
		Operation<int> *ops = new Operation<int>[1];
		ops[0].type = Operation<int>::OpType::pushBack;
		ops[0].val = i;

		Desc<int> *desc = new Desc<int>(TRANSACTION_SIZE, ops);

		RWSet<int> *set = new RWSet<int>();
		//set->executeTransaction(array, desc);
	}
}

int main(int argc, char *argv[])
{
	auto array = new SegmentedVector<Page<int> *>();

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