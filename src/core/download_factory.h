// The DownloadFactory class assures that loading torrents can be done
// anywhere in the code by queueing the task. The user may change
// settings while, or even after, the torrent is loading.

#ifndef RTORRENT_CORE_DOWNLOAD_FACTORY_H
#define RTORRENT_CORE_DOWNLOAD_FACTORY_H

#include <functional>
#include <iosfwd>

#include <torrent/object.h>
#include <torrent/utils/scheduler.h>

#include "http_queue.h"

namespace core {

class Download;
class Manager;

class DownloadFactory {
public:
  typedef std::function<void ()> slot_void;
  typedef std::vector<std::string> command_list_type;

  // Do not destroy this object while it is in a HttpQueue.
  DownloadFactory(Manager* m);
  ~DownloadFactory();

  // Calling of receive_load() is delayed so you can change whatever
  // you want without fear of the slots being triggered as you call
  // load() or commit().
  void                load(const std::string& uri);
  void                load_raw_data(const std::string& input);
  void                commit();

  command_list_type&         commands()     { return m_commands; }
  torrent::Object::map_type& variables()    { return m_variables; }

  bool                get_session() const   { return m_session; }
  void                set_session(bool v)   { m_session = v; }

  bool                get_start() const     { return m_start; }
  void                set_start(bool v)     { m_start = v; }

  bool                get_init_load() const { return m_initLoad; }
  void                set_init_load(bool v) { m_initLoad = v; }

  bool                print_log() const     { return m_printLog; }
  void                set_print_log(bool v) { m_printLog = v; }

  void                slot_finished(slot_void s) { m_slot_finished = s; }

private:
  void                receive_load();
  void                receive_loaded();
  void                receive_commit();
  void                receive_success();
  void                receive_failed(const std::string& msg);

  void                log_created(Download* download, torrent::Object* rtorrent);

  void                initialize_rtorrent(Download* download, torrent::Object* rtorrent);

  Manager*                       m_manager;
  std::shared_ptr<std::iostream> m_stream;
  torrent::Object*               m_object{};

  bool                m_commited{};
  bool                m_loaded{};

  std::string         m_uri;
  bool                m_session{};
  bool                m_start{};
  bool                m_printLog{true};
  bool                m_isFile{};
  bool                m_initLoad{};

  command_list_type         m_commands;
  torrent::Object::map_type m_variables;

  slot_void                      m_slot_finished;
  torrent::utils::SchedulerEntry m_task_load;
  torrent::utils::SchedulerEntry m_task_commit;
};

bool is_network_uri(const std::string& uri);
bool is_magnet_uri(const std::string& uri);

}

#endif
