// $Id$

#include "List_test.h"

#include <heist/list.h>
#include <algorithm>
#include <stdio.h>
#include <string>

using std::vector;
using heist::list;

template <class A>
static void check(list<A> l, vector<A> v)
{
    int len = 0;
    for (typename vector<A>::iterator it = v.begin(); it != v.end(); it++) {
        char msg[81];
        sprintf(msg, "list too short - %d", len);
        CPPUNIT_ASSERT_MESSAGE(msg, l);
        sprintf(msg, "mismatch at idx %d", len);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, *it, l.head());
        l = l.tail();
        len++;
    }
    CPPUNIT_ASSERT_MESSAGE("list too long", !l);
}

void List_test::test1()
{
    check(list<int>(55, list<int>(56, list<int>())), {55, 56});
}

void List_test::test2()
{
    check('a' %= 'b' %= 'c' %= list<char>(), {'a', 'b', 'c'});
}

void List_test::test3()
{
    check((10 %= 11 %= 12 %= list<int>()) + (20 %= 21 %= 22 %= list<int>()), {10,11,12,20,21,22}); 
}

void List_test::testEquality()
{
    CPPUNIT_ASSERT((1 %= 5 %= list<int>()) == (1 %= 5 %= list<int>()));
    CPPUNIT_ASSERT(list<int>() == list<int>());
    CPPUNIT_ASSERT((2 %= 5 %= list<int>()) != (1 %= 5 %= list<int>()));
    CPPUNIT_ASSERT((1 %= 5 %= list<int>()) != (1 %= 6 %= list<int>()));
    CPPUNIT_ASSERT((1 %= 5 %= list<int>()) != (1 %= list<int>()));
    CPPUNIT_ASSERT((1 %= list<int>()) != (1 %= 5 %= list<int>()));
    CPPUNIT_ASSERT(list<int>() != (9 %= list<int>()));
    CPPUNIT_ASSERT((9 %= list<int>()) != list<int>());
    CPPUNIT_ASSERT((9 %= list<int>()) == (9 %= list<int>()));
    CPPUNIT_ASSERT((9 %= list<int>()) != (8 %= list<int>()));
}

void List_test::testReverse()
{
    list<int> one = 10 %= 15 %= 8 %= 1 %= list<int>();
    list<int> two = 1 %= 8 %= 15 %= 10 %= list<int>();
    CPPUNIT_ASSERT_EQUAL(one, two.reverse());
}

void List_test::testMap()
{
    list<int> one = 1 %= 3 %= 2 %= list<int>();
    list<char> newOne = one.map([] (int c) {return (char) (c + 64);});
    list<char> sb = 'A' %= 'C' %= 'B' %= list<char>();
    CPPUNIT_ASSERT(newOne == sb);
}

