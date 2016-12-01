// $Id$

#include "LRUCache_test.h"

#include "LRUCache.h"

using std::string;
using std::tuple;
using std::make_tuple;
using heist::list;

void LRUCache_test::test1()
{
    {
        LRUCache<int, char> c = LRUCache<int, char>(10)
            .insert(10,'a')
            .insert(5,'b')
            .insert(7,'c')
            .insert(8,'d');
        CPPUNIT_ASSERT(c.isConsistent());
        CPPUNIT_ASSERT(
            c.toList() == (
                make_tuple(5, 'b') %=
                make_tuple(7, 'c') %=
                make_tuple(8, 'd') %=
                make_tuple(10, 'a') %= list<tuple<int, char>>()
            )
        );
    }
    {
        LRUCache<int, string> c = LRUCache<int, string>(4)
            .insert(10,"a")
            .insert(5,"b")
            .insert(7,"c")
            .insert(8,"d")
            .insert(12,"e");
        CPPUNIT_ASSERT(c.isConsistent());
        CPPUNIT_ASSERT(
            c.toList() == (
                tuple<int, string>(5, "b") %=
                tuple<int, string>(7, "c") %=
                tuple<int, string>(8, "d") %=
                tuple<int, string>(12, "e") %= list<tuple<int, string>>()
            )
        );
    }
    {
        LRUCache<int, string> c = LRUCache<int, string>(4)
            .insert(10,"a")
            .insert(5,"b")
            .insert(7,"c")
            .insert(8,"d")
            .insert(12,"e")
            .insert(1,"f");
        CPPUNIT_ASSERT(c.isConsistent());
        CPPUNIT_ASSERT(
            c.toList() == (
                tuple<int, string>(1, "f") %=
                tuple<int, string>(7, "c") %=
                tuple<int, string>(8, "d") %=
                tuple<int, string>(12, "e") %= list<tuple<int, string>>()
            )
        );
    }
    {
        LRUCache<int, string> c = LRUCache<int, string>(4)
            .insert(10,"a")
            .insert(5,"b")
            .insert(7,"c")
            .insert(8,"d")
            .touch(10)
            .insert(12,"e")
            .insert(1,"f");
        CPPUNIT_ASSERT(c.isConsistent());
        //cout << c.toList().fmap<string>([] (tuple<int, string> p) {return get<1>(p);}) << endl;
        CPPUNIT_ASSERT(
            c.toList() == (
                tuple<int, string>(1, "f") %=
                tuple<int, string>(8, "d") %=
                tuple<int, string>(10, "a") %=
                tuple<int, string>(12, "e") %= list<tuple<int, string>>()
            )
        );
    }
    {
        LRUCache<int, string> c = LRUCache<int, string>(4)
            .insert(10,"a")
            .insert(5,"b")
            .insert(7,"c")
            .insert(8,"d")
            .insert(12,"e")
            .touch(10)
            .insert(1,"f");
        CPPUNIT_ASSERT(c.isConsistent());
        //cout << c.toList().fmap<string>([] (tuple<int, string> p) {return get<1>(p);}) << endl;
        CPPUNIT_ASSERT(
            c.toList() == (
                tuple<int, string>(1, "f") %=
                tuple<int, string>(7, "c") %=
                tuple<int, string>(8, "d") %=
                tuple<int, string>(12, "e") %= list<tuple<int, string>>()
            )
        );
    }
    {
        LRUCache<int, string> c = LRUCache<int, string>(4)
            .insert(10,"a")
            .insert(5,"b")
            .insert(7,"c")
            .insert(8,"d")
            .remove(5)
            .insert(12,"e")
            .touch(10)
            .insert(1,"f");
        CPPUNIT_ASSERT(c.isConsistent());
        //cout << c.toList().fmap<string>([] (tuple<int, string> p) {return get<1>(p);}) << endl;
        CPPUNIT_ASSERT(
            c.toList() == (
                tuple<int, string>(1, "f") %=
                tuple<int, string>(8, "d") %=
                tuple<int, string>(10, "a") %=
                tuple<int, string>(12, "e") %= list<tuple<int, string>>()
            )
        );
    }
}

