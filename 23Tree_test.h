// $Id$

#ifndef _23TREE_TEST_H_
#define _23TREE_TEST_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class TwoThreeTree_test : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TwoThreeTree_test);
    CPPUNIT_TEST(testInsert1);
    CPPUNIT_TEST(testInsert2);
    CPPUNIT_TEST(testLowerBound);
    CPPUNIT_TEST(testDelete1);
    CPPUNIT_TEST(testDelete2);
    CPPUNIT_TEST(testMap1);
    CPPUNIT_TEST(testMap2);
    CPPUNIT_TEST(testMultiMap1);
    CPPUNIT_TEST(testMultiMap2);
    CPPUNIT_TEST(testEquality);
    CPPUNIT_TEST(testToList);
    CPPUNIT_TEST_SUITE_END();

    public:
        TwoThreeTree_test() {}
        virtual ~TwoThreeTree_test() {}

        void setUp() {}
        void tearDown() {}

        void testInsert1();
        void testInsert2();
        void testLowerBound();
        void testDelete1();
        void testDelete2();
        void testMap(int testNo);
        void testMap1() {testMap(1);}
        void testMap2() {testMap(2);}
        void testMultiMap(int testNo);
        void testMultiMap1() {testMultiMap(1);}
        void testMultiMap2() {testMultiMap(2);}
        void testEquality();
        void testToList();
};

#endif

