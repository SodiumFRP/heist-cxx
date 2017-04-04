/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_MAP_H_
#define _HEIST_MAP_H_

#include <heist/set.h>
#include <iostream>


namespace heist {
    template <class K, class A>
    class map
    {
    private:
        struct entry {
            entry(K k, A a)
            : k(k),
              oa(boost::make_optional(a))
            { }
            entry(K k)  // For searching
            : k(k)
            { }
            K k;
            long long unique;
            boost::optional<A> oa;

            bool operator < (const entry& other) const { return k < other.k; }
            bool operator == (const entry& other) const { return k == other.k; }
        };

        set<entry> entries;

        map(const set<entry>& entries)
            : entries(entries)
        {
        }

        static map<K, A> from_list(const heist::list<std::tuple<K,A>>& pairs)
        {
            return pairs.foldl([] (const map<K, A>& m, std::tuple<K, A> ka) {
                    return m.insert(std::get<0>(ka), std::get<1>(ka));
                }, map<K, A>());
        }

        static map<K, A> from_pairs(const heist::list<std::pair<K,A>>& pairs)
        {
            return pairs.foldl([] (const map<K, A>& m, std::pair<K, A> ka) {
                    return m.insert(ka.first, ka.second);
                }, map<K, A>());
        }

    public:
        class iterator {
            friend class map<K,A>;
        private:
            iterator(typename set<entry>::iterator it) : it(it) {}
            typename set<entry>::iterator it;
        public:
            map<K, A> remove() const
            {
                return map<K, A>(it.remove());
            }

            boost::optional<iterator> next() const {
                auto oit = it.next();
                if (oit)
                    return boost::make_optional(iterator(oit.get()));
                else
                    return boost::optional<iterator>();
            }

            boost::optional<iterator> prev() const {
                auto oit = it.prev();
                if (oit)
                    return boost::make_optional(iterator(oit.get()));
                else
                    return boost::optional<iterator>();
            }

            const K& get_key() const {return it.get().k;}
            const A& get_value() const {return it.get().oa.get();}
        };

        map() {}

        map(const heist::list<std::pair<K,A>>& pairs) {
            *this = from_pairs(pairs);
        }

        map(const heist::list<std::tuple<K,A>>& tuples) {
            *this = from_list(tuples);
        }

        map(std::initializer_list<std::pair<K,A>> il) {
            *this = from_pairs(heist::list<std::pair<K,A>>(il));
        }

        map<K, A> insert(K k, A a) const {
            return map<K, A>(entries.insert(entry(k, a)));
        }

        map<K, A> remove(K k) const {
            auto oit = find(k);
            return oit ? oit.get().remove()
                       : map<K, A>(*this);
        }

        boost::optional<iterator> begin() const {
            auto oit = entries.begin();
            if (oit)
                return boost::make_optional(iterator(oit.get()));
            else
                return boost::optional<iterator>();
        };

        boost::optional<iterator> end() const {
            auto oit = entries.end();
            if (oit)
                return boost::make_optional(iterator(oit.get()));
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
                return boost::make_optional(iterator(oit.get()));
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
                return boost::make_optional(iterator(oit.get()));
            else
                return boost::optional<iterator>();
        }

        boost::optional<iterator> find(const K& k) const {
            auto oit = entries.find(entry(k));
            if (oit)
                return boost::make_optional(iterator(oit.get()));
            else
                return boost::optional<iterator>();
        }

        boost::optional<A> lookup(const K& k) const {
            auto it0 = find(k);
            if (it0)
                return boost::make_optional(it0.get().get_value());
            else
                return boost::optional<A>();
        }

        /*!
         * Alter the specified entry in the map, where boost::optional<A>() means
         * 'not present'.
         */
        map<K, A> alter(const K& k, std::function<boost::optional<A>(boost::optional<A>)> f) const {
            auto it0 = find(k);
            if (it0) {
                auto it = it0.get();
                auto newOA = f(boost::make_optional(it.get_value()));
                if (newOA)
                    return insert(k, newOA.get());
                else
                    return it.remove();
            }
            else {
                auto newOA = f(boost::optional<A>());
                if (newOA)
                    return insert(k, newOA.get());
                else
                    return *this;
            }
        }

        /*!
         * Adjust the specified entry in the map if it's present, no-op otherwise.
         */
        map<K, A> adjust(const K& k, std::function<A(A)> f) const {
            auto it0 = find(k);
            if (it0)
                return insert(k, f(it0.get().get_value()));
            else
                return *this;
        }

        bool operator == (const map<K,A>& other) const { return entries == other.entries; }
        bool operator != (const map<K,A>& other) const { return entries != other.entries; }

        heist::list<std::tuple<K, A>> to_list() const {
            return entries.to_list().map(
                [] (const entry& e) {return std::make_tuple(e.k, e.oa.get());});
        }

        heist::list<K> keys() const {
            return entries.to_list().map(
                [] (const entry& e) {return e.k;});
        }

        heist::list<A> elems() const {
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
        map<K, typename std::result_of<Fn(A)>::type> map_(const Fn& f) const {
            typedef typename std::result_of<Fn(A)>::type B;
            return this->to_list().template foldl<map<K, B>>([f] (const map<K, B>& m, std::tuple<K, A> ka) {
                    return m.insert(std::get<0>(ka), f(std::get<1>(ka)));
                }, map<K, B>());
        }

        template <class B>
        static
        B foldl(std::function<B(const B&, const K&, const A&)> f, B a,
            boost::optional<typename heist::map<K,A>::iterator> oit)
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
        map<K, A> operator + (const map<K, A>& other) const {
            return other.foldl<map<K, A>>([] (const map<K, A>& m, const K& k, const A& a)
                      {return m.insert(k, a);}, *this);
        }
    };

    template <class K, class A>
    std::ostream& operator << (std::ostream& os, const heist::map<K, A>& m)
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
