#include "config.h"

#include "download.h"
#include "session_manager.h"

namespace core {

void
SessionManager::activate(const std::string& path) {
  m_path = path;

  if (m_path.empty())
    throw std::logic_error("core::SessionManager::activate(...) received an empty path");

  if (m_path.rbegin() != '/')
    m_path += '/';
}

void
SessionManager::disable() {
  m_path = "";
}

void
SessionManager::save(Download* d) {
  std::fstream f(

  
}

void
SessionManager::remove(Download* d) {
}

}
