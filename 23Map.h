// $Id$

#ifndef _23MAP_H_
#define _23MAP_H_

#include "23Set.h"
#include <iostream>


namespace heist {
    template <class K, class A, class L, class MAP>
    class Map_
    {
    private:
        struct Entry {
            Entry(K k, A a)
            : k(k),
              oa(boost::make_optional(a))
            { }
            Entry(K k)  // For searching
            : k(k)
            { }
            K k;
            long long unique;
            boost::optional<A> oa;

            bool operator < (const Entry& other) const { return k < other.k; }
            bool operator == (const Entry& other) const { return k == other.k; }
        };

        LSet<Entry, L> entries;

        Map_(const LSet<Entry, L>& entries)
            : entries(entries)
        {
        }

    public:
        class Iterator {
            friend class Map_<K,A,L,MAP>;
        private:
            Iterator(typename LSet<Entry, L>::Iterator it) : it(it) {}
            typename LSet<Entry, L>::Iterator it;
        public:
            MAP remove() const
            {
                return MAP(it.remove());
            }

            boost::optional<Iterator> next() const {
                auto oit = it.next();
                if (oit)
                    return boost::make_optional(Iterator(oit.get()));
                else
                    return boost::optional<Iterator>();
            }

            boost::optional<Iterator> prev() const {
                auto oit = it.prev();
                if (oit)
                    return boost::make_optional(Iterator(oit.get()));
                else
                    return boost::optional<Iterator>();
            }

            const K& getKey() const {return it.get().k;}
            const A& getValue() const {return it.get().oa.get();}
        };

        Map_() {}

        static MAP fromList(const heist::list<std::tuple<K,A>>& pairs)
        {
            return foldl<MAP, std::tuple<K,A>>([] (const MAP& m, std::tuple<K, A> ka) {
                    return m.insert(std::get<0>(ka), std::get<1>(ka));
                }, MAP(), pairs);
        }

        static MAP fromPairs(const heist::list<std::pair<K,A>>& pairs)
        {
            return foldl<MAP, std::pair<K,A>>([] (const MAP& m, std::pair<K, A> ka) {
                    return m.insert(ka.first, ka.second);
                }, MAP(), pairs);
        }

        Map_(const heist::list<std::pair<K,A>>& pairs) {
            *this = fromPairs(pairs);
        }

        Map_(std::initializer_list<std::pair<K,A>> il) {
            *this = fromPairs(heist::list<std::pair<K,A>>(il));
        }

        MAP insert(K k, A a) const {
            return MAP(entries.insert(Entry(k, a)));
        }

        MAP remove(K k) const {
            auto oit = find(k);
            return oit ? oit.get().remove()
                       : MAP(*this);
        }

        boost::optional<Iterator> begin() const {
            auto oit = entries.begin();
            if (oit)
                return boost::make_optional(Iterator(oit.get()));
            else
                return boost::optional<Iterator>();
        };

        boost::optional<Iterator> end() const {
            auto oit = entries.end();
            if (oit)
                return boost::make_optional(Iterator(oit.get()));
            else
                return boost::optional<Iterator>();
        };

        /*!
         * Return an iterator pointing to the smallest value >= the pivot,
         * undefined if all values are < the pivot.
         */
        boost::optional<Iterator> lower_bound(const K& k) const {
            auto oit = entries.lower_bound(Entry(k));
            if (oit)
                return boost::make_optional(Iterator(oit.get()));
            else
                return boost::optional<Iterator>();
        }

        /*!
         * Return an iterator pointing to the largest value <= the pivot,
         * undefined if all values are > the pivot.
         */
        boost::optional<Iterator> upper_bound(const K& k) const {
            auto oit = entries.upper_bound(Entry(k));
            if (oit)
                return boost::make_optional(Iterator(oit.get()));
            else
                return boost::optional<Iterator>();
        }

        boost::optional<Iterator> find(const K& k) const {
            auto oit = entries.find(Entry(k));
            if (oit)
                return boost::make_optional(Iterator(oit.get()));
            else
                return boost::optional<Iterator>();
        }

        boost::optional<A> lookup(const K& k) const {
            auto it0 = find(k);
            if (it0)
                return boost::make_optional(it0.get().getValue());
            else
                return boost::optional<A>();
        }

