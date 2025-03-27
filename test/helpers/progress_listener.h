#include <memory>
#include <vector>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestListener.h>

#include "torrent/utils/log_buffer.h"

struct failure_type {
  std::string name;
  torrent::log_buffer_ptr log;
};

typedef std::unique_ptr<CppUnit::TestFailure> test_failure_ptr;
typedef std::vector<CppUnit::Test*> test_list_type;
typedef std::vector<failure_type> failure_list_type;

class progress_listener : public CppUnit::TestListener {
public:
  progress_listener() : m_last_test_failed(false) {}

  void startTest(CppUnit::Test *test) override;
  void addFailure(const CppUnit::TestFailure &failure) override;
  void endTest(CppUnit::Test *test) override;

  void startSuite(CppUnit::Test *suite) override;
  void endSuite(CppUnit::Test *suite) override;

  //Called by a TestRunner before running the test.
  // void startTestRun(CppUnit::Test *test, CppUnit::TestResult *event_manager) override;

  // Called by a TestRunner after running the test.
  // void endTestRun(CppUnit::Test *test, CppUnit::TestResult *event_manager) override;

  const failure_list_type& failures() { return m_failures; }
  failure_list_type&& move_failures() { return std::move(m_failures); }

private:
  progress_listener(const progress_listener& rhs) = delete;
  void operator =(const progress_listener& rhs) = delete;

  test_list_type    m_test_path;
  failure_list_type m_failures;
  bool              m_last_test_failed;

  torrent::log_buffer_ptr m_current_log_buffer;
};
