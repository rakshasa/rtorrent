#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_TRANSFER_LIST_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_TRANSFER_LIST_H

#include <list>
#include <torrent/data/block_transfer.h>

#include "window.h"

namespace core {
  class Download;
}

namespace display {

class WindowDownloadTransferList : public Window {
public:
  struct assigned_type {
    assigned_type(torrent::BlockTransfer::key_type key, uint32_t last, char id) : m_key(key), m_last(last), m_id(id) { }

    torrent::BlockTransfer::key_type m_key;
    uint32_t m_last;
    char m_id;
  };

  typedef std::vector<assigned_type> assigned_vector;

  WindowDownloadTransferList(core::Download* d, unsigned int* focus);

  virtual void        redraw();

  unsigned int        rows() const;
  unsigned int        max_focus() const        { return std::max<int>(rows() - height() + 1, 0); }

private:
  char                key_id(torrent::BlockTransfer::key_type key);

  core::Download*     m_download;
  assigned_vector     m_assigned;
};

}

#endif
