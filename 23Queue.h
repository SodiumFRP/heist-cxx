// $Id$

#ifndef _23QUEUE_H_
#define _23QUEUE_H_

#include <23Map.h>
#include "InternalError.h"

namespace heist {

    template <class A>
    class Queue {
        private:
            Map<int, A> m;
            int head, tail;
    
        private:
            Queue(Map<int, A> m, int head, int tail) : m(m), head(head), tail(tail) {}

        public:
            Queue() : head(0), tail(0) {}

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
            Queue<A> push(A a) const {
                return Queue<A>(m.insert(tail, a), head, tail+1);
            }

            /*!
             * Pop an item from the head of the queue.  Will throw an exception
             * if the queue is empty.
             */
            std::tuple<A, Queue<A>> pop() const {
                auto oit = m.find(head);
                if (oit)
                    return std::make_tuple(oit.get().getValue(), Queue<A>(oit.get().remove(), head+1, tail));
                else
                    THROW(InternalError, "Queue::pop() empty");
            }
    };

};

#endif

