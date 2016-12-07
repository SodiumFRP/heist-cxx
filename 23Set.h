// $Id$

/**
 * Immutable set.
 *
 * Passing the whole map by value is cheap (unlike std::map, etc).
 *
 * It's a 2-3 tree so it's self-balancing.  All operations are O(log N) like std::map.
 *
 * When you insert an item, both the old map (before the insert) and the new one
 * (after the insert) are valid.  The memory consumed is only the difference between
 * the two.  Whichever you don't ultimately want gets cleaned up when you don't
 * reference it any more.
 *
 * The major advantage is that you can pass these between threads without having to
 * care at all about synchronizing between the threads.  The producer can race ahead
 * modifying its own copy secure in the knowledge that the consumer has only the
 * version it was sent.  Race conditions are absolutely impossible.
 *
 * The interface is pretty hard to stuff things up in.  For example, an iterator that
 * points nowhere is a concept that doesn't exist (unlike std::map, etc).
 */
#ifndef _23SET_H_
#define _23SET_H_

#include "FMap.h"
#include "LightPtr.h"
#include <heist/list.h>

#include <boost/variant.hpp>
#include <mutex>
#include <stdexcept>
#include <unistd.h>  // for size_t

namespace Set23_Impl {
    typedef heist::UnsafeLightPtr Ptr;
    typedef int (*Comparator)(const Ptr&, const Ptr&);

    struct Empty { };

    struct Leaf1 {
        private: Leaf1() : a(Ptr::DUMMY) {} public:
        Leaf1(const Leaf1& other)
            : a(other.a)
        {
        }
        Leaf1(const Ptr& a)
            : a(a)
        {
        }
        Ptr a;
    };

    struct Leaf2 {
        private: Leaf2() : a(Ptr::DUMMY), b(Ptr::DUMMY) {} public:
        Leaf2(const Leaf2& other)
            : a(other.a), b(other.b)
        {
        }
        Leaf2(const Ptr& a, const Ptr& b)
            : a(a), b(b)
        {
        }
        Ptr a, b;
    };

    struct Node2;
    struct Node3;

    struct Iterator;

    struct NoOfIndicesVisitor : public boost::static_visitor<int>
    {
        int operator()(const Leaf1& l1) const {return 1;}
        int operator()(const Leaf2& l2) const {return 2;}
        int operator()(const Node2& n2) const {return 3;}
        int operator()(const Node3& n3) const {return 5;}
    };

    struct Node {
        Node(
            boost::variant<
                Leaf1,
                Leaf2,
                boost::recursive_wrapper<Node2>,
                boost::recursive_wrapper<Node3>
            > n
        ) : n(n) {}

        boost::variant<
            Leaf1,
            Leaf2,
            boost::recursive_wrapper<Node2>,
            boost::recursive_wrapper<Node3>
        > n;

        int getNoOfIndices() const
        {
            return boost::apply_visitor(NoOfIndicesVisitor(), n);
        }

        /*!
         * The result of inserting into a node: Either the new node, or the
         * overflow (as a Node2) if it doesn't fit.
         */
        typedef boost::variant<
            boost::recursive_wrapper<Node>,
            boost::recursive_wrapper<Node2>
        > InsertResult;

        InsertResult insert(const Comparator& compare, const Ptr& a) const;

        Iterator begin() const;
        Iterator end() const;
        boost::optional<Iterator> lower_bound(const Comparator& compare, const Ptr& a) const;
        boost::optional<Iterator> find(const Comparator& compare, const Ptr& a) const;
    };

    struct Position {
        Position(const Node& node, int ix) : node(node), ix(ix) {}
        Node node;
        int ix;
    };

    struct Iterator {
        Iterator() {}
        Iterator(const heist::list<Position>& stack) : stack(stack) {}
        heist::list<Position> stack;
        boost::optional<Iterator> next() const {return move(1);}
        boost::optional<Iterator> prev() const {return move(-1);}

        const Ptr& get() const;
        boost::optional<Node> remove() const;

