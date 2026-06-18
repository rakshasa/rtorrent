#include "config.h"

#include "input_event.h"

#include <torrent/common.h>
#include <torrent/system/poll.h>

#include "display/attributes.h"

namespace input {

void
InputEvent::insert() {
  torrent::this_thread::poll()->open(this);
  torrent::this_thread::poll()->insert_read(this);
  // EPOLLERR/EPOLLHUP is always delivered by epoll regardless of registration;
  // register for it so a controlling-terminal/pty hangup on stdin is dispatched
  // to event_error() instead of aborting the whole client (see event_error()).
  torrent::this_thread::poll()->insert_error(this);
}

void
InputEvent::remove() {
  // The file descriptor doubles as the open-state guard, mirroring SCgiTask.
  // event_error() may already have dropped stdin from the poll set and cleared
  // the fd; the shutdown path (Control::cleanup) still calls remove(), and a
  // second remove_and_close() would throw "event not found" via event_mask().
  if (!is_open())
    return;

  torrent::this_thread::poll()->remove_and_close(this);
  set_file_descriptor(-1);
}

void
InputEvent::event_read() {
  int c;

  while ((c = getch()) != ERR)
    m_slotPressed(c);
}

void
InputEvent::event_write() {
}

void
InputEvent::event_error() {
  // The controlling terminal/pty hung up (EPOLLERR on stdin). Drop stdin from
  // the poll set instead of letting Poll::process() throw an internal_error and
  // kill the client -- a regression introduced with the 0.16.13 callback/poll
  // rework. rtorrent keeps running with no keyboard input. Once removed the
  // event is out of the poll table, so no further event_error() is dispatched
  // to it; clearing the fd lets the shutdown remove() no-op.
  torrent::this_thread::poll()->remove_and_close(this);
  set_file_descriptor(-1);
}

}
