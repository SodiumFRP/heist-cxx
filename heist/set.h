/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_SET_H_
#define _HEIST_SET_H_
/*
 * Immutable set.
 *
 * Passing the whole set by value is cheap (unlike std::set, etc).
 *
 * It's a 2-3 tree so it's self-balancing.  All operations are O(log N) like std::set.
 *
 * When you insert an item, both the old set (before the insert) and the new one
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
 * points nowhere is a concept that doesn't exist (unlike std::set, etc).
 */

#include <heist/light_ptr.h>
#include <heist/list.h>
#include <heist/pooled_locker.h>

#include <boost/variant.hpp>
#include <unistd.h>  // for size_t

namespace heist {
    namespace impl {
        typedef heist::unsafe_light_ptr Ptr;
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
    
        struct iterator;
    
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
    
            iterator begin() const;
            iterator end() const;
            boost::optional<iterator> lower_bound(const Comparator& compare, const Ptr& a) const;
            boost::optional<iterator> find(const Comparator& compare, const Ptr& a) const;
        };
    
        struct Position {
            Position(const Node& node, int ix) : node(node), ix(ix) {}
            Node node;
            int ix;
        };
    
        struct iterator {
            iterator() {}
            iterator(const heist::list<Position>& stack) : stack(stack) {}
            heist::list<Position> stack;
            boost::optional<iterator> next() const {return move(1);}
            boost::optional<iterator> prev() const {return move(-1);}
    
            const Ptr& get() const;
            boost::optional<Node> remove() const;
    
            /*!
             * Reconstruct the root node of the tree that this iterator refers to.
             */
            Node unwind() const;
    
          private:
            boost::optional<iterator> move(int dir) const;
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
            const A& a = *a0.cast_ptr<A>(NULL);
            const A& b = *b0.cast_ptr<A>(NULL);
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
};

namespace heist {
    namespace impl {
        template <typename L>
        struct lock_holder {
            L& locker;
            lock_holder(const L& locker)
            : locker(*const_cast<L*>(&locker)) { this->locker.lock(); }
            ~lock_holder()                     { this->locker.unlock(); }
        };
    }