        /*!
         * Reconstruct the root node of the tree that this iterator refers to.
         */
        Node unwind() const;

      private:
        boost::optional<Iterator> move(int dir) const;
    };

    struct Node2 {
        private: Node2() : p(Ptr::DUMMY), a(Ptr::DUMMY), q(Ptr::DUMMY) {} public:
        Node2(const Node2& other)
            : p(other.p), a(other.a), q(other.q)
        {
        }
        Node2(const Ptr& p, const Ptr& a, const Ptr& q)
            : p(p), a(a), q(q)
        {
        }
        Ptr p;
        Ptr a;
        Ptr q;
    };

    struct Node3 {
        private: Node3() : p(Ptr::DUMMY), a(Ptr::DUMMY), q(Ptr::DUMMY), b(Ptr::DUMMY), r(Ptr::DUMMY) {} public:
        Node3(const Node3& other)
            : p(other.p), a(other.a), q(other.q), b(other.b), r(other.r)
        {
        }
        Node3(const Ptr& p, const Ptr& a, const Ptr& q, const Ptr& b, const Ptr& r)
            : p(p), a(a), q(q), b(b), r(r)
        {
        }
        Ptr p;
        Ptr a;
        Ptr q;
        Ptr b;
        Ptr r;
    };

    template <class A>
    int comparator(const Ptr& a0, const Ptr& b0)
    {
        const A& a = *a0.castPtr<A>(NULL);
        const A& b = *b0.castPtr<A>(NULL);
        if (a < b)  return -1; else
        if (a == b) return 0; else
                    return 1;
    }

    void nullDeleter(void* a0);

    /*!
     * Must be called with lock held.
     */
    template <class A>
    Ptr mkPtr(A* a)
    {
        return Ptr(a, heist::deleter<A>);
    }
};

namespace heist {
    template <class L>
    struct LockHolder {
        L& locker;
        LockHolder(const L& locker) : locker(*const_cast<L*>(&locker)) { this->locker.lock(); }
        ~LockHolder()                                { this->locker.unlock(); }
    };

