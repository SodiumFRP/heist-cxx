// $Id$

#ifndef _LRUCACHE_H_
#define _LRUCACHE_H_

#include "23Map.h"
#include "Unit.h"
#include <functional>

/*!
 * Immutable LRU cache.
 */
template <class K, class A, class U = Unit>
class LRUCache
{
friend class LRUCache_test;

private:
    heist::Map<K, std::tuple<long long, A>> values;
    heist::Map<long long, K> recency;
    long long nextSeq;
    int size_;
    std::function<bool(const LRUCache<K, A, U>&, const U&)> purgeCondition;

    LRUCache(
        heist::Map<K, std::tuple<long long, A>> values,
        heist::Map<long long, K> recency,
        long long nextSeq,
        int size,
        std::function<bool(const LRUCache<K, A, U>&, const U&)> purgeCondition
    ) : values(values), recency(recency), nextSeq(nextSeq), size_(size), purgeCondition(purgeCondition) {}

public:
    LRUCache(std::function<bool(const LRUCache<K, A, U>&, const U&)> purgeCondition) 
    : nextSeq(0), size_(0), purgeCondition(std::move(purgeCondition)) {}
    LRUCache(int maxSize) : nextSeq(0), size_(0), purgeCondition(
        [maxSize] (const LRUCache<K, A, U>& cache, const U&) { return cache.size() > maxSize; }
    ) {}

    inline int size() const {return size_;}

    /*!
     * Make the specified key most recently used, no-op if the key doesn't exist.
     */
    LRUCache<K, A, U> touch(K k) const
    {
        auto va0 = values.lookup(k);
        if (va0) {
            auto va = va0.get();
            auto oldSeq = std::get<0>(va);
            auto value = std::get<1>(va);
            auto rit = recency.find(oldSeq).get();
            return LRUCache(
                values.insert(k, std::make_tuple(nextSeq, value)),
                rit.remove().insert(nextSeq, k),
                nextSeq+1,
                size_,
                purgeCondition
            );
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
    LRUCache<K, A, U> insert(K k, A a, U u = U()) const
    {
        auto va0 = values.lookup(k);
        if (va0) {  // exists already - update value & touch
            auto va = va0.get();
            auto oldSeq = std::get<0>(va);
            auto rit = recency.find(oldSeq).get();
            return LRUCache(
                values.insert(k, std::make_tuple(nextSeq, a)),
                rit.remove().insert(nextSeq, k),
                nextSeq+1,
                size_,
                purgeCondition
            ).purge(u);
        }
        else  // adding a new item
            return LRUCache(
                values.insert(k, std::make_tuple(nextSeq, a)),
                recency.insert(nextSeq, k),
                nextSeq+1,
                size_+1,
                purgeCondition
            ).purge(u);
    }

    LRUCache<K, A, U> remove(K k) const
    {
        auto vit0 = values.find(k);
        if (vit0) {
            auto vit = vit0.get();
            auto oldSeq = std::get<0>(vit.getValue());
            auto rit = recency.find(oldSeq).get();
            return LRUCache(
                vit.remove(),
                rit.remove(),
                nextSeq,
                size_-1,
                purgeCondition
            );  // We want to support any merge condition so always check it.
        }
        else
            return *this;
    }

    heist::list<std::tuple<K, A>> toList() const
    {
        return values.toList().map(
            [] (std::tuple<K, std::tuple<long long, A>> ka) {
                return std::make_tuple(std::get<0>(ka), std::get<1>(std::get<1>(ka)));
            }
        );
    }

    boost::optional<K> oldest() const
    {
        auto oIter = recency.begin();
        return oIter
            ? boost::optional<K>(oIter.get().getValue())
            : boost::optional<K>();
    }

    /*!
     * Explicitly purge, which is only ever necessary if the purge condition
     * depends on something that can change.
     */
    LRUCache<K, A, U> purge(const U& u) const
    {
        LRUCache<K, A, U> me = *this;
        while (auto rit0 = recency.begin()) {
            if (purgeCondition(me, u))
                me = remove(rit0.get().getValue());
            else
                break;
        }
        return me;
    }

private:
    /*!
     * Test hook: Check consistency.
     */
    inline bool isConsistent()
    {
        heist::Set<long long> vuniqs;
        for (auto it0 = values.begin(); it0; it0 = it0.get().next()) {
            auto it = it0.get();
            vuniqs = vuniqs.insert(std::get<0>(it.getValue()));
        }
        heist::Set<long long> runiqs;
        for (auto it0 = recency.begin(); it0; it0 = it0.get().next()) {
            auto it = it0.get();
            runiqs = runiqs.insert(it.getKey());
        }
        return vuniqs == runiqs;
    }
};

#endif

