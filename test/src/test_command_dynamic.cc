#include "config.h"

#include "test/src/test_command_dynamic.h"

#include "helpers/assert.h"

#include "control.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandDynamic);

void initialize_command_dynamic();
void initialize_command_ui();

void
TestCommandDynamic::setUp() {
  m_test_main_thread = TestMainThread::create();
  m_test_main_thread->init_thread();

  if (rpc::commands.find("method.insert") == rpc::commands.end()) {
    setlocale(LC_ALL, "");
    // cachedTime = rak::timer::current();
    control = new Control;

    initialize_command_dynamic();
    initialize_command_ui();
  }
}

void
TestCommandDynamic::tearDown() {
  m_test_main_thread.reset();
}

void
TestCommandDynamic::test_basics() {
  rpc::commands.call_command("method.insert.value", rpc::create_object_list("test_basics.1", int64_t(1)));
  CPPUNIT_ASSERT(rpc::commands.call_command("test_basics.1", torrent::Object()).as_value() == 1);
}

void
TestCommandDynamic::test_get_set() {
  rpc::commands.call_command("method.insert.simple", rpc::create_object_list("test_get_set.1", "cat=1"));
  CPPUNIT_ASSERT(rpc::commands.call_command("test_get_set.1", torrent::Object()).as_string() == "1");
  CPPUNIT_ASSERT(rpc::commands.call_command("method.get", "test_get_set.1").as_string() == "cat=1");

  rpc::commands.call_command("method.set", rpc::create_object_list("test_get_set.1", "cat=2"));
  CPPUNIT_ASSERT(rpc::commands.call_command("method.get", "test_get_set.1").as_string() == "cat=2");
}

void
TestCommandDynamic::test_old_style() {
  rpc::commands.call_command("method.insert", rpc::create_object_list("test_old_style.1", "value", int64_t(1)));
  CPPUNIT_ASSERT(rpc::commands.call_command("test_old_style.1", torrent::Object()).as_value() == 1);

  rpc::commands.call_command("method.insert", rpc::create_object_list("test_old_style.2", "bool", int64_t(5)));
  CPPUNIT_ASSERT(rpc::commands.call_command("test_old_style.2", torrent::Object()).as_value() == 1);

  rpc::commands.call_command("method.insert", rpc::create_object_list("test_old_style.3", "string", "test.2"));
  CPPUNIT_ASSERT(rpc::commands.call_command("test_old_style.3", torrent::Object()).as_string() == "test.2");

  rpc::commands.call_command("method.insert", rpc::create_object_list("test_old_style.4", "simple", "cat=test.3"));
  CPPUNIT_ASSERT(rpc::commands.call_command("test_old_style.4", torrent::Object()).as_string() == "test.3");
}

void
TestCommandDynamic::test_insert_list() {
  torrent::Object key_only = torrent::Object::create_list();
  key_only.as_list().push_back("test_insert_list.1");

  rpc::commands.call_command("method.insert.list", key_only);

  torrent::Object result = rpc::commands.call_command("test_insert_list.1", torrent::Object());

  CPPUNIT_ASSERT(result.is_list());
  CPPUNIT_ASSERT(result.as_list().empty());

  rpc::commands.call_command("method.insert.list",
                             rpc::create_object_list("test_insert_list.2", rpc::create_object_list("a", "b")));

  torrent::Object filled = rpc::commands.call_command("test_insert_list.2", torrent::Object());

  CPPUNIT_ASSERT(filled.is_list());
  CPPUNIT_ASSERT_EQUAL((size_t)2, filled.as_list().size());
}

void
TestCommandDynamic::test_value_base() {
  auto value = [](std::initializer_list<torrent::Object> objects) {
    auto args = torrent::Object::create_list();

    for (const auto& object : objects)
      args.as_list().push_back(object);

    return rpc::commands.call_command("value", args).as_value();
  };

  CPPUNIT_ASSERT_EQUAL(int64_t(10), value({"10"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(255), value({"ff", int64_t(16)}));

  // strtoll only defines base 0 and base 2 through 36.
  ASSERT_CATCH_INPUT_ERROR( { value({"10", int64_t(1)}); } );
  ASSERT_CATCH_INPUT_ERROR( { value({"10", int64_t(37)}); } );
  ASSERT_CATCH_INPUT_ERROR( { value({"10", int64_t(-1)}); } );

  // An out-of-range base must not be narrowed into a valid one.
  ASSERT_CATCH_INPUT_ERROR( { value({"10", int64_t(1) << 40}); } );
  ASSERT_CATCH_INPUT_ERROR( { value({"ff", (int64_t(1) << 32) + 16}); } );

  // A number too large for the result must be rejected, not clamped.
  ASSERT_CATCH_INPUT_ERROR( { value({"99999999999999999999999"}); } );
}
