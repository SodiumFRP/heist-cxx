/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#include <heist/light_ptr.h>
#include <mutex>

namespace heist {
    static std::mutex* get_lightptr_lock()
    {
        static std::mutex* mt;
        if (mt == nullptr)
            mt = new std::mutex;
        return mt;
    }

#define DEFINE_LIGHTPTR(Name, GET_LOCK, LOCK, UNLOCK) \
    Name::Name() \
        : value(nullptr), count(nullptr) \
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
        if (count != nullptr && --count->count == 0) { \
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

DEFINE_LIGHTPTR(light_ptr, std::mutex* m = get_lightptr_lock(),
                          m->lock(),
                          m->unlock())
DEFINE_LIGHTPTR(unsafe_light_ptr,,,)

}
