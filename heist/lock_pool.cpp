/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#include <heist/lock_pool.h>


namespace heist {
    namespace impl {
#ifdef SUPPORTS_INIT_PRIORITY
        spin_lock lock_pool[1<<SODIUM_IMPL_LOCK_POOL_BITS] __attribute__ ((init_priority (101)));
#else
        spin_lock lock_pool[1<<SODIUM_IMPL_LOCK_POOL_BITS];
#endif
    }
}

