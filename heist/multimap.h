/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_MULTIMAP_H_
#define _HEIST_MULTIMAP_H_

#include <heist/set.h>
#include <heist/supply.h>
#include <iostream>


namespace heist {
    template <class K, class A>
    class multimap
    {
    private:
        struct entry {
            entry(K k, long long unique, A a)
            : k(k),
              unique(unique),
              oa(boost::make_optional(a))
            { }
            entry(K k)  // For searching
            : k(k),
              unique(0)
            { }
            K k;
            long long unique;
            boost::optional<A> oa;

            bool operator < (const entry& other) const {
                return k == other.k ? unique < other.unique
                                    : k < other.k;
            }
            bool operator == (const entry& other) const {
                return k == other.k && unique == other.unique;
            }
        };

        set<entry> entries;
        supply<long long> sup;     // We assume this supply's value has already been used,
                                   // so it must always be split before using.

        multimap(const set<entry>& entries, const supply<long long>& sup)
            : entries(entries),
              sup(sup)
        {
        }

        static multimap<K, A> from_list(const heist::list<std::tuple<K,A>>& pairs)
        {
            return pairs.template foldl<multimap<K, A>>([] (const multimap<K, A>& m, const std::tuple<K, A>& ka) {
                    return m.insert(std::get<0>(ka), std::get<1>(ka));
                }, multimap<K, A>());
        }

        static multimap<K, A> from_pairs(const heist::list<std::pair<K,A>>& pairs)
        {
            return pairs.template foldl<multimap<K, A>>([] (const multimap<K, A>& m, const std::pair<K, A>& ka) {
                    return m.insert(ka.first, ka.second);
                }, multimap<K, A>());
        }

    public:
        class iterator {
            friend class multimap<K,A>;
        private:
            iterator(typename set<entry>::iterator it, supply<long long> sup) : it(it), sup(sup) {}
            typename set<entry>::iterator it;
            supply<long long> sup;
        public:
            multimap<K, A> remove() const
            {
                return multimap<K, A>(it.remove(), sup);
            }

            boost::optional<iterator> next() const {
                auto oit = it.next();
                if (oit)
                    return boost::make_optional(iterator(oit.get(), sup));
                else
                    return boost::optional<iterator>();
            }

            boost::optional<iterator> prev() const {
                auto oit = it.prev();
                if (oit)
                    return boost::make_optional(iterator(oit.get(), sup));
                else
                    return boost::optional<iterator>();
            }

            const K& get_key() const {return it.get().k;}
            const A& get_value() const {return it.get().oa.get();}
        };

        multimap() : sup(0) {}

        multimap(const heist::list<std::pair<K,A>>& pairs) : sup(0) {
            *this = from_pairs(pairs);
        }

        multimap(const heist::list<std::tuple<K,A>>& tuples) : sup(0) {
            *this = from_list(tuples);
        }

        multimap(std::initializer_list<std::pair<K,A>> il) : sup(0) {
            *this = from_pairs(heist::list<std::pair<K,A>>(il));
        }

        multimap<K, A> insert(K k, A a) const {
            auto p = sup.split2();
            long long unique = std::get<0>(p).get();
            return multimap<K, A>(entries.insert(entry(k, unique, a)), std::get<1>(p));
        }

        multimap<K, A> remove(K k) const {
            auto oit = find(k);
            return oit ? oit.get().remove()
                       : multimap<K, A>(*this);
        }

        boost::optional<iterator> begin() const {
            auto oit = entries.begin();
            if (oit)
                return boost::make_optional(iterator(oit.get(), sup));
            else
                return boost::optional<iterator>();
        };

        boost::optional<iterator> end() const {
            auto oit = entries.end();
            if (oit)
                return boost::make_optional(iterator(oit.get(), sup));
            else
                return boost::optional<iterator>();
        };

        /*!
         * Return an iterator pointing to the smallest value >= the pivot,
         * undefined if all values are < the pivot.
         */
        boost::optional<iterator> lower_bound(const K& k) const {
            auto oit = entries.lower_bound(entry(k));
            if (oit)
                return boost::make_optional(iterator(oit.get(), sup));
            else
                return boost::optional<iterator>();
        }

        /*!
         * Return an iterator pointing to the largest value <= the pivot,
         * undefined if all values are > the pivot.
         */
        boost::optional<iterator> upper_bound(const K& k) const {
            auto oit = entries.upper_bound(entry(k));
            if (oit)
                return boost::make_optional(iterator(oit.get(), sup));
            else
                return boost::optional<iterator>();
        }

