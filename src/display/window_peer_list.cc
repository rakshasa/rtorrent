#include "config.h"

#include <stdexcept>
#include <torrent/rate.h>
#include <torrent/data/block_transfer.h>
#include <torrent/data/piece.h>
#include <torrent/net/socket_address.h>
#include <torrent/peer/client_list.h>
#include <torrent/peer/peer_info.h>

#include "core/download.h"
#include "rak/algorithm.h"

#include "canvas.h"
#include "utils.h"
#include "window_peer_list.h"

namespace display {

WindowPeerList::WindowPeerList(core::Download* d, PList* l, PList::iterator* f) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_full),
  m_download(d),
  m_list(l),
  m_focus(f) {
}

void
WindowPeerList::redraw() {
  schedule_update();
  m_canvas->erase();

  int x = 2;
  int y = 0;

  m_canvas->print(x, y, "IP");      x += 25;
  m_canvas->print(x, y, "UP");      x += 7;
  m_canvas->print(x, y, "DOWN");    x += 7;
  m_canvas->print(x, y, "PEER");    x += 7;
  m_canvas->print(x, y, "CT/RE/LO"); x += 10;
  m_canvas->print(x, y, "QS");      x += 6;
  m_canvas->print(x, y, "DONE");    x += 6;
  m_canvas->print(x, y, "REQ");     x += 6;
  m_canvas->print(x, y, "SNUB");    x += 6;
  m_canvas->print(x, y, "FAILED");

  ++y;

  if (m_list->empty())
    return;

  typedef std::pair<PList::iterator, PList::iterator> Range;

  Range range = rak::advance_bidirectional(m_list->begin(),
                                           *m_focus != m_list->end() ? *m_focus : m_list->begin(),
                                           m_list->end(),
                                           m_canvas->height() - y);

  if (m_download->download()->file_list()->size_chunks() <= 0)
    throw std::logic_error("WindowPeerList::redraw() m_slotChunksTotal() returned invalid value");

  while (range.first != range.second) {
    torrent::Peer* p = *range.first;

    x = 0;

    auto ip_address = torrent::sa_addr_str(p->address());

    if (ip_address.size() >= 24) {
      ip_address.replace(ip_address.begin() + 21, ip_address.end(), "...");
    }

    m_canvas->print(x, y, "%c %s",
                    range.first == *m_focus ? '*' : ' ',
                    ip_address.c_str());
    x += 27;

    m_canvas->print(x, y, "%.1f", (double)p->up_rate()->rate() / 1024); x += 7;
    m_canvas->print(x, y, "%.1f", (double)p->down_rate()->rate() / 1024); x += 7;
    m_canvas->print(x, y, "%.1f", (double)p->peer_rate()->rate() / 1024); x += 7;

    char remoteChoked;
    char peerType;

    if (!p->is_down_choked_limited())
      remoteChoked = 'U';
    else if (p->is_down_queued())
      remoteChoked = 'Q';
    else
      remoteChoked = 'C';

    if (p->peer_info()->is_blocked())
      peerType = 'u';
    else if (p->peer_info()->is_preferred())
      peerType = 'p';
    else
      peerType = ' ';

    m_canvas->print(x, y, "%c%c/%c%c/%c%c",
                    p->is_encrypted() ? (p->is_incoming() ? 'R' : 'L') : (p->is_incoming() ? 'r' : 'l'),
                    peerType,
                    p->is_down_choked() ? std::tolower(remoteChoked) : remoteChoked,

                    p->is_down_interested() ? 'i' : 'n',
                    p->is_up_choked() ? 'c' : 'u',
                    p->is_up_interested() ? 'i' : 'n');
    x += 10;

    m_canvas->print(x, y, "%i/%i", p->outgoing_queue_size(), p->incoming_queue_size());
    x += 6;

    m_canvas->print(x, y, "%3i", done_percentage(*range.first));
    x += 6;

    const torrent::BlockTransfer* transfer = p->transfer();

    if (transfer != NULL)
      m_canvas->print(x, y, "%i", transfer->index());

    x += 6;

    if (p->is_snubbed())
      m_canvas->print(x, y, "*");

    x += 6;

    if (p->failed_counter() != 0)
      m_canvas->print(x, y, "%u", p->failed_counter());

    x += 7;

    char buf[128];
    print_client_version(buf, buf + 128, p->peer_info()->client_info());

    m_canvas->print(x, y, "%s", buf);

    ++y;
    ++range.first;
  }
}

int
WindowPeerList::done_percentage(torrent::Peer* p) {
  int chunks = m_download->download()->file_list()->size_chunks();

  return chunks ? (100 * p->chunks_done()) / chunks : 0;
}

}
