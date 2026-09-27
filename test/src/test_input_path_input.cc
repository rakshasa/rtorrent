#include "config.h"

#include "test/src/test_input_path_input.h"

#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include "input/path_input.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestInputPathInput);

void
TestInputPathInput::setUp() {
  test_fixture::setUp();

  char temp_dir[] = "/tmp/rtorrent_test_path_input_XXXXXX";

  CPPUNIT_ASSERT(mkdtemp(temp_dir) != nullptr);

  m_temp_dir = temp_dir;

  CPPUNIT_ASSERT_EQUAL(0, mkdir((m_temp_dir + "/alpha").c_str(), 0755));
  CPPUNIT_ASSERT_EQUAL(0, mkdir((m_temp_dir + "/alpha/beta").c_str(), 0755));
}

void
TestInputPathInput::tearDown() {
  rmdir((m_temp_dir + "/alpha/beta").c_str());
  rmdir((m_temp_dir + "/alpha").c_str());
  rmdir(m_temp_dir.c_str());

  test_fixture::tearDown();
}

// Tab at the end of the line completes the trailing component.
void
TestInputPathInput::test_complete_at_end() {
  input::PathInput input;

  input.str() = m_temp_dir + "/al";
  input.set_pos(input.str().size());

  CPPUNIT_ASSERT(input.pressed('\t'));

  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/alpha/", input.str());
  CPPUNIT_ASSERT_EQUAL(input.str().size(), input.get_pos());
}

// The same completion with the cursor left inside the line. Only the text up to
// the cursor selects the entry, and the rest of the line is discarded.
void
TestInputPathInput::test_complete_mid_line() {
  input::PathInput input;

  input.str() = m_temp_dir + "/alZZZ";
  input.set_pos(m_temp_dir.size() + 3);

  CPPUNIT_ASSERT(input.pressed('\t'));

  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/alpha/", input.str());
  CPPUNIT_ASSERT_EQUAL(input.str().size(), input.get_pos());
}

// A cursor resting on a separator keeps completing the component after it.
void
TestInputPathInput::test_cursor_on_separator() {
  input::PathInput input;

  input.str() = m_temp_dir + "/alpha/be";
  input.set_pos(m_temp_dir.size() + 6);

  CPPUNIT_ASSERT(input.pressed('\t'));

  CPPUNIT_ASSERT_EQUAL(m_temp_dir + "/alpha/beta/", input.str());
  CPPUNIT_ASSERT_EQUAL(input.str().size(), input.get_pos());
}
