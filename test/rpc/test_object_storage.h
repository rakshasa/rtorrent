#include "test/helpers/test_fixture.h"

#include "rpc/object_storage.h"

class TestObjectStorage : public test_fixture {
  CPPUNIT_TEST_SUITE(TestObjectStorage);

  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_conversions);
  CPPUNIT_TEST(test_validate_keys);
  CPPUNIT_TEST(test_access);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_basics();

  void test_conversions();
  void test_validate_keys();

  void test_access();

private:
  rpc::object_storage m_storage;
};
