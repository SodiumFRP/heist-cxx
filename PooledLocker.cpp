// $Id$

#include "PooledLocker.h"

using namespace std;


#define NO_OF_MUTEXES 61
static Mutex* bank[NO_OF_MUTEXES];
static volatile int nextBank = 0;

static inline int advance(int nextBank)
{
    ++nextBank;
    return nextBank == NO_OF_MUTEXES ? 0 : nextBank;
}

PooledLocker::PooledLocker()
{
    // Don't care about race conditions, because we're going to get different
    // maps sharing mutexes anyway.
    int chosen = nextBank;
    nextBank = advance(nextBank);
    if (bank[chosen] == NULL)
        bank[chosen] = new Mutex;
    mutex = bank[chosen];
}
