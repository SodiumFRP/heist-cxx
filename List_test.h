// $Id$

#ifndef _LIST_TEST_H_
#define _LIST_TEST_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class List_test : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(List_test);
    CPPUNIT_TEST(test1);
    CPPUNIT_TEST(test2);
    CPPUNIT_TEST(test3);
    CPPUNIT_TEST(testEquality);
    CPPUNIT_TEST(testReverse);
    CPPUNIT_TEST(testMap);
    CPPUNIT_TEST_SUITE_END();

    public:
        List_test() {}
        virtual ~List_test() {}

        void setUp() {}
        void tearDown() {}

        void test1();
        void test2();
        void test3();
        void testEquality();
        void testReverse();
        void testMap();
};

#endif

