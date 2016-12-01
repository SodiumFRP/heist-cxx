// $Id$

#ifndef _LIGHTPTR_H_
#define _LIGHTPTR_H_

namespace heist {
    template <class A>
    void deleter(void* a0)
    {
        delete (A*)a0;
    }

    namespace impl {
        typedef void (*Deleter)(void*);
        struct Count {
            Count(
                int count,
                Deleter del
            ) : count(count), del(del) {}
            int count;
            Deleter del;
        };
    };

    /*!
     * An untyped reference-counting smart pointer, thread-safe variant.
     */
    #define DECLARE_LIGHTPTR(Name) \
        struct Name { \
            static Name DUMMY;  /* A null value that does not work, but can be used to */ \
                                    /* satisfy the compiler for unusable private constructors */ \
            Name(); \
            Name(const Name& other); \
            template <class A> static inline Name create(const A& a) { \
                return Name(new A(a), deleter<A>); \
            } \
            Name(void* value, impl::Deleter del); \
            ~Name(); \
            Name& operator = (const Name& other); \
            void* value; \
            impl::Count* count; \
         \
            template <class A> inline A* castPtr(A*) {return (A*)value;} \
            template <class A> inline const A* castPtr(A*) const {return (A*)value;} \
        };

    DECLARE_LIGHTPTR(LightPtr)        // Thread-safe variant
    DECLARE_LIGHTPTR(UnsafeLightPtr)  // Non-thread-safe variant
};

#endif

