/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_SEQ_H_
#define _HEIST_SEQ_H_

#include <heist/map.h>
#include <boost/optional.hpp>

namespace heist {
    /*!
     * A sequence of values, essentially an immutable equivalent of a doubly-linked list.
     */
    template <class A>
    class seq
    {
        private:
            map<int, A> m;
            seq(const map<int, A>& m) : m(m) {}

        public:
            class iterator {
                friend class seq<A>;
                private:
                    typename map<int, A>::iterator it;
                    iterator(const typename map<int, A>::iterator& it) : it(it) {}

                public:
                    boost::optional<iterator> next() {
                        auto oIt = it.next();
                        if (oIt)
                            return boost::optional<iterator>(iterator(oIt.get()));
                        else
                            return boost::optional<iterator>();
                    }

                    boost::optional<iterator> prev() {
                        auto oIt = it.prev();
                        if (oIt)
                            return boost::optional<iterator>(iterator(oIt.get()));
                        else
                            return boost::optional<iterator>();
                    }

                    A get() const { return it.get_value(); }

                    seq<A> remove() const
                    {
                        return seq<A>(it.remove());
                    }
            };
            friend class seq<A>::iterator;

            seq() {}

            boost::optional<iterator> begin() const
            {
                auto oIt = m.begin();
                if (oIt)
                    return boost::optional<iterator>(iterator(oIt.get()));
                else
                    return boost::optional<iterator>();
            }

            boost::optional<iterator> end() const
            {
                auto oIt = m.end();
                if (oIt)
                    return boost::optional<iterator>(iterator(oIt.get()));
                else
                    return boost::optional<iterator>();
            }

            seq prepend(const A& a) const {
                auto oIt = m.begin();
                if (oIt)
                    return seq<A>(m.insert(oIt.get().get_key() - 1, a));
                else
                    return seq<A>(map<int, A>().insert(0, a));
            }

            seq append(const A& a) const {
                auto oIt = m.end();
                if (oIt)
                    return seq<A>(m.insert(oIt.get().get_key() + 1, a));
                else
                    return seq<A>(map<int, A>().insert(0, a));
            }
    };
};

#endif
