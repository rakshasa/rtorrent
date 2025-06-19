#ifndef LIBTORRENT_HELPER_UTILS_H
#define LIBTORRENT_HELPER_UTILS_H

#include <algorithm>
#include <iostream>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <torrent/utils/log.h>

static void
dump_failures(const failure_list_type& failures) {
  if (failures.empty())
    return;

  std::cout << std::endl
            << "=================" << std::endl
            << "Failed Test Logs:" << std::endl
            << "=================" << std::endl;

  for (const auto& failure : failures) {
    if (failure.log->empty())
      return;

    std::cout << '\n'
              << failure.name << '\n';

    // Doesn't print dump messages as log_buffer drops them.
    for (const auto& entry : *failure.log)
      std::cout << entry.timestamp << ' ' << entry.message << '\n';

    std::cout << std::flush;
  }
  std::cout << std::endl;
}

static
void add_tests(CppUnit::TextUi::TestRunner& runner, const char* c_test_names) {
  if (c_test_names == NULL || std::string(c_test_names).empty()) {
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    return;
  }

  const std::string& test_names(c_test_names);

  size_t pos = 0;
  size_t next = 0;

  while ((next = test_names.find(',', pos)) < test_names.size()) {
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry(test_names.substr(pos, next - pos)).makeTest());
    pos = next + 1;
  }

  runner.addTest(CppUnit::TestFactoryRegistry::getRegistry(test_names.substr(pos)).makeTest());
}

#endif
