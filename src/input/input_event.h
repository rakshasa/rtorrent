#ifndef RTORRENT_INPUT_INPUT_EVENT_H
#define RTORRENT_INPUT_INPUT_EVENT_H

#include <functional>

#include <torrent/event.h>
#include <torrent/poll.h>

namespace input {

class InputEvent : public torrent::Event {
public:
  typedef std::function<void (int)> slot_int;

  InputEvent(int fd) { m_fileDesc = fd; }

  void                insert(torrent::Poll* p);
  void                remove(torrent::Poll* p);

  void                event_read();
  void                event_write();
  void                event_error();

  void                slot_pressed(slot_int s) { m_slotPressed = s; }

private:
  slot_int            m_slotPressed;
};

}

#endif
