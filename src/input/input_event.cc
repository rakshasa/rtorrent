#include "config.h"

#include "input_event.h"

#include "display/attributes.h"

namespace input {

void
InputEvent::insert(torrent::net::Poll* p) {
  p->open(this);
  p->insert_read(this);
}

void
InputEvent::remove(torrent::net::Poll* p) {
  p->remove_read(this);
  p->close(this);
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
}

}
