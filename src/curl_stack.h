#ifndef LIBTORRENT_CURL_STACK_H
#define LIBTORRENT_CURL_STACK_H

#include <list>

class CurlStack {
  friend class CurlGet;

 public:
  typedef std::list<CurlGet*> CurlGetList;

  CurlStack();
  ~CurlStack();

  int  get_size() const    { return m_size; }
  bool is_busy() const { return !m_getList.empty(); }

  void perform();

  void fdset(fd_set* readfds, fd_set* writefds, fd_set* exceptfds, int& maxFd);

  static void global_init();
  static void global_cleanup();

 protected:
  void add_get(CurlGet* get);
  void remove_get(CurlGet* get);

 private:
  void* m_handle;

  int m_size;
  CurlGetList m_getList;
};

#endif

