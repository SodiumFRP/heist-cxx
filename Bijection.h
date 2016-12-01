// $Id$

#ifndef _BIJECTION_H_
#define _BIJECTION_H_

#include "23Map.h"

namespace heist {
    
    /*!
     * An efficiently searched one-to-one correspondence between one value and
     * another.
     */
    template <class L, class R>
    class Bijection {
        private:
            Map<L,R> forward;
            Map<R,L> back;

            Bijection(const Map<L,R>& forward, const Map<R,L>& back)
                : forward(forward), back(back) {}
        public:
            Bijection() {}

            /*!
             * Add a two-way association between l and r to our correspondence.
             */
            Bijection<L,R> associate(const L& l, const R& r) const {
                return Bijection<L,R>(forward.insert(l,r), back.insert(r,l));
            }
            
            /*!
             * Remove the two-way correspondence between l and whatever we associated it with.
             */
            Bijection<L,R> forwardUnassociate(const L& l) const {
                auto oR = forwardAssociation(l);
                return oR ? unassociate(l, oR.get()) : *this;
            }

            /*!
             *  Remove the two-way correspondence between r and whatever we associated it with.
             */
            Bijection<L,R> backUnassociate(const R& r) const {
                auto oL = backAssociation(r);
                return oL ? unassociate(oL.get(), r) : *this;
            }

            /*!
             * Return what we associated l with.
             */
            boost::optional<R> forwardAssociation(const L& l) const {
                return forward.lookup(l);
            }

            /*!
             * Return what we associated r with.
             */
            boost::optional<L> backAssociation(const R& r) const {
                return back.lookup(r);
            }

            class Iterator {
                friend class Bijection<L,R>;
                private:
                    typename heist::Map<L,R>::Iterator iter;

                    Iterator(const typename heist::Map<L,R>::Iterator& iter) : iter(iter) {}

                public:
                    boost::optional<Iterator> next() const {
                        auto oIt = iter.next();
                        return oIt ? boost::optional<Iterator>(Iterator(oIt.get()))
                                   : boost::optional<Iterator>();
                    }

                    boost::optional<Iterator> prev() const {
                        auto oIt = iter.prev();
                        return oIt ? boost::optional<Iterator>(Iterator(oIt.get()))
                                   : boost::optional<Iterator>();
                    }

                    const L& getLeft() const { return iter.getKey(); }
                    const R& getRight() const { return iter.getValue(); }
            };

            boost::optional<Iterator> begin() const {
                auto oIt = forward.begin();
                return oIt ? boost::optional<Iterator>(Iterator(oIt.get()))
                           : boost::optional<Iterator>();
            }

            boost::optional<Iterator> end() const {
                auto oIt = forward.end();
                return oIt ? boost::optional<Iterator>(Iterator(oIt.get()))
                           : boost::optional<Iterator>();
            }

        private:
            Bijection<L,R> unassociate(const L& l, const R& r) const {
                return Bijection<L,R>(forward.remove(l), back.remove(r));
            }
    };
};

#endif

