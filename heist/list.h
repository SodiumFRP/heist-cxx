/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#ifndef _HEIST_LIST_H_
#define _HEIST_LIST_H_

#include <functional>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <mutex>
#include <vector>
#include <iostream>
#include <list>
#include <initializer_list>
#include <heist/lock_pool.h>



// ------ New implementation

namespace heist {

    template <class A>
    struct cons;

    template <class A>
    void intrusive_ptr_add_ref(cons<A>* p)
    {
        heist::impl::spin_lock* l = heist::impl::spin_get_and_lock(p);
        p->ref_count++;
        l->unlock();
    }

    template <class A>
    void intrusive_ptr_release(cons<A>* p)
    {
        heist::impl::spin_lock* l = heist::impl::spin_get_and_lock(p);
        if (--p->ref_count == 0) {
            l->unlock();
            delete p;
        }
        else
            l->unlock();
    }

    template <class A>
    struct cons {
        cons(const A& head, const boost::intrusive_ptr<cons<A>>& tail) : ref_count(0), head(head), tail(tail) {}
        ~cons() {
            // Optimization to allow it to clean up long lists without
            // using up the stack.
            if (tail && tail->ref_count == 1) {
                std::vector<cons<A>*> ptrs;
                cons<A>* p = this;
                while (true) {
                    boost::intrusive_ptr<cons<A>>& l = p->tail;
                    if (!l)
                        break;
                    p = &*l;
                    if (!p->tail || p->tail->ref_count != 1)
                        break;
                    ptrs.push_back(p);
                }
                for (int i= (int)ptrs.size()-1; i >= 0; i--)
                    ptrs[i]->tail = NULL;
            }
        }
        int ref_count;
        A head;
        boost::intrusive_ptr<cons<A>> tail;
    };
}

namespace heist {

    template <class A> class list;
    template <class A> list<A> concat(list<list<A>> lists);
    template <class A> list<A> cat_optional(list<boost::optional<A>> xs);

    template <class A> class list
    {
        friend class cons<A>;
        private:
            boost::intrusive_ptr<cons<A>> ocons;
            list(const boost::intrusive_ptr<cons<A>>& ocons) : ocons(ocons) {}

        public:
            typedef A value_type;

            /*!
             * An empty list.
             */
            list() {}
            /*!
             * construct a list.  Better to use % operator.
             */
            list(const A& head, const list<A>& tail) : ocons(new cons<A>(head, tail.ocons)) {}
            /*!
             * construct a list from a C++11 initializer list.
             */
            list(std::initializer_list<A> il) {
                if (il.begin() != il.end()) {
                    auto it = il.end();
                    do {
                        --it;
                        ocons = boost::intrusive_ptr<cons<A>>(new cons<A>(*it, ocons));
                    }
                    while (it != il.begin());
                }
            }

            /*!
             * Check whether this list is non-empty.  If it returns true, then it's valid to
             * use head() and tail().
             */
            inline operator bool() const
            {
                return (bool)ocons;
            }
    
            /*!
             * Return the head of this list.  Caller must ensure that this list isn't
             * empty before calling, by casting to bool.
             */
            const A& head() const {return ocons->head;}
    
            /*!
             * Return the tail of this list.  Caller must ensure that this list isn't
             * empty before calling, by casting to bool.
             */
            list<A> tail() const {return list<A>(ocons->tail);}

            bool operator == (const list<A>& other) const {
                list<A> one = *this;
                list<A> two = other;
                while (one && two) {
                    if (!(one.head() == two.head())) return false;
                    one = one.tail();
                    two = two.tail();
                }
                return !one && !two;
            }
            
            bool operator != (const list<A>& other) const {
                return !(*this == other);
            }

            A operator [] (int ix) const {
                list<A> l = *this;
                while (l && ix > 0) {
                    l = l.tail();
                    ix--;
                }
                return l.head();
            };

            size_t size() const
            {
                size_t len = 0;
                list<A> xs = *this;
                while (xs) {len++; xs = xs.tail();}
                return len;
            }

            /*!
             * Map a function over the list, producing a new list of modified values.
             */
            template <class Fn>
            list<typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
                typedef typename std::result_of<Fn(A)>::type B;
                list<B> out;
                list<A> in = *this;
                while (in) {
                    out = f(in.head()) %= out;
                    in = in.tail();
                }
                return out.reverse();
            }

            list<A> filter(std::function<bool(const A&)> pred) const
            {
                list<A> xs(*this);
                list<A> ys;
                while (xs) {
                    if (pred(xs.head()))
                        ys = xs.head() %= ys;
                    xs = xs.tail();
                }
                return ys.reverse();
            }

            list<A> reverse() const
            {
                list<A> acc;
                list<A> as = *this;
                while (as) {
                    acc = as.head() %= acc;
                    as = as.tail();
                }
                return acc;
            }

            /*!
             * Map then concat.
             */
            template <class Fn>
            list<typename std::result_of<Fn(A)>::type::value_type> concatMap(const Fn& f) const {
                return concat(this->map(f));
            }

            /*!
             * Map A's to optional Bs, then take the defined values.
             */
            template <class Fn>
            list<typename std::result_of<Fn(A)>::type::value_type>
                    map_optional(const Fn& f) const {
                return cat_optional(this->map(f));
            }

