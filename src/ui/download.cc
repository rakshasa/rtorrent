#include "config.h"

#include <stdexcept>
#include <torrent/torrent.h>

#include "input/bindings.h"
#include "display/window_title.h"
#include "display/window_peer_list.h"

#include "control.h"
#include "download.h"

namespace ui {

Download::Download(DPtr d, Control* c) :
  m_download(d),
  m_title(new WTitle(d->get_download().get_name())),
  m_control(c),
  m_bindings(new input::Bindings) {

  m_window = new WPeerList(&m_peers);
  m_window->slot_chunks_total(sigc::mem_fun(m_download->get_download(), &torrent::Download::get_chunks_total));

  (*m_bindings)[KEY_UP]   = sigc::mem_fun(*this, &Download::receive_prev);
  (*m_bindings)[KEY_DOWN] = sigc::mem_fun(*this, &Download::receive_next);

  m_download->get_download().peer_list(m_peers);

  m_connPeerConnected = m_download->get_download().signal_peer_connected(sigc::mem_fun(*this, &Download::receive_peer_connected));
  m_connPeerDisconnected = m_download->get_download().signal_peer_disconnected(sigc::mem_fun(*this, &Download::receive_peer_disconnected));
}

Download::~Download() {
  m_connPeerConnected.disconnect();
  m_connPeerDisconnected.disconnect();

  delete m_window;
  delete m_title;
  delete m_bindings;
}

void
Download::activate() {
  m_control->get_display().push_front(m_window);
  m_control->get_display().push_front(m_title);
  m_control->get_input().push_front(m_bindings);
}

void
Download::disable() {
  m_control->get_display().erase(m_window);
  m_control->get_display().erase(m_title);
  m_control->get_input().erase(m_bindings);
}

void
Download::receive_next() {
  if (m_window->get_focus() == m_window->get_list().end())
    m_window->set_focus(m_window->get_list().begin());
  else
    m_window->set_focus(++m_window->get_focus());
}

void
Download::receive_prev() {
  if (m_window->get_focus() == m_window->get_list().begin())
    m_window->set_focus(m_window->get_list().end());
  else
    m_window->set_focus(--m_window->get_focus());
}

void
Download::receive_peer_connected(torrent::Peer p) {
  m_peers.push_back(p);
}

void
Download::receive_peer_disconnected(torrent::Peer p) {
  PList::iterator itr = std::find(m_peers.begin(), m_peers.end(), p);

  if (itr == m_peers.end())
    throw std::logic_error("Download::receive_peer_disconnected(...) received a peer we don't have in our list");

  if (itr == m_window->get_focus())
    m_window->set_focus(m_peers.erase(itr));
  else
    m_peers.erase(itr);
}

}