    template <class A, class L, class SET> class Set_
    {
    private:
        L locker;
        boost::optional<Set23_Impl::Node> r;

    public:
        static SET fromList(heist::list<A> xs) {
            Set_<A,L,SET> s;
            while (xs) {
                s = s.insert(xs.head());
                xs = xs.tail();
            }
            return SET(s);
        }

        Set_() {}
        Set_(const heist::list<A>& xs) {
            *this = fromList(xs);
        }
        Set_(std::initializer_list<A> il) {
            Set_<A,L,SET> s;
            for (auto it = il.begin(); it != il.end(); ++it)
                s = s.insert(*it);
            *this = s;
        }
        Set_(const L& locker, boost::optional<Set23_Impl::Node> r)
            : locker(*const_cast<L*>(&locker))
        {
            this->locker.lock();
            this->r = r;
            this->locker.unlock();
        }
        Set_(const Set_<A, L, SET>& other) : locker(other.locker) {
            locker.lock();
            this->r = other.r;
            locker.unlock();
        }
        Set_<A, L, SET>& operator = (const Set_<A, L, SET>& other) {
            locker.lock();
            r = boost::optional<Set23_Impl::Node>();
            locker.unlock();
            this->locker = other.locker;
            locker.lock();
            r = other.r;
            locker.unlock();
            return *this;
        }
        ~Set_() {
            locker.lock();
            r = boost::optional<Set23_Impl::Node>();
            locker.unlock();
        }

        static SET singleton(const A& x) {
            return SET(Set_<A,L,SET>().insert(x));
        }

        class Iterator {
            private:
                L locker;
                typename Set23_Impl::Iterator it;
            public:
                Iterator(const L& locker, const Set23_Impl::Iterator& it) : locker(locker) {
                    this->locker.lock();
                    this->it = it;
                    this->locker.unlock();
                }
                Iterator(const Iterator& other) : locker(other.locker) {
                    locker.lock();
                    this->it = other.it;
                    locker.unlock();
                }
                Iterator& operator = (const Iterator& other) {
                    locker.lock();
                    this->it = Set23_Impl::Iterator();
                    locker.unlock();
                    this->locker = other.locker;
                    locker.lock();
                    this->it = other.it;
                    locker.unlock();
                    return *this;
                }
                ~Iterator() {
                    locker.lock();
                    this->it = Set23_Impl::Iterator();
                    locker.unlock();
                }
                boost::optional<Iterator> next() const {
                    LockHolder<L> lh(locker);
                    auto oit2 = it.next();
                    if (oit2)
                        return boost::make_optional(Set_<A,L,SET>::Iterator(locker, oit2.get()));
                    else
                        return boost::optional<Iterator>();
                }
                boost::optional<Iterator> prev() const {
                    LockHolder<L> lh(locker);
                    auto oit2 = it.prev();
                    if (oit2)
                        return boost::make_optional(Set_<A,L,SET>::Iterator(locker, oit2.get()));
                    else
                        return boost::optional<Iterator>();
                }
                const A& get() const
                {
                    return *(A*)it.get().value;
                }

                SET remove() const
                {
                    LockHolder<L> lh(locker);
                    return SET(Set_<A,L,SET>(locker, it.remove()));
                }
        };

        boost::optional<Iterator> begin() const
        {
            LockHolder<L> lh(locker);
            const L& locker = this->locker;
            return fmap<Set23_Impl::Node, Iterator>(
                [&locker] (const Set23_Impl::Node& node) -> Iterator {return Iterator(locker, node.begin());},
                r
            );
        }

        boost::optional<Iterator> end() const
        {
            LockHolder<L> lh(locker);
            const L& locker = this->locker;
            return fmap<Set23_Impl::Node, Iterator>(
                [&locker] (const Set23_Impl::Node& node) -> Iterator {return Iterator(locker, node.end());},
                r
            );
        }

        /*!
         * Return an iterator pointing to the smallest value >= the pivot,
         * undefined if all values are < the pivot.
         */
        boost::optional<Iterator> lower_bound(const A& pivot) const
        {
            LockHolder<L> lh(locker);
            if (r) {
                Set23_Impl::Ptr elt((void*)&pivot, Set23_Impl::nullDeleter);
                auto oit = r.get().lower_bound(Set23_Impl::comparator<A>, elt);
                if (oit)
                    return boost::make_optional(Iterator(locker, oit.get()));
            }
            return boost::optional<Iterator>();
        }

        /*!
         * Return an iterator pointing to the largest value <= the pivot,
         * undefined if all values are > the pivot.
         */
        boost::optional<Iterator> upper_bound(const A& pivot) const
        {
            LockHolder<L> lh(locker);
            auto oit = lower_bound(pivot);
            if (oit) {
                auto it = oit.get();
                return pivot < it.get() ? it.prev() : oit;
            }
            else
                return end();
        }

        boost::optional<Iterator> find(const A& a) const
        {
            LockHolder<L> lh(locker);
            if (r) {
                Set23_Impl::Ptr elt((void*)&a, Set23_Impl::nullDeleter);
                auto oit = r.get().find(Set23_Impl::comparator<A>, elt);
                if (oit)
                    return boost::make_optional(Iterator(locker, oit.get()));
            }
            return boost::optional<Iterator>();
        }

        bool contains(const A& a) const
        {
            return (bool)find(a);
        }

        SET insert(const A& a) const {
            LockHolder<L> lh(locker);
            Set23_Impl::Comparator compare = Set23_Impl::comparator<A>;
            Set23_Impl::Ptr newPtr(new A(a), heist::deleter<A>);
            if (r) {
                auto result = r.get().insert(compare, newPtr);
                if (const Set23_Impl::Node* newNode = boost::get<Set23_Impl::Node>(&result))
                    return SET(Set_<A,L,SET>(locker, *newNode));
                else
                    return SET(Set_<A,L,SET>(locker, Set23_Impl::Node(boost::get<Set23_Impl::Node2>(result))));
            }
            else
                return SET(Set_<A,L,SET>(locker, Set23_Impl::Node(Set23_Impl::Leaf1(newPtr))));
        }

        SET remove(const A& a) const {
            auto oit = find(a);
            return SET(oit ? oit.get().remove()
                           : *this);
        }

        bool operator == (const Set_<A,L,SET>& other) const {
            boost::optional<Set_<A,L,SET>::Iterator> it1 = begin();
            boost::optional<Set_<A,L,SET>::Iterator> it2 = other.begin();
            while (it1 && it2) {
                if (!(it1.get().get() == it2.get().get())) return false;
                it1 = it1.get().next();
                it2 = it2.get().next();
            }
            return !it1 && !it2;
        }

        bool operator != (const Set_<A,L,SET>& other) const {
            return ! (*this == other);
        }

        list<A> toList() const {
            list<A> acc;
            for (auto it = end(); it; it = it.get().prev())
                acc = it.get().get() %= acc;
            return acc;
        }

        /*!
         * Monoidal append = set union.
         */
        Set_<A,L,SET> operator + (const Set_<A,L,SET>& other) const;

        size_t size() const {
            size_t len = 0;
            auto oit = begin();
            while (oit) {len++; oit = oit.get().next();}
            return len;
        }
    };

