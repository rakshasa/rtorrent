#include "config.h"

#include "test/rpc/test_command.h"

#include <torrent/exceptions.h>

#include "rpc/command.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommand);

bool
command_stack_all_empty() {
  return std::all_of(rpc::command_base::stack_begin(), rpc::command_base::stack_end(),
                     [](auto& obj) { return obj.is_empty(); });
}

void
TestCommand::test_stack() {
  torrent::Object::list_type args;
  rpc::command_base::stack_type stack;
  torrent::Object* last_stack;

  // Test empty stack.
  CPPUNIT_ASSERT(command_stack_all_empty());

  last_stack = rpc::command_base::push_stack(args, &stack);
  CPPUNIT_ASSERT(command_stack_all_empty());

  rpc::command_base::pop_stack(&stack, last_stack);
  CPPUNIT_ASSERT(command_stack_all_empty());

  // Test stack with one.
  args.push_back(int64_t(1));

  last_stack = rpc::command_base::push_stack(args, &stack);
  CPPUNIT_ASSERT(!command_stack_all_empty());
  CPPUNIT_ASSERT(rpc::command_base::stack_begin()->as_value() == 1);

  rpc::command_base::pop_stack(&stack, last_stack);
  CPPUNIT_ASSERT(command_stack_all_empty());

  // Test stack with two
  args.clear();
  args.push_back(int64_t(2));
  args.push_back(int64_t(3));

  last_stack = rpc::command_base::push_stack(args, &stack);
  CPPUNIT_ASSERT(!command_stack_all_empty());
  CPPUNIT_ASSERT(rpc::command_base::current_stack[0].as_value() == 2);
  CPPUNIT_ASSERT(rpc::command_base::current_stack[1].as_value() == 3);

  rpc::command_base::pop_stack(&stack, last_stack);
  CPPUNIT_ASSERT(command_stack_all_empty());
}

void
TestCommand::test_stack_double() {
  torrent::Object::list_type args;
  rpc::command_base::stack_type stack_first;
  rpc::command_base::stack_type stack_second;
  torrent::Object* last_stack_first;
  torrent::Object* last_stack_second;

  // Test double-stacked.
  args.push_back(int64_t(1));

  last_stack_first = rpc::command_base::push_stack(args, &stack_first);
  CPPUNIT_ASSERT(!command_stack_all_empty());
  CPPUNIT_ASSERT(rpc::command_base::current_stack[0].as_value() == 1);

  args.clear();
  args.push_back(int64_t(2));
  args.push_back(int64_t(3));

  last_stack_second = rpc::command_base::push_stack(args, &stack_second);
  CPPUNIT_ASSERT(!command_stack_all_empty());

  CPPUNIT_ASSERT(rpc::command_base::current_stack[0].as_value() == 2);
  CPPUNIT_ASSERT(rpc::command_base::current_stack[1].as_value() == 3);

  rpc::command_base::pop_stack(&stack_second, last_stack_second);
  CPPUNIT_ASSERT(!command_stack_all_empty());
  CPPUNIT_ASSERT(rpc::command_base::current_stack[0].as_value() == 1);

  rpc::command_base::pop_stack(&stack_first, last_stack_first);
  CPPUNIT_ASSERT(command_stack_all_empty());
}

void
TestCommand::test_preparsed_commands() {
  unsigned int prepare_count = 0;
  rpc::preparsed_commands commands([&prepare_count](auto& prepared) {
    ++prepare_count;
    // Reentrant access must not invoke the same callback recursively.
    prepared.prepare_if_needed();
    prepared.push_back(rpc::parse_command_object("string.length=abc"));
  });

  CPPUNIT_ASSERT_EQUAL(0u, prepare_count);
  CPPUNIT_ASSERT(commands.empty());

  commands.prepare_if_needed();
  CPPUNIT_ASSERT_EQUAL(1u, prepare_count);

  size_t count = 0;
  for (auto& itr : commands) {
    CPPUNIT_ASSERT(itr.is_dict_key());
    ++count;
  }

  CPPUNIT_ASSERT_EQUAL(size_t(1), count);
  CPPUNIT_ASSERT_EQUAL(1u, prepare_count);

  for (auto& itr : commands)
    CPPUNIT_ASSERT(itr.is_dict_key());

  CPPUNIT_ASSERT_EQUAL(1u, prepare_count);
}

void
TestCommand::test_parse_command_object() {
  auto command = rpc::parse_command_object("\tstring.length=abc  ");
  CPPUNIT_ASSERT(command.is_dict_key());
  CPPUNIT_ASSERT_EQUAL(std::string("string.length"), command.as_dict_key());

  // This helper has no way to return the next-command pointer, so it must not
  // silently accept a multipart command separated by ';'.
  CPPUNIT_ASSERT_THROW(rpc::parse_command_object("string.length=abc;string.length=def"), torrent::input_error);

  // Unlike parse_command (which parses command files), this helper handles one
  // multicall command and must reject newline boundaries, including CRLF.
  CPPUNIT_ASSERT_THROW(rpc::parse_command_object("string.length=abc\nstring.length=def"), torrent::input_error);
  CPPUNIT_ASSERT_THROW(rpc::parse_command_object("string.length=abc\r\nstring.length=def"), torrent::input_error);
}
