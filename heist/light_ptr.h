/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_LIGHTPTR_H_
#define _HEIST_LIGHTPTR_H_

#include <utility>

namespace heist {
    template <typename A>
    void deleter(void* a0)
    {
        delete (A*)a0;
    }

    namespace impl {
        typedef void (*deleter)(void*);
        struct count {
            count(
                int c_,
                deleter del_
            ) : c(c_), del(del_) {}
            int c;
            deleter del;
        };
    };

    /*!
     * An untyped reference-counting smart pointer, thread-safe variant.
     */
    #define HEIST_DECLARE_LIGHTPTR(name) \
        struct name { \
            static name DUMMY;  /* A null value that does not work, but can be used to */ \
                                /* satisfy the compiler for unusable private constructors */ \
            name(); \
            name(const name& other); \
            name(name&& other) : value(other.value), count(other.count) \
            { \
                other.value = nullptr; \
                other.count = nullptr; \
            } \
            template <typename A> static inline name create(const A& a) { \
                return name(new A(a), deleter<A>); \
            } \
            template <typename A> static inline name create(A&& a) { \
                return name(new A(std::move(a)), deleter<A>); \
            } \
            name(void* value, impl::deleter del); \
            ~name(); \
            name& operator = (const name& other); \
            void* value; \
            impl::count* count; \
         \
            template <typename A> inline A* cast_ptr(A*) {return (A*)value;} \
            template <typename A> inline const A* cast_ptr(A*) const {return (A*)value;} \
        };

    HEIST_DECLARE_LIGHTPTR(light_ptr)        // Thread-safe variant
    HEIST_DECLARE_LIGHTPTR(unsafe_light_ptr)  // Non-thread-safe variant
}

#endif

