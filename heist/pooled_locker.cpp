/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#include "pooled_locker.h"


namespace heist {
    namespace impl {
        #define NO_OF_MUTEXES 61
        static std::recursive_mutex* bank[NO_OF_MUTEXES];
        static volatile int nextBank = 0;
        
        static inline int advance(int nextBank)
        {
            ++nextBank;
            return nextBank == NO_OF_MUTEXES ? 0 : nextBank;
        }
        
        pooled_locker::pooled_locker()
        {
            // Don't care about race conditions, because we're going to get different
            // maps sharing mutexes anyway.
            int chosen = nextBank;
            nextBank = advance(nextBank);
            if (bank[chosen] == NULL)
                bank[chosen] = new std::recursive_mutex;
            mutex = bank[chosen];
        }
    }
}
