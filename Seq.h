#ifndef _SEQ_H_
#define _SEQ_H_

#include "23Map.h"
#include <boost/optional.hpp>

namespace heist {
    /*!
     * A sequence of values, essentially an immutable equivalent of a doubly-linked list.
     */
    template <class A>
    class Seq
    {
        private:
            Map<int, A> m;
            Seq(const Map<int, A>& m) : m(m) {}

        public:
            class Iterator {
                friend class Seq<A>;
                private:
                    typename Map<int, A>::Iterator it;
                    Iterator(const typename Map<int, A>::Iterator& it) : it(it) {}

                public:
                    boost::optional<Iterator> next() {
                        auto oIt = it.next();
                        if (oIt)
                            return boost::optional<Iterator>(Iterator(oIt.get()));
                        else
                            return boost::optional<Iterator>();
                    }

                    boost::optional<Iterator> prev() {
                        auto oIt = it.prev();
                        if (oIt)
                            return boost::optional<Iterator>(Iterator(oIt.get()));
                        else
                            return boost::optional<Iterator>();
                    }

                    A get() const { return it.getValue(); }

                    Seq<A> remove() const
                    {
                        return Seq<A>(it.remove());
                    }
            };
            friend class Seq<A>::Iterator;

            Seq() {}

            boost::optional<Iterator> begin() const
            {
                auto oIt = m.begin();
                if (oIt)
                    return boost::optional<Iterator>(Iterator(oIt.get()));
                else
                    return boost::optional<Iterator>();
            }

            boost::optional<Iterator> end() const
            {
                auto oIt = m.end();
                if (oIt)
                    return boost::optional<Iterator>(Iterator(oIt.get()));
                else
                    return boost::optional<Iterator>();
            }

            Seq prepend(const A& a) const {
                auto oIt = m.begin();
                if (oIt)
                    return Seq<A>(m.insert(oIt.get().getKey() - 1, a));
                else
                    return Seq<A>(Map<int, A>().insert(0, a));
            }

            Seq append(const A& a) const {
                auto oIt = m.end();
                if (oIt)
                    return Seq<A>(m.insert(oIt.get().getKey() + 1, a));
                else
                    return Seq<A>(Map<int, A>().insert(0, a));
            }
    };
};

#endif
