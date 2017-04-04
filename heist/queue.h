/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_QUEUE_H_
#define _HEIST_QUEUE_H_

#include <heist/map.h>
#include <stdexcept>

namespace heist {

    template <class A>
    class queue {
        private:
            map<int, A> m;
            int head, tail;

        private:
            queue(map<int, A> m, int head, int tail) : m(m), head(head), tail(tail) {}

        public:
            queue() : head(0), tail(0) {}

            /*!
             * True if this queue has anything in it.
             */
            operator bool() const
            {
                return head != tail;
            }

            /*! 
             * Push an item onto the tail of the queue.
             */
            queue<A> push(A a) const {
                return queue<A>(m.insert(tail, a), head, tail+1);
            }

            /*!
             * Pop an item from the head of the queue.  Will throw an exception
             * if the queue is empty.
             */
            std::tuple<A, queue<A>> pop() const {
                auto oit = m.find(head);
                if (oit)
                    return std::make_tuple(oit.get().get_value(), queue<A>(oit.get().remove(), head+1, tail));
                else
                    throw std::runtime_error("queue::pop() empty");
            }
    };

};

#endif

