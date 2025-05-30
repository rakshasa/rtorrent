#ifndef RTORRENT_CORE_DOWNLOAD_LIST_H
#define RTORRENT_CORE_DOWNLOAD_LIST_H

#include <iosfwd>
#include <list>
#include <string>

namespace torrent {
  class HashString;
  class Object;
}

namespace core {

class Download;

// Container for all downloads. Add slots to the slot maps to cause
// some action to be taken when the torrent changes states. Don't
// change the states from outside of core.
//
// Fix apply_on_ratio if the base_type is changed.

class DownloadList : private std::list<Download*> {
public:
  typedef std::list<Download*>               base_type;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;
  using base_type::value_type;
  using base_type::pointer;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  DownloadList() = default;

  void                clear();

  void                session_save();

  iterator            find(const torrent::HashString& hash);

  iterator            find_hex(const char* hash);
  Download*           find_hex_ptr(const char* hash);

  // Might move this to DownloadFactory.
  Download*           create(std::istream* str, uint32_t tracker_key, bool printLog);
  Download*           create(torrent::Object* obj, uint32_t tracker_key, bool printLog);

  iterator            insert(Download* d);

  void                erase_ptr(Download* d);
  iterator            erase(iterator itr);

  //void                save(Download* d);

  bool                open(Download* d);
  void                open_throw(Download* d);

  void                close(Download* d);
  void                close_directly(Download* d);
  void                close_quick(Download* d);
  void                close_throw(Download* d);

  void                resume(Download* d, int flags = 0);
  void                pause(Download* d, int flags = 0);

  void                resume_default(Download* d) { resume(d); }
  void                pause_default(Download* d) { pause(d); }

  void                check_hash(Download* d);

  enum {
    D_SLOTS_INSERT,
    D_SLOTS_ERASE,
    D_SLOTS_OPEN,
    D_SLOTS_CLOSE,
    D_SLOTS_START,
    D_SLOTS_STOP,
    D_SLOTS_HASH_QUEUED,
    D_SLOTS_HASH_REMOVED,
    D_SLOTS_HASH_DONE,
    D_SLOTS_FINISHED,

    SLOTS_MAX_SIZE
  };

  static const char* slot_name(int m) {
    switch(m) {
    case D_SLOTS_INSERT: return "event.download.inserted";
    case D_SLOTS_ERASE: return "event.download.erased";
    case D_SLOTS_OPEN: return "event.download.opened";
    case D_SLOTS_CLOSE: return "event.download.closed";
    case D_SLOTS_START: return "event.download.resumed";
    case D_SLOTS_STOP: return "event.download.paused";
    case D_SLOTS_HASH_QUEUED: return "event.download.hash_queued";
    case D_SLOTS_HASH_REMOVED: return "event.download.hash_removed";
    case D_SLOTS_HASH_DONE: return "event.download.hash_done";
    case D_SLOTS_FINISHED: return "event.download.finished";
    default: return "BORK";
    }
  }

  // The finished slots will be called when an active download with
  // "finished" == 0 performs a hash check which returns a done
  // torrent.
  //
  // But how to avoid sending 'completed' messages to the tracker?
  // Also we need to handle cases when a hashing torrent starts up
  // after a shutdown.

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void                hash_done(Download* d);
  void                hash_queue(Download* d, int type);

  inline void         check_contains(Download* d);

  void                received_finished(Download* d);
  void                confirm_finished(Download* d);

  void                process_meta_download(Download* d);
};

}

#endif
