#ifndef RTORRENT_UTILS_FUNCTIONAL_H
#define RTORRENT_UTILS_FUNCTIONAL_H

#include <functional>

namespace utils {

// Create a class that calls a std::function when returning or exiting a scope.
class scope_guard {
public:
  scope_guard(std::function<void()> on_exit_scope) : m_on_exit_scope(on_exit_scope) {}
  ~scope_guard() { m_on_exit_scope(); }

private:
  std::function<void()> m_on_exit_scope;
};

} // namespace utils

#endif

