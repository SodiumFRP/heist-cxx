// $Id$

#ifndef _SUPPLY_H_
#define _SUPPLY_H_

#include <boost/optional.hpp>
#include <tr1/memory>
#include "Mutex.h"

/*!
 * Functional supply of unique values.
 */
template <class A>
class Supply
{
    private:
        struct Common {
            Common(A nextValue, const std::function<A(A)>& succ) : nextValue(nextValue), succ(succ) {}
            Mutex mutex;
            A nextValue;
            std::function<A(A)> succ;
        };
        struct State {
            boost::optional<A> value;
            boost::optional<std::tuple<Supply<A>, Supply<A>>> supplies;
        };
        std::tr1::shared_ptr<Common> common;
        std::tr1::shared_ptr<State> state;

        Supply(const std::tr1::shared_ptr<Common>& common)
          : common(common),
            state(new State) {}

    public:
        Supply(A initValue, std::function<A(A)> succ = [] (A a) { return a+1; })
          : common(new Common(initValue, succ)),
            state(new State) {}

        /*!
         * Get this supply's unique value, which is always the same for this supply
         * (no matter how much it is passed around by value).
         */
        A get() const
        {
            MutexLock ml(common->mutex);
            if (!state->value) {
                state->value = boost::make_optional(common->nextValue);
                common->nextValue = common->succ(common->nextValue);
            }
            return state->value.get();
        }

        /*!
         * Split this supply into two new supplies, each of which is different to the
         * input supply.
         */
        std::tuple<Supply<A>, Supply<A>> split2() const
        {
            using namespace std;
            using namespace boost;
            MutexLock ml(common->mutex);
            if (!state->supplies)
                state->supplies = make_optional(make_tuple(Supply(common), Supply(common)));
            return state->supplies.get();
        }
};

#endif

