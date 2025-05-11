#include "config.h"

#include "test/rpc/test_command.h"

#include "rpc/command.h"

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
