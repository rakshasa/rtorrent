#ifndef RTORRENT_CORE_SESSION_MANAGER_H
#define RTORRENT_CORE_SESSION_MANAGER_H

#include <string>

namespace core {

class Download;

class SessionManager {
public:
  
  void activate(const std::string& path);
  void disable();

  bool is_active() { return !m_path.empty(); }

  void save(Download* d);
  void remove(Download* d);

private:
  std::string m_path;
};

}

#endif
