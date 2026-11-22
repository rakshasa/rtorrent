#include "test/helpers/test_fixture.h"

class TestSchedulerSimple : public test_fixture {
  CPPUNIT_TEST_SUITE(TestSchedulerSimple);

  CPPUNIT_TEST(test_update_refuses_when_resumed_erases_current);
  CPPUNIT_TEST(test_update_refuses_when_resumed_reorders_started);
  CPPUNIT_TEST(test_removed_refuses_when_resumed_hides_other);
  CPPUNIT_TEST(test_full_limit_ignores_started_changes);
  CPPUNIT_TEST(test_hidden_entries_do_not_refuse);
  CPPUNIT_TEST(test_update_skips_empty_slot);
  CPPUNIT_TEST(test_removed_skips_empty_slot);
  CPPUNIT_TEST(test_pause_refuses_when_handler_restores_active);
  CPPUNIT_TEST(test_unlimited_and_zero_limits);
  CPPUNIT_TEST(test_refusal_from_started_event_is_logged);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_update_refuses_when_resumed_erases_current();
  void test_update_refuses_when_resumed_reorders_started();
  void test_removed_refuses_when_resumed_hides_other();
  void test_full_limit_ignores_started_changes();
  void test_hidden_entries_do_not_refuse();
  void test_update_skips_empty_slot();
  void test_removed_skips_empty_slot();
  void test_pause_refuses_when_handler_restores_active();
  void test_unlimited_and_zero_limits();
  void test_refusal_from_started_event_is_logged();
};
