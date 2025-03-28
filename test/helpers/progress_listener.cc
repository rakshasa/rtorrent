#include "config.h"

#include "progress_listener.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include "torrent/utils/log.h"
#include "torrent/utils/log_buffer.h"

static std::string
get_test_path(const test_list_type& tl) {
  if (tl.size() < 2)
    return "";

  return std::accumulate(std::next(tl.begin()), std::prev(tl.end()), std::string(), [](std::string result, CppUnit::Test* test) {
      return std::move(result) + test->getName() + "::";
    });
}

void
progress_listener::startTest(CppUnit::Test *test) {
  std::cout << get_test_path(m_test_path) << test->getName() << std::flush;

  torrent::log_cleanup();

  m_last_test_failed = false;
  m_current_log_buffer = torrent::log_open_log_buffer("test_output");
}

void
progress_listener::addFailure(const CppUnit::TestFailure &failure) {
  // AddFailure is called for parent test suits, so only deal with leafs.
  if (m_current_log_buffer == nullptr)
    return;

  std::cout << " : " << (failure.isError() ? "error" : "assertion") << std::flush;

  m_last_test_failed = true;
  m_failures.push_back(failure_type{ failure.failedTestName(), std::move(m_current_log_buffer) });
}

void
progress_listener::endTest(CppUnit::Test *test) {
  std::cout << (m_last_test_failed ? "" : " : OK") << std::endl;

  m_current_log_buffer.reset();
  torrent::log_cleanup();
}

void
progress_listener::startSuite(CppUnit::Test *suite) {
  m_test_path.push_back(suite);

  if (suite->countTestCases() > 0)
    std::cout << std::endl << get_test_path(m_test_path) << suite->getName() << ":" << std::endl;
}

void
progress_listener::endSuite(CppUnit::Test *suite) {
  m_test_path.pop_back();
}