        bool operator == (const multimap<K, A>& other) const {
            boost::optional<multimap<K, A>::iterator> it1 = begin();
            boost::optional<multimap<K, A>::iterator> it2 = other.begin();
            while (it1 && it2) {
                if (!(it1.get().get_key() == it2.get().get_key())) return false;
                if (!(it1.get().get_value() == it2.get().get_value())) return false;
                it1 = it1.get().next();
                it2 = it2.get().next();
            }
            return !it1 && !it2;
        }

        bool operator != (const multimap<K, A>& other) const {
            return ! (*this == other);
        }

        bool operator < (const multimap<K, A>& other) const {
            boost::optional<multimap<K, A>::iterator> it1 = begin();
            boost::optional<multimap<K, A>::iterator> it2 = other.begin();
            while (it1 && it2) {
                if (it1.get().get_key() < it2.get().get_key()) return true;
                if (it2.get().get_key() < it1.get().get_key()) return false;
                if (it1.get().get_value() < it2.get().get_value()) return true;
                if (it2.get().get_value() < it1.get().get_value()) return false;
                it1 = it1.get().next();
                it2 = it2.get().next();
            }
            return (bool)it2;
        }

        bool operator > (const multimap<K, A>& other) const {
            return other < *this;
        }

        bool operator <= (const multimap<K, A>& other) const {
            return !(*this > other);
        }

        bool operator >= (const multimap<K, A>& other) const {
            return !(*this < other);
        }

        heist::list<std::tuple<K, A>> to_list() const {
            return entries.to_list().map(
                [] (const entry& e) {return std::make_tuple(e.k, e.oa.get());});
        }

        heist::list<K> keys() const {
            return entries.to_list().map(
                [] (const entry& e) {return e.k;});
        }

        heist::list<A> values() const {
            return entries.to_list().map(
                [] (const entry& e) {return e.oa.get();});
        }

        size_t size() const {
            size_t len = 0;
            auto oit = begin();
            while (oit) {len++; oit = oit.get().next();}
            return len;
        }

        /*!
         * map a function over the map elements.
         */
        template <class Fn>
        multimap<K, typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
            typedef typename std::result_of<Fn(A)>::type B;
            return this->to_list().template foldl<multimap<K, B>>([f] (const multimap<K, B>& m, std::tuple<K, A> ka) {
                    return m.insert(std::get<0>(ka), f(std::get<1>(ka)));
                }, multimap<K, B>());
        }

        template <class B>
        static
        B foldl(std::function<B(const B&, const K&, const A&)> f, B a,
            boost::optional<typename heist::multimap<K,A>::iterator> oit)
        {
            while (oit) {
                auto it = oit.get();
                a = f(a, it.get_key(), it.get_value());
                oit = it.next();
            }
            return a;
        }

        /*!
         * Fold the map's values into a single value using the specified accumulator function.
         */
        template <class B>
        B foldl(std::function<B(const B&, const K&, const A&)> f, const B& a) const
        {
            return foldl<B>(f, a, this->begin());
        }

        /*!
         * Monoidal append = set union.
         */
        multimap<K, A> operator + (const multimap<K, A>& other) const {
            return other.foldl<multimap<K, A>>([] (const multimap<K, A>& m, const K& k, const A& a)
                      {return m.insert(k, a);}, *this);
        }

        multimap<K, A> filter(std::function<bool(const A&)> pred) const
        {
            multimap<K, A> out;
            for (auto o_iter = begin(); o_iter; o_iter = o_iter.get().next()) {
                const auto& key = o_iter.get().get_key();
                const auto& value = o_iter.get().get_value();
                if (pred(value))
                    out = out.insert(key, value);
            }
            return out;
        }

        multimap<K, A> filter_with_key(std::function<bool(const K&, const A&)> pred) const
        {
            multimap<K, A> out;
            for (auto o_iter = begin(); o_iter; o_iter = o_iter.get().next()) {
                const auto& key = o_iter.get().get_key();
                const auto& value = o_iter.get().get_value();
                if (pred(key, value))
                    out = out.insert(key, value);
            }
            return out;
        }

        /*!
         * True if this map contains any elements.
         */
        operator bool () const { return (bool)entries; }
    };

    template <class K, class A>
    std::ostream& operator << (std::ostream& os, const heist::multimap<K, A>& m)
    {
        os << "{";
        bool first = true;
        for (auto oit = m.begin(); oit; oit = oit.get().next()) {
            if (first) first = false; else os << "," << std::endl;
            os << oit.get().get_key();
            os << " -> ";
            os << oit.get().get_value();
        }
        os << "}";
        return os;
    }
};

#endif
