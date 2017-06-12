/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_LOCKPOOL_H_
#define _HEIST_LOCKPOOL_H_

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
#include <os/lock.h>
#else
#include <libkern/OSAtomic.h>
#endif
#elif defined(__TI_COMPILER_VERSION__)
#else
#include <mutex>
#endif
#include <stdint.h>
#include <limits.h>

namespace heist {
    namespace impl {
        struct spin_lock {
#if defined(SODIUM_SINGLE_THREADED)
            inline void lock() {}
            inline void unlock() {}
#elif defined(__APPLE__)
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
	    os_unfair_lock sl;
	    spin_lock() : sl() {
	    }
            inline void lock() {
                os_unfair_lock_lock(&sl);
            }
            inline void unlock() {
                os_unfair_lock_unlock(&sl);
            }
#else
            OSSpinLock sl;
            spin_lock() : sl(OS_SPINLOCK_INIT) {
            }
            inline void lock() {
                OSSpinLockLock(&sl);
            }
            inline void unlock() {
                OSSpinLockUnlock(&sl);
            }
#endif
#else
            bool initialized;
            std::recursive_mutex m;
            spin_lock() : initialized(true) {
            }
            inline void lock() {
                // Make sure nothing bad happens if this is called before the constructor.
                // This can happen during static initialization if data structures that use
                // this lock pool are declared statically.
                if (initialized) m.lock();
            }
            inline void unlock() {
                if (initialized) m.unlock();
            }
#endif
        };
		#if defined(SODIUM_SINGLE_THREADED)
			#define SODIUM_IMPL_LOCK_POOL_BITS 1
		#else
			#define SODIUM_IMPL_LOCK_POOL_BITS 7
        #endif
        extern spin_lock lock_pool[1<<SODIUM_IMPL_LOCK_POOL_BITS];

        // Use Knuth's integer hash function ("The Art of Computer Programming", section 6.4)
        inline spin_lock* spin_get_and_lock(void* addr)
        {
#if defined(SODIUM_SINGLE_THREADED)
        	return &lock_pool[0];
#else
            spin_lock* l = &lock_pool[(uint32_t)((uint32_t)
                (uintptr_t)(addr)
                * (uint32_t)2654435761U) >> (32 - SODIUM_IMPL_LOCK_POOL_BITS)];
            l->lock();
            return l;
#endif
        }
    }
}

#endif