    /*!
     * Thread-safe variant of Set.
     */
    template <class A>
    class Set : public Set_<A, std::mutex, Set<A>>
    {
        public:
            Set() {}
            Set(const Set_<A, std::mutex, Set<A>>& other) : Set_<A, std::mutex, Set<A>>(other) {}
            Set(const heist::list<A>& xs) : Set_<A, std::mutex, Set<A>>(xs) {}
            Set(std::initializer_list<A> il) : Set_<A, std::mutex, Set<A>>(il) {}
    };

    struct NullLocker {
        inline void lock() {}
        inline void unlock() {}
    };

    /*!
     * Thread-unsafe but fast variant of Set.
     */
    template <class A>
    class USet : public Set_<A, NullLocker, USet<A>>
    {
        public:
            USet() {}
            USet(const Set_<A, NullLocker, USet<A>>& other) : Set_<A, NullLocker, USet<A>>(other) {}
            USet(const heist::list<A>& xs) : Set_<A, NullLocker, USet<A>>(xs) {}
            USet(std::initializer_list<A> il) : Set_<A, NullLocker, USet<A>>(il) {}
    };

    /*!
     * Set with specified locker.
     */
    template <class A, class L>
    class LSet : public Set_<A, L, LSet<A, L>>
    {
        public:
            LSet() {}
            LSet(const Set_<A, L, LSet<A, L>>& other) : Set_<A, L, LSet<A, L>>(other) {}
            LSet(const heist::list<A>& xs) : Set_<A, L, LSet<A, L>>(xs) {}
            LSet(std::initializer_list<A> il) : Set_<A, L, LSet<A, L>>(il) {}
    };

    /*!
     * Map a function over the map elements.
     */
    template <class A, class B>
    Set<B> fmap(std::function<B(const A&)> f, const Set<A>& other) {
        return foldl<Set<B>, A>([=] (const Set<B>& s, A a) {
                return s.insert(f(a));
            }, Set<B>(), other.toList());
    }

    /*!
     * Map a function over the map elements.
     */
    template <class A, class B>
    USet<B> fmap(std::function<B(const A&)> f, const USet<A>& other) {
        return foldl<USet<B>, A>([=] (const USet<B>& s, A a) {
                return s.insert(f(a));
            }, USet<B>(), other.toList());
    }

    template <class A, class B>
    A foldl(std::function<A(const A&, const B&)> f, A a, boost::optional<typename heist::Set<B>::Iterator> oit)
    {
        while (oit) {
            auto it = oit.get();
            a = f(a, it.get());
            oit = it.next();
        }
        return a;
    }

