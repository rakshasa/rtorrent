#ifndef RTORRENT_ENGINE_CURL_STACK_H
#define RTORRENT_ENGINE_CURL_STACK_H

#include <list>

namespace engine {

class CurlStack {
 public:
  friend class CurlGet;

  typedef std::list<CurlGet*> CurlGetList;

  CurlStack();
  ~CurlStack();

  int         get_size() const { return m_size; }
  bool        is_busy() const  { return !m_getList.empty(); }

  void        perform();

  // TODO: Set fd_set's only once?
  void        fdset(fd_set* readfds, fd_set* writefds, fd_set* exceptfds, int* maxFd);

  static void global_init();
  static void global_cleanup();

 protected:
  void        add_get(CurlGet* get);
  void        remove_get(CurlGet* get);

 private:
  void*       m_handle;

  int         m_size;
  CurlGetList m_getList;
};

}

#endif
