// $Id$

#ifndef _FMAP_H_
#define _FMAP_H_

#include <boost/optional.hpp>

namespace heist {

    /*!
     * Map a function over an optional type.
     */
    template <class A, class B>
    boost::optional<B> fmap(std::function<B(const A&)> f, boost::optional<A> oa)
    {
        return oa ? boost::optional<B>(f(oa.get()))
                  : boost::optional<B>();
    }

    template <class A, class B>
    std::function<std::tuple<B, A>(std::tuple<A, B>)> swap()
    {
        return [] (std::tuple<A,B> ab) {
            return std::make_tuple(std::get<1>(ab), std::get<0>(ab));
        };
    }

    template <class A, class B>
    std::function<A(std::tuple<A, B>)> fst()
    {
        return [] (std::tuple<A,B> ab) {
            return std::get<0>(ab);
        };
    }

    template <class A, class B>
    std::function<B(std::tuple<A, B>)> snd()
    {
        return [] (std::tuple<A,B> ab) {
            return std::get<1>(ab);
        };
    }

};

#endif