    template <class A> class set
    {
    private:
        impl::pooled_locker locker;
        boost::optional<heist::impl::Node> r;

        static set from_list(heist::list<A> xs) {
            set<A> s;
            while (xs) {
                s = s.insert(xs.head());
                xs = xs.tail();
            }
            return set(s);
        }

    public:
        set() {}
        set(const heist::list<A>& xs) {
            *this = from_list(xs);
        }
        set(std::initializer_list<A> il) {
            set<A> s;
            for (auto it = il.begin(); it != il.end(); ++it)
                s = s.insert(*it);
            *this = s;
        }
        set(const impl::pooled_locker& locker, boost::optional<heist::impl::Node> r)
            : locker(*const_cast<impl::pooled_locker*>(&locker))
        {
            this->locker.lock();
            this->r = r;
            this->locker.unlock();
        }
        set(const set<A>& other) : locker(other.locker) {
            locker.lock();
            this->r = other.r;
            locker.unlock();
        }
        set<A>& operator = (const set<A>& other) {
            locker.lock();
            r = boost::optional<heist::impl::Node>();
            locker.unlock();
            this->locker = other.locker;
            locker.lock();
            r = other.r;
            locker.unlock();
            return *this;
        }
        ~set() {
            locker.lock();
            r = boost::optional<heist::impl::Node>();
            locker.unlock();
        }

        static set singleton(const A& x) {
            return set(set<A>().insert(x));
        }

        class iterator {
            private:
                impl::pooled_locker locker;
                typename heist::impl::iterator it;
            public:
                iterator(const impl::pooled_locker& locker, const heist::impl::iterator& it) : locker(locker) {
                    this->locker.lock();
                    this->it = it;
                    this->locker.unlock();
                }
                iterator(const iterator& other) : locker(other.locker) {
                    locker.lock();
                    this->it = other.it;
                    locker.unlock();
                }
                iterator& operator = (const iterator& other) {
                    locker.lock();
                    this->it = heist::impl::iterator();
                    locker.unlock();
                    this->locker = other.locker;
                    locker.lock();
                    this->it = other.it;
                    locker.unlock();
                    return *this;
                }
                ~iterator() {
                    locker.lock();
                    this->it = heist::impl::iterator();
                    locker.unlock();
                }
                boost::optional<iterator> next() const {
                    impl::lock_holder<impl::pooled_locker> lh(locker);
                    auto oit2 = it.next();
                    if (oit2)
                        return boost::make_optional(set<A>::iterator(locker, oit2.get()));
                    else
                        return boost::optional<iterator>();
                }
                boost::optional<iterator> prev() const {
                    impl::lock_holder<impl::pooled_locker> lh(locker);
                    auto oit2 = it.prev();
                    if (oit2)
                        return boost::make_optional(set<A>::iterator(locker, oit2.get()));
                    else
                        return boost::optional<iterator>();
                }
                const A& get() const
                {
                    return *(A*)it.get().value;
                }

                set remove() const
                {
                    impl::lock_holder<impl::pooled_locker> lh(locker);
                    return set(set<A>(locker, it.remove()));
                }
        };

        boost::optional<iterator> begin() const
        {
            impl::lock_holder<impl::pooled_locker> lh(locker);
            const impl::pooled_locker& locker = this->locker;
            return r ? boost::optional<iterator>(iterator(locker, r.get().begin()))
                     : boost::optional<iterator>();
        }

        boost::optional<iterator> end() const
        {
            impl::lock_holder<impl::pooled_locker> lh(locker);
            const impl::pooled_locker& locker = this->locker;
            return r ? boost::optional<iterator>(iterator(locker, r.get().end()))
                     : boost::optional<iterator>();
        }

        /*!
         * Return an iterator pointing to the smallest value >= the pivot,
         * undefined if all values are < the pivot.
         */
        boost::optional<iterator> lower_bound(const A& pivot) const
        {
            impl::lock_holder<impl::pooled_locker> lh(locker);
            if (r) {
                heist::impl::Ptr elt((void*)&pivot, heist::impl::nullDeleter);
                auto oit = r.get().lower_bound(heist::impl::comparator<A>, elt);
                if (oit)
                    return boost::make_optional(iterator(locker, oit.get()));
            }
            return boost::optional<iterator>();
        }

        /*!
         * Return an iterator pointing to the largest value <= the pivot,
         * undefined if all values are > the pivot.
         */
        boost::optional<iterator> upper_bound(const A& pivot) const
        {
            impl::lock_holder<impl::pooled_locker> lh(locker);
            auto oit = lower_bound(pivot);
            if (oit) {
                auto it = oit.get();
                return pivot < it.get() ? it.prev() : oit;
            }
            else
                return end();
        }

        boost::optional<iterator> find(const A& a) const
        {
            impl::lock_holder<impl::pooled_locker> lh(locker);
            if (r) {
                heist::impl::Ptr elt((void*)&a, heist::impl::nullDeleter);
                auto oit = r.get().find(heist::impl::comparator<A>, elt);
                if (oit)
                    return boost::make_optional(iterator(locker, oit.get()));
            }
            return boost::optional<iterator>();
        }

        bool contains(const A& a) const
        {
            return (bool)find(a);
        }

        set insert(const A& a) const {
            impl::lock_holder<impl::pooled_locker> lh(locker);
            heist::impl::Comparator compare = heist::impl::comparator<A>;
            heist::impl::Ptr newPtr(new A(a), heist::deleter<A>);
            if (r) {
                auto result = r.get().insert(compare, newPtr);
                if (const heist::impl::Node* newNode = boost::get<heist::impl::Node>(&result))
                    return set(set<A>(locker, *newNode));
                else
                    return set(set<A>(locker, heist::impl::Node(boost::get<heist::impl::Node2>(result))));
            }
            else
                return set(set<A>(locker, heist::impl::Node(heist::impl::Leaf1(newPtr))));
        }

        set remove(const A& a) const {
            auto oit = find(a);
            return set(oit ? oit.get().remove()
                           : *this);
        }

        bool operator == (const set<A>& other) const {
            boost::optional<set<A>::iterator> it1 = begin();
            boost::optional<set<A>::iterator> it2 = other.begin();
            while (it1 && it2) {
                if (!(it1.get().get() == it2.get().get())) return false;
                it1 = it1.get().next();
                it2 = it2.get().next();
            }
            return !it1 && !it2;
        }

        bool operator != (const set<A>& other) const {
            return ! (*this == other);
        }

        bool operator < (const set<A>& other) const {
            boost::optional<set<A>::iterator> it1 = begin();
            boost::optional<set<A>::iterator> it2 = other.begin();
            while (it1 && it2) {
                if (it1.get().get() < it2.get().get()) return true;
                if (it2.get().get() < it1.get().get()) return false;
                it1 = it1.get().next();
                it2 = it2.get().next();
            }
            return (bool)it2;
        }

        bool operator > (const set<A>& other) const {
            return other < *this;
        }

        bool operator <= (const set<A>& other) const {
            return !(*this > other);
        }

        bool operator >= (const set<A>& other) const {
            return !(*this < other);
        }

        list<A> to_list() const {
            list<A> acc;
            for (auto it = end(); it; it = it.get().prev())
                acc = it.get().get() %= acc;
            return acc;
        }

        size_t size() const {
            size_t len = 0;
            auto oit = begin();
            while (oit) {len++; oit = oit.get().next();}
            return len;
        }

        template <class B>
        static B foldl(std::function<B(const B&, const A&)> f, B b,
                boost::optional<typename heist::set<A>::iterator> oit)
        {
            while (oit) {
                auto it = oit.get();
                b = f(b, it.get());
                oit = it.next();
            }
            return b;
        }

        /*!
         * Fold the set's values into a single value using the specified binary operation.
         */
        template <class B>
        B foldl(std::function<B(const B&, const A&)> f, B b) const
        {
            return foldl<B>(f, b, begin());
        }
    
        /*!
         * Fold a non-empty set with no initial value.
         */
        A foldl1(std::function<A(const A&, const A&)> f) const
        {
            auto oit = begin();
            assert(oit);
            auto it = oit.get();
            return foldl<A>(f, it.get(), it.next());
        }

        /*!
         * Map a function over the set elements.
         */
        template <class Fn>
        set<typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
            typedef typename std::result_of<Fn(A)>::type B;
            return to_list().template foldl<set<B>>([f] (const set<B>& s, A a) {
                    return s.insert(f(a));
                }, set<B>());
        }

