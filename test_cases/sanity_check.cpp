#include <iostream>
#include "../define.hpp"

using namespace std;

int main(void)
{
#if defined(TRANSACTION_SIZE) || defined(THREAD_COUNT)
    return 1;
#endif

#if defined(SEGMENTVEC) || defined(COMPACTVEC) || defined(COARSEVEC) || defined(STMVEC)
    return 2;
#endif

    return 0;
}