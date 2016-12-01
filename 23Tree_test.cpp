// $Id$

#include "23Tree_test.h"
#include "23Map.h"
#include "23MultiMap.h"
#include "23Set.h"
#include "Random.h"
#include <cppunit/TestAssert.h>
#include <map>
#include <set>

using namespace std;
using namespace boost;
using namespace heist;

#define TEST_SIZE 5000

static tuple<set<int>, Set<int>> construct()
{
    set<int> mset;
    Set<int> iset;
    for (int i = 0; i < TEST_SIZE; i++) {
        int value = randomRange(TEST_SIZE);
        mset.insert(value);
        iset = iset.insert(value);
    }
    return make_tuple(mset, iset);
}

static void compare(const set<int>& mset, Set<int> iset)
{
    set<int>::const_iterator mit = mset.begin();
    auto iit0 = iset.begin();
    while (iit0) {
        Set<int>::Iterator iit = iit0.get();
        CPPUNIT_ASSERT(mit != mset.end());
        CPPUNIT_ASSERT_EQUAL(*mit, iit.get());
        iit0 = iit.next();
        mit++;
    }
    CPPUNIT_ASSERT(mit == mset.end());
    
    // Test find
    for (int i = 0; i < TEST_SIZE; i++)
        CPPUNIT_ASSERT_EQUAL(mset.find(i) != mset.end(), (bool)iset.find(i)); 
}

void TwoThreeTree_test::testInsert1()
{
    auto sets = construct();
    auto mset = get<0>(sets);
    auto iset = get<1>(sets);
    compare(mset, iset);
}

void TwoThreeTree_test::testInsert2()
{
    auto sets = construct();
    // Compare in reverse order to test that our iterators can go backwards
    auto mset = get<0>(sets);
    auto iset = get<1>(sets);
    set<int>::iterator mit = mset.end();
    auto iit0 = iset.end();
    while (iit0) {
        Set<int>::Iterator iit = iit0.get();
        CPPUNIT_ASSERT(mit != mset.begin());
        --mit;
        CPPUNIT_ASSERT_EQUAL(*mit, iit.get());
        iit0 = iit.prev();
    }
    CPPUNIT_ASSERT(mit == mset.begin());
}

void TwoThreeTree_test::testLowerBound()
{
    auto sets = construct();
    auto mset = get<0>(sets);
    auto iset = get<1>(sets);
    for (int i = 1; i < (TEST_SIZE / 5); i++) {
        int x = randomRange(TEST_SIZE * 11 / 10) - (TEST_SIZE/20);
        auto iit0 = iset.lower_bound(x);
        auto mit = mset.lower_bound(x);
        CPPUNIT_ASSERT_EQUAL((bool) iit0, mit != mset.end());
        if (iit0) {
            auto iit = iit0.get();
            CPPUNIT_ASSERT_EQUAL(iit.get(), *mit);
        }
    }
}

void TwoThreeTree_test::testDelete1()
{
    auto sets = construct();
    auto mset = get<0>(sets);
    auto iset = get<1>(sets);
    for (int i = 0; i < (TEST_SIZE/2); i++) {
        int x = randomRange(TEST_SIZE);
        auto mit = mset.find(x);
        if (mit != mset.end())
            mset.erase(mit);
        if (auto iit = iset.find(x))
            iset = iit.get().remove();
    }
    compare(mset, iset);
}

void TwoThreeTree_test::testDelete2()
{
    set<int> mset;
    Set<int> iset;
    for (int i = 0; i < TEST_SIZE; i++) {
        int value = randomRange(TEST_SIZE);
        mset.insert(value);
        iset = iset.insert(value);
        int x = randomRange(TEST_SIZE);
        auto mit = mset.find(x);
        if (mit != mset.end())
            mset.erase(mit);
        if (auto iit = iset.find(x))
            iset = iit.get().remove();
    }
    compare(mset, iset);
}

