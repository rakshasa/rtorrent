#include "config.h"

#include <cstdlib>
#include <stdexcept>
#include <signal.h>
#include <string.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include "helpers/progress_listener.h"
#include "helpers/protectors.h"
#include "helpers/utils.h"

CPPUNIT_REGISTRY_ADD_TO_DEFAULT("rpc");
CPPUNIT_REGISTRY_ADD_TO_DEFAULT("src");

void
do_test_panic(int signum) {
  signal(signum, SIG_DFL);

  std::cout << std::endl << std::endl << "Caught " << strsignal(signum) << ", dumping stack:" << std::endl << std::endl;

#ifdef HAVE_BACKTRACE
  void* stackPtrs[20];

  // Print the stack and exit.
  int stackSize = backtrace(stackPtrs, 20);
  char** stackStrings = backtrace_symbols(stackPtrs, stackSize);

  for (int i = 0; i < stackSize; ++i)
    std::cout << stackStrings[i] << std::endl;

#else
  std::cout << "Stack dump not enabled." << std::endl;
#endif

  std::cout << std::endl;
  torrent::log_cleanup();
  std::abort();
}

void
register_signal_handlers() {
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = &do_test_panic;

  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    std::cout << "Could not register signal handlers." << std::endl;
    exit(-1);
  }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  register_signal_handlers();

  CppUnit::TestResult controller;
  CppUnit::TestResultCollector result;
  progress_listener progress;

  controller.addListener(&result);
  controller.addListener(&progress);

  controller.popProtector();
  controller.pushProtector(new ExceptionProtector());

  CppUnit::TextUi::TestRunner runner;
  add_tests(runner, std::getenv("TEST_NAME"));

  try {
    std::cout << "Running ";
    runner.run( controller );

    // TODO: Make outputter.
    dump_failures(progress.failures());

    // Print test in a compiler compatible format.
    CppUnit::CompilerOutputter outputter( &result, std::cerr );
    outputter.write();

  } catch ( std::invalid_argument &e ) { // Test path not resolved
    std::cerr  <<  std::endl <<  "ERROR: "  <<  e.what() << std::endl;
    return 1;
  }

  return result.wasSuccessful() ? 0 : 1;
}
