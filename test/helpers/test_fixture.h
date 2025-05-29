#ifndef LIBTORRENT_HELPER_TEST_FIXTURE_H
#define LIBTORRENT_HELPER_TEST_FIXTURE_H

#include <cppunit/TestFixture.h>

#include "test/helpers/mock_function.h"

class test_fixture : public CppUnit::TestFixture {
public:
  void setUp();
  void tearDown();
};

#endif
