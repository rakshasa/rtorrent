#ifndef RTORRENT_UI_ROOT_H
#define RTORRENT_UI_ROOT_H

#include <cstdint>
#include <memory>

#include "input/bindings.h"
#include "download_list.h"

class Control;

namespace display {
  class Frame;
  class WindowTitle;
  class WindowHttpQueue;
  class WindowInput;
  class WindowStatusbar;
}

namespace input {
  class TextInput;
}

namespace ui {

enum NavigationKeymap {
  RT_KEY_LEFT,
  RT_KEY_RIGHT,
  RT_KEY_UP,
  RT_KEY_DOWN,
  RT_KEY_PPAGE,
  RT_KEY_NPAGE,
  RT_KEY_HOME,
  RT_KEY_END,
  RT_KEY_DELETE_ITEM,
  RT_KEY_DISCONNECT_PEER,
  RT_KEY_DISPLAY_LOG,
  RT_KEY_TOGGLE_LAYOUT,
  RT_KEYMAP_MAX
};

class DownloadList;

typedef std::vector<std::string> ThrottleNameList;

class Root {
public:
  typedef display::WindowTitle     WTitle;
  typedef display::WindowHttpQueue WHttpQueue;
  typedef display::WindowInput     WInput;
  typedef display::WindowStatusbar WStatusbar;

  typedef std::map<int, int> InputHistoryPointers;
  typedef std::vector<std::string> InputHistoryCategory;
  typedef std::map<int, InputHistoryCategory> InputHistory;

  Root();

  void                init(Control* c);
  void                cleanup();

  const auto&         window_title() const                    { return m_windowTitle; }
  const auto&         window_statusbar() const                { return m_windowStatusbar; }
  const auto&         window_input() const                    { return m_windowInput; }

  const auto&         download_list() const                   { return m_downloadList; }

  void                set_down_throttle(unsigned int throttle);
  void                set_up_throttle(unsigned int throttle);

  // Rename to raw or something, make base function.
  void                set_down_throttle_i64(int64_t throttle) { set_down_throttle(throttle >> 10); }
  void                set_up_throttle_i64(int64_t throttle)   { set_up_throttle(throttle >> 10); }

  void                adjust_down_throttle(int throttle);
  void                adjust_up_throttle(int throttle);

  const char*         get_throttle_keys();

  ThrottleNameList&   get_status_throttle_up_names()          { return m_throttle_up_names; }
  ThrottleNameList&   get_status_throttle_down_names()        { return m_throttle_down_names; }

  void                set_status_throttle_up_names(const ThrottleNameList& throttle_list)      { m_throttle_up_names = throttle_list; }
  void                set_status_throttle_down_names(const ThrottleNameList& throttle_list)    { m_throttle_down_names = throttle_list; }

  void                enable_input(const std::string& title, input::TextInput* input, ui::DownloadList::Input type);
  void                disable_input();

  input::TextInput*   current_input();

  int                 get_input_history_size()                { return m_input_history_length; }
  void                set_input_history_size(int size);
  void                add_to_input_history(ui::DownloadList::Input type, std::string item);

  void                load_input_history();
  void                save_input_history();
  void                clear_input_history();

  const std::string&  keymap_style()                          { return m_keymap_style; }
  void                set_keymap_style(const std::string& style);
  const int           navigation_key(NavigationKeymap key);

private:
  void                setup_keys();

  Control*                      m_control{nullptr};
  std::unique_ptr<DownloadList> m_downloadList;

  std::unique_ptr<WTitle>       m_windowTitle;
  std::unique_ptr<WHttpQueue>   m_windowHttpQueue;
  std::unique_ptr<WInput>       m_windowInput;
  std::unique_ptr<WStatusbar>   m_windowStatusbar;

  input::Bindings      m_bindings;

  int                  m_input_history_length{99};
  std::string          m_input_history_last_input{""};
  int                  m_input_history_pointer_get{0};
  InputHistory         m_input_history;
  InputHistoryPointers m_input_history_pointers;

  void                prev_in_input_history(ui::DownloadList::Input type);
  void                next_in_input_history(ui::DownloadList::Input type);

  void                reset_input_history_attributes(ui::DownloadList::Input type);

  std::string         m_keymap_style;
  const int*          m_keymap;

  ThrottleNameList    m_throttle_up_names;
  ThrottleNameList    m_throttle_down_names;
};

}

#endif
