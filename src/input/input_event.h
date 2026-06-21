#ifndef RTORRENT_INPUT_INPUT_EVENT_H
#define RTORRENT_INPUT_INPUT_EVENT_H

#include <functional>

#include <torrent/event.h>

namespace input {

class InputEvent : public torrent::Event {
public:
  typedef std::function<void (int)> slot_int;

  InputEvent(int fd) { m_fileDesc = fd; }

  const char*         type_name() const override { return "input"; }

  void                insert();
  void                remove();

  void                event_read() override;
  void                event_write() override;
  void                event_error() override;

  void                slot_pressed(slot_int s) { m_slotPressed = s; }

private:
  slot_int            m_slotPressed;
};

}

#endif