            std::tuple<heist::list<A>, heist::list<A>> splitAt(int i) const {
                heist::list<A> xs = *this;
                heist::list<A> fst;
                while (i > 0 && (bool)xs) {
                    fst = heist::list<A>(xs.head(), fst);
                    xs = xs.tail();
                    i--;
                }
                return std::tuple<heist::list<A>, heist::list<A>>(fst.reverse(), xs);
            }

            list<A> intersperse(A x) const {
                const auto& xs = *this;
                if (xs && xs.tail())
                    return xs.head() %= x %= xs.tail().intersperse(x);
                else
                    return xs;
            }

            bool any(std::function<bool(const A&)> pred) const {
                auto xs = *this;
                while (xs) {
                    if (pred(xs.head())) return true;
                    xs = xs.tail();
                }
                return false;
            }

            std::list<A> toStdList() const {
                std::list<A> out;
                auto xs = *this;
                while (xs) {
                    out.push_back(xs.head());
                    xs = xs.tail();
                }
                return out;
            }

            bool contains(const A& a) const
            {
                auto xs = *this;
                while (xs) {
                    if (xs.head() == a)
                        return true;
                    xs = xs.tail();
                }
                return false;
            }

            template <class B>
            B foldl(std::function<B(const B&,const A&)> f, B b) const
            {
                auto xs = *this;
                while (xs) {
                    b = f(b, xs.head());
                    xs = xs.tail();
                }
                return b;
            }

            template <class B>
            B foldr(std::function<B(const A&,const B&)> f, B b) const
            {
                if (*this)
                    return f(this->head(), this->tail().foldr(f, b));
                else
                    return b;
            }

            /*!
             * Fold a non-empty set with no initial value.
             */
            A foldl1(std::function<A(const A&, const A&)> f) const
            {
                assert(*this);
                return this->tail().foldl(f, this->head());
            }

            /*!
             * Fold a non-empty set with no initial value.
             */
            A foldr1(std::function<A(const A&, const A&)> f) const
            {
                assert(*this);
                return this->tail().foldr(f, this->head());
            }

            /*!
             * Return the lists of items from xs that do and do not match the predicate,
             * respectively.
             */
            std::tuple<list<A>, list<A>> partition(std::function<bool(const A&)> pred) const
            {
                auto xs = *this;
                list<A> ins;
                list<A> outs;
                while (xs) {
                    if (pred(xs.head()))
                        ins = xs.head() %= ins;
                    else
                        outs = xs.head() %= outs;
                    xs = xs.tail();
                }
                return std::make_tuple(ins.reverse(), outs.reverse());
            }
    };

    /*!
     * Operator synonym for list constructor - right-associative.
     */
    template <class A>
    inline list<A> operator %= (const A& x, const list<A>& xs)
    {
        return list<A>(x, xs);
    }

    /*!
     * To do (when we have really long lists): Uses too much stack.
     */
    template <class A>
    list<A> operator + (const list<A>& one, const list<A>& tother)
    {
        if (one)
            return one.head() %= (one.tail() + tother);
        else
            return tother;
    }

    template <class A>
    std::ostream& operator << (std::ostream& os, const list<A>& xs0) {
        os << "[";
        list<A> xs(xs0);
        while (xs) {
            os << xs.head();
            xs = xs.tail();
            if (xs) os << ",";
        }
        os << "]";
        return os;
    }

    /*!
     * Concatenate the list of lists into a single list.
     */
    template <class A>
    list<A> concat(list<list<A>> lists) {
        return lists.template foldr<list<A>>([] (const list<A>& a, const list<A>& b) {
                return a + b;
            }, list<A>());
    }

    /*!
     * Filter the defined values and put them into the output list. 
     */
    template <class A>
    list<A> cat_optional(list<boost::optional<A>> xs) {
        return
            xs.filter([] (const boost::optional<A>& oa) { return (bool)oa; })
              .map([] (boost::optional<A> oa) { return oa.get(); });
    }

    template <class A, class B, class C>
    list<C> zip_with(std::function<C(const A&,const B&)> f, list<A> as, list<B> bs)
    {
        list<C> cs;
        while (as && bs) {
            cs = f(as.head(), bs.head()) %= cs;
            as = as.tail();
            bs = bs.tail();
        }
        return cs.reverse();
    }

    template <class A, class B>
    std::tuple<list<A>, list<B>> unzip(list<std::tuple<A, B>> tuples)
    {
        return std::make_tuple(
                tuples.map([] (const std::tuple<A,B>& t) {return std::get<0>(t);}),
                tuples.map([] (const std::tuple<A,B>& t) {return std::get<1>(t);})
            );
    }

    template <class A, class B, class C>
    std::tuple<list<A>, list<B>, list<C>> unzip3(list<std::tuple<A, B, C>> tuples)
    {
        return std::make_tuple(
                tuples.map([] (const std::tuple<A,B,C>& t) {return std::get<0>(t);}),
                tuples.map([] (const std::tuple<A,B,C>& t) {return std::get<1>(t);}),
                tuples.map([] (const std::tuple<A,B,C>& t) {return std::get<2>(t);})
            );
    }
}

#endif

