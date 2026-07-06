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
}

void
InputEvent::remove() {
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
  torrent::this_thread::poll()->remove_and_close(this);
  set_file_descriptor(-1);
}

}
