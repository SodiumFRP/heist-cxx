/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_POOLED_LOCKER_H_
#define _HEIST_POOLED_LOCKER_H_

#include <mutex>

namespace heist {
    namespace impl {
        /*!
         * The inefficiency of a naive locker mainly comes from the fact that we want
         * to deallocate the mutex when we have finished with it, and therefore we need
         * to thread-safely maintain a reference count, adding a large cost to copying
         * it by value. This implementation avoids that by preallocating a pool, and
         * picking an item effectively at random from that pool. It means that different
         * maps may end up with the same mutex, but the probability of contention should
         * be very low, as it's spread over the whole pool.
         */
        class pooled_locker {
            public:
                pooled_locker();
                ~pooled_locker() {}
                inline void lock() const   {const_cast<pooled_locker*>(this)->mutex->lock();}
                inline void unlock() const {const_cast<pooled_locker*>(this)->mutex->unlock();}
            private:
                std::recursive_mutex* mutex;
        };
    }
}

#endif
