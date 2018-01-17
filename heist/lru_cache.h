/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2018 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_LRUCACHE_H_
#define _HEIST_LRUCACHE_H_

#include <heist/map.h>
#include <functional>

namespace heist {
    
    /*!
     * Immutable LRU cache.
     */
    template <class K, class A>
    class lru_cache
    {
    friend class lru_cache_test;
    
    private:
        heist::map<K, std::tuple<long long, A>> values;
        heist::map<long long, K> recency;
        long long next_seq;
        int size_;
        std::function<bool(const lru_cache<K, A>&)> purge_condition;
    
        lru_cache(
            heist::map<K, std::tuple<long long, A>> values,
            heist::map<long long, K> recency,
            long long next_seq,
            int size,
            std::function<bool(const lru_cache<K, A>&)> purge_condition
        ) : values(values), recency(recency), next_seq(next_seq), size_(size), purge_condition(purge_condition) {}
    
    public:
        lru_cache(std::function<bool(const lru_cache<K, A>&)> purge_condition) 
        : next_seq(0), size_(0), purge_condition(purge_condition) {}
        lru_cache(int maxSize) : next_seq(0), size_(0), purge_condition(
            [maxSize] (const lru_cache<K,A>& cache) { return cache.size() > maxSize; }
        ) {}
    
        inline int size() const {return size_;}

        /*!
         * Make the specified key most recently used, no-op if the key doesn't exist.
         */
        lru_cache<K, A> touch(K k) const
        {
            auto va0 = values.lookup(k);
            if (va0) {
                auto va = va0.get();
                auto oldSeq = std::get<0>(va);
                auto value = std::get<1>(va);
                auto rit = recency.find(oldSeq).get();
                return lru_cache(
                    values.insert(k, std::make_tuple(next_seq, value)),
                    rit.remove().insert(next_seq, k),
                    next_seq+1,
                    size_,
                    purge_condition
                ).purge();
            }
            else
                return *this;
        }

        /*!
         * Passive look-up.
         */
        boost::optional<A> lookup(K k) const
        {
            auto ova = values.lookup(k);
            if (ova)
                return std::get<1>(ova.get());
            else
                return boost::optional<A>();
        }

        /*!
         * Insert and touch.
         */
        lru_cache<K, A> insert(K k, A a) const
        {
            auto va0 = values.lookup(k);
            if (va0) {  // exists already - update value & touch
                auto va = va0.get();
                auto oldSeq = std::get<0>(va);
                auto rit = recency.find(oldSeq).get();
                return lru_cache(
                    values.insert(k, std::make_tuple(next_seq, a)),
                    rit.remove().insert(next_seq, k),
                    next_seq+1,
                    size_,
                    purge_condition
                ).purge();
            }
            else  // adding a new item
                return lru_cache(
                    values.insert(k, std::make_tuple(next_seq, a)),
                    recency.insert(next_seq, k),
                    next_seq+1,
                    size_+1,
                    purge_condition
                ).purge();
        }

        lru_cache<K, A> remove(K k) const
        {
            auto vit0 = values.find(k);
            if (vit0) {
                auto vit = vit0.get();
                auto oldSeq = std::get<0>(vit.get_value());
                auto rit = recency.find(oldSeq).get();
                return lru_cache(
                    vit.remove(),
                    rit.remove(),
                    next_seq,
                    size_-1,
                    purge_condition
                ).purge();  // We want to support any merge condition so always check it.
            }
            else
                return *this;
        }

        /*!
         * Fetch the oldest - least recently touched - key/value pair.
         */
        boost::optional<std::tuple<K, A>> oldest() const
        {
            auto oOldestK = recency.begin();
            if (oOldestK) {
                const auto& k = oOldestK.get().get_value();
                return boost::make_optional(
                    std::tuple<K, A>(k, lookup(k).get())
                );
            }
            else
                return boost::optional<std::tuple<K, A>>();
        }
    
        heist::list<std::tuple<K, A>> to_list() const
        {
            return values.to_list().map(
                [] (std::tuple<K, std::tuple<long long, A>> ka) {
                    return std::make_tuple(std::get<0>(ka), std::get<1>(std::get<1>(ka)));
                }
            );
        }

        /*!
         * Normally old data are purged automatically. However, in the case
         * where the purge condition function depends on external state - time being
         * a common example, you might want to purge explicitly.
         */
        lru_cache<K, A> purge() const
        {
            auto rit0 = recency.begin();
            if (rit0 && purge_condition(*this)) {
                return remove(rit0.get().get_value());
            }
            else
                return *this;
        }
    
    private:
        /*!
         * Test hook: Check consistency.
         */
        inline bool is_consistent()
        {
            heist::set<long long> vuniqs;
            for (auto it0 = values.begin(); it0; it0 = it0.get().next()) {
                auto it = it0.get();
                vuniqs = vuniqs.insert(std::get<0>(it.get_value()));
            }
            heist::set<long long> runiqs;
            for (auto it0 = recency.begin(); it0; it0 = it0.get().next()) {
                auto it = it0.get();
                runiqs = runiqs.insert(it.get_key());
            }
            return vuniqs == runiqs;
        }
    };

}  // end namespace heist

#endif
