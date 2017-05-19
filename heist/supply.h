/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_SUPPLY_H_
#define _HEIST_SUPPLY_H_

#include <boost/optional.hpp>
#include <memory>
#include <mutex>
#include <functional>

/*!
 * Functional supply of unique values.
 */
template <class A>
class supply
{
    private:
        struct common_t {
            common_t(A nextValue, const std::function<A(A)>& succ) : nextValue(nextValue), succ(succ) {}
            std::mutex mutex;
            A nextValue;
            std::function<A(A)> succ;
        };
        struct state_t {
            boost::optional<A> value;
            boost::optional<std::tuple<supply<A>, supply<A>>> supplies;
        };
        std::shared_ptr<common_t> common;
        std::shared_ptr<state_t> state;

        supply(const std::shared_ptr<common_t>& common)
          : common(common),
            state(new state_t) {}

    public:
        supply(A init_value, std::function<A(A)> succ = [] (A a) { return a+1; })
          : common(new common_t(init_value, succ)),
            state(new state_t) {}

        /*!
         * Get this supply's unique value, which is always the same for this supply
         * (no matter how much it is passed around by value).
         */
        A get() const
        {
            common->mutex.lock();
            if (!state->value) {
                state->value = boost::make_optional(common->nextValue);
                common->nextValue = common->succ(common->nextValue);
            }
            A value = state->value.get();
            common->mutex.unlock();
            return value;
        }

        /*!
         * Split this supply into two new supplies, each of which is different to the
         * input supply.
         */
        std::tuple<supply<A>, supply<A>> split2() const
        {
            using namespace std;
            using namespace boost;
            common->mutex.lock();
            if (!state->supplies)
                state->supplies = make_optional(make_tuple(supply(common), supply(common)));
            auto p = state->supplies.get();
            common->mutex.unlock();
            return p;
        }
};

#endif