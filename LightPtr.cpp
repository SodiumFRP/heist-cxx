// $Id$

#include "LightPtr.h"

#include <pthread.h>

namespace heist {
#if defined(HAVE_PTHREAD_SPIN_LOCK)
    static pthread_spinlock_t* get_lightptr_lock()
    {
        static pthread_spinlock_t* gs;
        if (gs == NULL) {
            gs = new pthread_spinlock_t;
            pthread_spin_init(gs, PTHREAD_PROCESS_PRIVATE);
        }
        return gs;
    }
#else
    static pthread_mutex_t* get_lightptr_lock()
    {
        static pthread_mutex_t* mt;
        if (mt == NULL) {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            mt = new pthread_mutex_t;
            pthread_mutex_init(mt, &attr);
        }
        return mt;
    }
#endif

#define DEFINE_LIGHTPTR(Name, GET_LOCK, LOCK, UNLOCK) \
    Name::Name() \
        : value(NULL), count(NULL) \
    { \
    } \
     \
    Name Name::DUMMY; \
     \
    Name::Name(void* value, impl::Deleter del) \
        : value(value), count(new impl::Count(1, del)) \
    { \
    } \
     \
    Name::Name(const Name& other) \
        : value(other.value), count(other.count) \
    { \
        GET_LOCK; \
        LOCK; \
        count->count++; \
        UNLOCK; \
    } \
     \
    Name::~Name() { \
        GET_LOCK; \
        LOCK; \
        if (count != NULL && --count->count == 0) { \
            UNLOCK; \
            count->del(value); delete count; \
        } \
        else { \
            UNLOCK; \
        } \
    } \
     \
    Name& Name::operator = (const Name& other) { \
        GET_LOCK; \
        LOCK; \
        if (--count->count == 0) { \
            UNLOCK; \
            count->del(value); delete count; \
        } \
        else { \
            UNLOCK; \
        } \
        value = other.value; \
        count = other.count; \
        LOCK; \
        count->count++; \
        UNLOCK; \
        return *this; \
    }

#if defined(HAVE_PTHREAD_SPIN_LOCK)
DEFINE_LIGHTPTR(LightPtr, pthread_spinlock_t* s = get_lightptr_lock(),
                          pthread_spin_lock(s),
                          pthread_spin_unlock(s))
#else
DEFINE_LIGHTPTR(LightPtr, pthread_mutex_t* m = get_lightptr_lock(),
                          pthread_mutex_lock(m),
                          pthread_mutex_unlock(m))
#endif

DEFINE_LIGHTPTR(UnsafeLightPtr,,,)

};