static void compare(const map<int, int>& mset, Map<int, int> imap)
{
    map<int, int>::const_iterator mit = mset.begin();
    auto iit0 = imap.begin();
    while (iit0) {
        Map<int, int>::Iterator iit = iit0.get();
        CPPUNIT_ASSERT(mit != mset.end());
        CPPUNIT_ASSERT_EQUAL(mit->first, iit.getKey());
        CPPUNIT_ASSERT_EQUAL(mit->second, iit.getValue());
        iit0 = iit.next();
        mit++;
    }
    CPPUNIT_ASSERT(mit == mset.end());
}

void TwoThreeTree_test::testMap(int testNo)
{
    map<int, int> mmap;
    Map<int, int> imap;
    for (int i = 0; i < TEST_SIZE; i++) {
        int x = randomRange(TEST_SIZE);
        mmap.insert(pair<int, int>(x, x));
        imap = imap.insert(x, x);
        x = randomRange(TEST_SIZE);
        auto mit = mmap.find(x);
        bool mpresent = mit != mmap.end();
        auto iit = imap.find(x);
        bool ipresent = (bool)iit;
        CPPUNIT_ASSERT_EQUAL(mpresent, ipresent);
        if (testNo == 2 && mpresent) {
            mmap.erase(mit);
            imap = iit.get().remove();
        }
    }
    compare(mmap, imap);
}

static void compare(const multimap<int, int>& mset, MultiMap<int, int> imap)
{
    multimap<int, int>::const_iterator mit = mset.begin();
    auto iit0 = imap.begin();
    while (iit0) {
        MultiMap<int, int>::Iterator iit = iit0.get();
        CPPUNIT_ASSERT(mit != mset.end());
        CPPUNIT_ASSERT_EQUAL(mit->first, iit.getKey());
        CPPUNIT_ASSERT_EQUAL(mit->second, iit.getValue());
        iit0 = iit.next();
        mit++;
    }
    CPPUNIT_ASSERT(mit == mset.end());
}

void TwoThreeTree_test::testMultiMap(int testNo)
{
    multimap<int, int> mmap;
    MultiMap<int, int> imap;
    for (int i = 0; i < TEST_SIZE; i++) {
        int x = randomRange(TEST_SIZE);
        mmap.insert(pair<int, int>(x, x));
        imap = imap.insert(x, x);
        x = randomRange(TEST_SIZE);
        auto mit = mmap.lower_bound(x);
        auto mit2 = mit;
        int mn = 0;
        while (mit2 != mmap.end() && mit2->first == x) {
            mn++;
            mit2++;
        }
        int in = 0;
        auto iit = imap.lower_bound(x);
        auto iit2 = iit;
        while (iit2 && iit2.get().getKey() == x) {
            in++;
            iit2 = iit2.get().next();
        }
        CPPUNIT_ASSERT_EQUAL(mn, in);
        if (testNo == 2 && in > 0) {
            mmap.erase(mit);
            imap = iit.get().remove();
        }
    }
    compare(mmap, imap);
}

void TwoThreeTree_test::testEquality()
{
    {
        Set<int> one = Set<int>().insert(7).insert(10).insert(5);
        Set<int> two = Set<int>().insert(5).insert(10).insert(7);
        CPPUNIT_ASSERT(one == two);
    }

    {
        Set<int> one = Set<int>().insert(11);
        Set<int> two = Set<int>().insert(11).insert(9).insert(15);
        CPPUNIT_ASSERT(one != two);
    }
    
    {
        Set<int> one = Set<int>().insert(11);
        Set<int> two = Set<int>().insert(99);
        CPPUNIT_ASSERT(one != two);
    }
}

void TwoThreeTree_test::testToList()
{
    CPPUNIT_ASSERT(
        Set<int>().insert(100).insert(11).insert(12).insert(102).insert(55).toList() ==
        (11 %= 12 %= 55 %= 100 %= 102 %= heist::list<int>())
    );
}

