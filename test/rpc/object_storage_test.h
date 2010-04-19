#include <cppunit/extensions/HelperMacros.h>

#include "rpc/object_storage.h"

class ObjectStorageTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ObjectStorageTest);
  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_conversions);
  CPPUNIT_TEST(test_validate_keys);
  CPPUNIT_TEST(test_access);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() { }
  void tearDown() {}

  void test_basics();

  void test_conversions();
  void test_validate_keys();

  void test_access();

private:
  rpc::object_storage m_storage;
};
