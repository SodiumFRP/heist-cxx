/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#include <heist/light_ptr.h>
#include <heist/lock_pool.h>

namespace heist {
#define HEIST_DEFINE_LIGHTPTR(Name, GET_AND_LOCK, UNLOCK) \
    Name::Name() \
        : value(nullptr), count(nullptr) \
    { \
    } \
     \
    Name Name::DUMMY; \
     \
    Name::Name(void* value_, impl::deleter del_) \
        : value(value_), count(new impl::count(1, del_)) \
    { \
    } \
     \
    Name::Name(const Name& other) \
        : value(other.value), count(other.count) \
    { \
        GET_AND_LOCK; \
        count->c++; \
        UNLOCK; \
    } \
    \
    Name::~Name() { \
        GET_AND_LOCK; \
        if (count != nullptr && --count->c == 0) { \
            UNLOCK; \
            count->del(value); delete count; \
        } \
        else { \
            UNLOCK; \
        } \
    } \
     \
    Name& Name::operator = (const Name& other) { \
        if (count != other.count) { \
            { \
                GET_AND_LOCK; \
                if (--count->c == 0) { \
                    UNLOCK; \
                    count->del(value); delete count; \
                } \
                else { \
                    UNLOCK; \
                } \
            } \
            value = other.value; \
            count = other.count; \
            { \
                GET_AND_LOCK; \
                count->c++; \
                UNLOCK; \
            } \
        } \
        return *this; \
    }

HEIST_DEFINE_LIGHTPTR(light_ptr, impl::spin_lock* l = impl::spin_get_and_lock(this->value),
                          l->unlock())

HEIST_DEFINE_LIGHTPTR(unsafe_light_ptr,,)

};