    template <class A, class B>
    A foldl(std::function<A(const A&, const B&)> f, A a, boost::optional<typename heist::USet<B>::Iterator> oit)
    {
        while (oit) {
            auto it = oit.get();
            a = f(a, it.get());
            oit = it.next();
        }
        return a;
    }

    /*!
     * Fold the set's values into a single value using the specified binary operation.
     */
    template <class A, class B>
    A foldl(std::function<A(const A&, const B&)> f, A a, const heist::Set<B> set)
    {
        return foldl(f, a, set.begin());
    }

    /*!
     * Fold the set's values into a single value using the specified binary operation.
     */
    template <class A, class B>
    A foldl(std::function<A(const A&, const B&)> f, A a, const heist::USet<B> set)
    {
        return foldl(f, a, set.begin());
    }

    /*!
     * Fold a non-empty set with no initial value.
     */
    template <class A>
    A foldl1(std::function<A(const A&, const A&)> f, const heist::Set<A>& set)
    {
        auto oit = set.begin();
        if (oit) {
            auto it = oit.get();
            return foldl(f, it.get(), it.next());
        }
        else
            HEIST_THROW(std::invalid_argument, "can't fold1 an empty set");
    }

    /*!
     * Fold a non-empty set with no initial value.
     */
    template <class A>
    A foldl1(std::function<A(const A&, const A&)> f, const heist::USet<A>& set)
    {
        auto oit = set.begin();
        if (oit) {
            auto it = oit.get();
            return foldl(f, it.get(), it.next());
        }
        else
            HEIST_THROW(std::invalid_argument, "can't fold1 an empty set");
    }

    /*!
     * Monoidal append = set union.
     */
    template <class A, class L, class SET>
    Set_<A,L,SET> Set_<A, L, SET>::operator + (const Set_<A,L,SET>& other) const {
        return foldl<Set_<A,L,SET>,A>(
            [] (const Set_<A,L,SET>& s, const A& b) {return s.insert(b);},
            *this,
            other);
    }

    /*!
     * Set difference a - b:  Return the value which is the first set after having removed
     * all the items in the second set from it.
     */
    template <class A>
    Set<A> difference(const Set<A>& sa, const Set<A>& sb)
    {
        return foldl<Set<A>, A>([=] (const Set<A>& s, const A& b) { return s.remove(b); }, sa, sb);
    }

    /*!
     * Set difference a - b:  Return the value which is the first set after having removed
     * all the items in the second set from it.
     */
    template <class A>
    USet<A> difference(const USet<A>& sa, const USet<A>& sb)
    {
        return foldl<USet<A>, A>([=] (const USet<A>& s, const A& b) { return s.remove(b); }, sa, sb);
    }

    /*!
     * Set intersection.
     */
    template <class A>
    Set<A> intersection(const Set<A>& sa, const Set<A>& sb)
    {
        Set<A> out;
        for (auto oit = sa.begin(); oit; oit = oit.get().next()) {
            auto elt = oit.get().get();
            if (sb.contains(elt))
                out = out.insert(elt);
        }
        return out;
    }

    /*!
     * Set intersection.
     */
    template <class A>
    USet<A> intersection(const USet<A>& sa, const USet<A>& sb)
    {
        USet<A> out;
        for (auto oit = sa.begin(); oit; oit = oit.get().next()) {
            auto elt = oit.get().get();
            if (sb.contains(elt))
                out = out.insert(elt);
        }
        return out;
    }
};  // end namespace

template <class A>
std::ostream& operator << (std::ostream& os, heist::Set<A> set) {
    bool first = true;
    os << "{";
    for (auto fs = set.toList(); fs; fs = fs.tail()) {
        os << (first ? "" : ",") << fs.head();
        first = false;
    }
    os << "}";
    return os;
}

template <class A>
std::ostream& operator << (std::ostream& os, heist::USet<A> set) {
    bool first = true;
    os << "{";
    for (auto fs = set.toList(); fs; fs = fs.tail()) {
        os << (first ? "" : ",") << fs.head();
        first = false;
    }
    os << "}";
    return os;
}

#endif
