// $Id$

#ifndef _LRUCACHE_TEST_H_
#define _LRUCACHE_TEST_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class LRUCache_test : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(LRUCache_test);
    CPPUNIT_TEST(test1);
    CPPUNIT_TEST_SUITE_END();

    public:
        LRUCache_test() {}
        virtual ~LRUCache_test() {}

        void setUp() {}
        void tearDown() {}

        void test1();
};

#endif

