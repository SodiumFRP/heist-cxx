#ifndef _POOLEDLOCKER_H_
#define _POOLEDLOCKER_H_

#include "Mutex.h"

/*!
 * The inefficiency of a naive locker mainly comes from the fact that we want
 * to deallocate the mutex when we have finished with it, and therefore we need
 * to thread-safely maintain a reference count, adding a large cost to copying
 * it by value. This implementation avoids that by preallocating a pool, and
 * picking an item effectively at random from that pool. It means that different
 * maps may end up with the same mutex, but the probability of contention should
 * be very low, as it's spread over the whole pool.
 */
class PooledLocker {
    public:
        PooledLocker();
        ~PooledLocker() {}
        inline void lock() const   {const_cast<PooledLocker*>(this)->mutex->lock();}
        inline void unlock() const {const_cast<PooledLocker*>(this)->mutex->unlock();}
    private:
        Mutex* mutex;
};

#endif
