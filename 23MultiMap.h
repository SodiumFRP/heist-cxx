// $Id$

#ifndef _23MULTIMAP_H_
#define _23MULTIMAP_H_

#include "Supply.h"
#include "23Set.h"


namespace heist {
    template <class K, class A>
    class MultiMap
    {
    private:
        struct Entry {
            Entry(K k, long long unique, A a)
            : k(k),
              unique(unique),
              oa(boost::make_optional(a))
            { }
            Entry(K k)  // For searching
            : k(k),
              unique(0)
            { }
            K k;
            long long unique;
            boost::optional<A> oa;

            bool operator < (const Entry& other) const {
                return k == other.k ? unique < other.unique
                                    : k < other.k; 
            }

            bool operator == (const Entry& other) const {
                return k == other.k && unique == other.unique;
            }
        };

        Set<Entry> entries;
        Supply<long long> supply;  // We assume this supply's value has already been used,
                                   // so it must always be split before using.

        MultiMap(Set<Entry> entries, Supply<long long> supply)
        : entries(entries),
          supply(supply)
        {
        }

    public:
        class Iterator {
            friend class MultiMap<K,A>;
        private:
            Iterator(typename Set<Entry>::Iterator it, Supply<long long> supply) : it(it), supply(supply) {}
            typename Set<Entry>::Iterator it;
            Supply<long long> supply;
        public:
            MultiMap<K,A> remove()
            {
                return MultiMap<K,A>(it.remove(), supply);
            }

            boost::optional<Iterator> next() {
                auto oit = it.next();
                if (oit)
                    return boost::make_optional(Iterator(oit.get(), supply));
                else
                    return boost::optional<Iterator>();
            }

            boost::optional<Iterator> prev() {
                auto oit = it->prev();
                if (oit)
                    return boost::make_optional(Iterator(oit.get(), supply));
                else
                    return boost::optional<Iterator>();
            }

            K getKey() {return it.get().k;}
            A getValue() {return it.get().oa.get();}
        };

        MultiMap() : supply(0) {}

        MultiMap<K, A> insert(K k, A a) const {
            auto p = supply.split2();
            long long unique = std::get<0>(p).get();
            return MultiMap(entries.insert(Entry(k, unique, a)), std::get<1>(p));
        }
        
        boost::optional<Iterator> begin() const {
            auto oit = entries.begin();
            if (oit)
                return boost::make_optional(Iterator(oit.get(), supply));
            else
                return boost::optional<Iterator>();
        };
        
        boost::optional<Iterator> end() const {
            auto oit = entries.end();
            if (oit)
                return boost::make_optional(Iterator(oit.get(), supply));
            else
                return boost::optional<Iterator>();
        };

        /*!
         * Return an iterator pointing to the smallest value >= the pivot,
         * undefined if all values are < the pivot.
         */
        boost::optional<Iterator> lower_bound(K k) const {
            auto oit = entries.lower_bound(Entry(k));
            if (oit)
                return boost::make_optional(Iterator(oit.get(), supply));
            else
                return boost::optional<Iterator>();
        }

        /*!
         * Return an iterator pointing to the largest value <= the pivot,
         * undefined if all values are > the pivot.
         */
        boost::optional<Iterator> upper_bound(K k) const {
            auto oit = entries.upper_bound(Entry(k));
            if (oit)
                return boost::make_optional(Iterator(oit.get(), supply));
            else
                return boost::optional<Iterator>();
        }

        bool operator == (const MultiMap<K, A>& other) const { return entries == other.entries; }
        bool operator != (const MultiMap<K, A>& other) const { return entries != other.entries; }
    };
};

#endif