        /*!
         * Monoidal append = set union.
         */
        set<A> operator + (const set<A>& other) const
        {
            return other.foldl<set<A>>(
                [] (const set<A>& s, const A& b) {return s.insert(b);},
                *this);
        }

        /*!
         * set difference a - b:  Return the value which is the first set after having removed
         * all the items in the second set from it.
         */
        set<A> operator - (const set<A>& sb) const
        {
            return sb.foldl<set<A>>([] (const set<A>& s, const A& b) { return s.remove(b); }, *this);
        }

        /*!
         * set intersection.
         */
        set<A> intersection(const set<A>& sb) const
        {
            set<A> out;
            for (auto oit = begin(); oit; oit = oit.get().next()) {
                auto elt = oit.get().get();
                if (sb.contains(elt))
                    out = out.insert(elt);
            }
            return out;
        }

        set<A> filter(std::function<bool(const A&)> pred) const
        {
            set<A> out;
            for (auto o_iter = begin(); o_iter; o_iter = o_iter.get().next()) {
                const auto& value = o_iter.get().get();
                if (pred(value))
                    out = out.insert(value);
            }
            return out;
        }
    };
}  // end namespace

template <class A>
std::ostream& operator << (std::ostream& os, heist::set<A> set) {
    bool first = true;
    os << "{";
    for (auto fs = set.to_list(); fs; fs = fs.tail()) {
        os << (first ? "" : ",") << fs.head();
        first = false;
    }
    os << "}";
    return os;
}

#endif