        /*!
         * Alter the specified entry in the map, where boost::optional<A>() means
         * 'not present'.
         */
        MAP alter(const K& k, std::function<boost::optional<A>(boost::optional<A>)> f) const {
            auto it0 = find(k);
            if (it0) {
                auto it = it0.get();
                auto newOA = f(boost::make_optional(it.getValue()));
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
        MAP adjust(const K& k, std::function<A(A)> f) const {
            auto it0 = find(k);
            if (it0)
                return insert(k, f(it0.get().getValue()));
            else
                return *this;
        }

        /*!
         * Monoidal append = set union.
         */
        MAP operator + (const Map_<K,A,L,MAP>& other) const;

        bool operator == (const Map_<K,A,L,MAP>& other) const { return entries == other.entries; }
        bool operator != (const Map_<K,A,L,MAP>& other) const { return entries != other.entries; }

        heist::list<std::tuple<K, A>> toList() const {
            return entries.toList().map(
                [] (const Entry& e) {return std::make_tuple(e.k, e.oa.get());});
        }

        heist::list<K> keys() const {
            return entries.toList().map(
                [] (const Entry& e) {return e.k;});
        }

        heist::list<A> elems() const {
            return entries.toList().map(
                [] (const Entry& e) {return e.oa.get();});
        }

        size_t size() const {
            size_t len = 0;
            auto oit = begin();
            while (oit) {len++; oit = oit.get().next();}
            return len;
        }
    };

    /*!
     * Thread-safe variant of Map.
     */
    template <class K, class A>
    class Map : public Map_<K, A, PooledLocker, Map<K, A>>
    {
        public:
            Map() {}
            Map(const Map_<K, A, PooledLocker, Map<K, A>>& other) : Map_<K, A, PooledLocker, Map<K, A>>(other) {}
            Map(const heist::list<std::pair<K,A>>& pairs) : Map_<K, A, PooledLocker, Map<K, A>>(pairs) {}
            Map(std::initializer_list<std::pair<K,A>> il) : Map_<K, A, PooledLocker, Map<K, A>>(il) {}
    };

    /*!
     * Thread-unsafe but fast variant of Map.
     */
    template <class K, class A>
    class UMap : public Map_<K, A, NullLocker, UMap<K, A>>
    {
        public:
            UMap() {}
            UMap(const Map_<K, A, NullLocker, UMap<K, A>>& other) : Map_<K, A, NullLocker, UMap<K, A>>(other) {}
            UMap(const heist::list<std::pair<K,A>>& pairs) : Map_<K, A, NullLocker, UMap<K, A>>(pairs) {}
            UMap(std::initializer_list<std::pair<K,A>> il) : Map_<K, A, NullLocker, UMap<K, A>>(il) {}
    };

    /*!
     * Map a function over the map elements.
     */
    template <class K, class A, class B>
    Map<K, B> fmap(std::function<B(const A&)> f, const Map<K, A>& other) {
        return foldl<Map<K, B>, std::tuple<K,A>>([=] (const Map<K, B>& m, std::tuple<K, A> ka) {
                return m.insert(std::get<0>(ka), f(std::get<1>(ka)));
            }, Map<K, B>(), other.toList());
    }

    /*!
     * Map a function over the map elements.
     */
    template <class K, class A, class B>
    UMap<K, B> fmap(std::function<B(const A&)> f, const UMap<K, A>& other) {
        return foldl<UMap<K, B>, std::tuple<K,A>>([=] (const UMap<K, B>& m, std::tuple<K, A> ka) {
                return m.insert(std::get<0>(ka), f(std::get<1>(ka)));
            }, UMap<K, B>(), other.toList());
    }

    template <class Acc, class K, class A>
    Acc foldl(std::function<Acc(const Acc&, const K&, const A&)> f, Acc a,
        boost::optional<typename heist::Map<K,A>::Iterator> oit)
    {
        while (oit) {
            auto it = oit.get();
            a = f(a, it.getKey(), it.getValue());
            oit = it.next();
        }
        return a;
    }

    template <class Acc, class K, class A>
    Acc foldl(std::function<Acc(const Acc&, const K&, const A&)> f, Acc a,
        boost::optional<typename heist::UMap<K,A>::Iterator> oit)
    {
        while (oit) {
            auto it = oit.get();
            a = f(a, it.getKey(), it.getValue());
            oit = it.next();
        }
        return a;
    }

    /*!
     * Fold the map's values into a single value using the specified accumulator function.
     */
    template <class Acc, class K, class A>
    Acc foldl(std::function<Acc(const Acc&, const K&, const A&)> f, const Acc& a, const heist::Map<K, A>& m)
    {
        return foldl(f, a, m.begin());
    }

    /*!
     * Fold the map's values into a single value using the specified accumulator function.
     */
    template <class Acc, class K, class A>
    Acc foldl(std::function<Acc(const Acc&, const K&, const A&)> f, const Acc& a, const heist::UMap<K, A>& m)
    {
        return foldl(f, a, m.begin());
    }

    /*!
     * Monoidal append = set union.
     */
    template <class K, class A, class L, class MAP>
    MAP Map_<K, A, L, MAP>::operator + (const Map_<K, A, L, MAP>& other) const {
        return foldl<MAP,K,A>([] (const MAP& m, const K& k, const A& a)
                  {return m.insert(k, a);}, MAP(*this), MAP(other));
    }

    template <class K, class A>
    std::ostream& operator << (std::ostream& os, const heist::UMap<K, A>& m)
    {
        os << "{";
        bool first = true;
        for (auto oit = m.begin(); oit; oit = oit.get().next()) {
            if (first) first = false; else os << "," << std::endl;
            os << oit.get().getKey();
            os << " -> ";
            os << oit.get().getValue();
        }
        os << "}";
        return os;
    }

    template <class K, class A>
    std::ostream& operator << (std::ostream& os, const heist::Map<K, A>& m)
    {
        os << "{";
        bool first = true;
        for (auto oit = m.begin(); oit; oit = oit.get().next()) {
            if (first) first = false; else os << "," << std::endl;
            os << oit.get().getKey();
            os << " -> ";
            os << oit.get().getValue();
        }
        os << "}";
        return os;
    }
};

#endif
