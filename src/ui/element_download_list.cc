#include "config.h"

#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/utils/log.h>

#include "core/download.h"
#include "core/manager.h"
#include "core/view.h"
#include "core/view_manager.h"
#include "display/frame.h"
#include "display/manager.h"
#include "input/manager.h"
#include "rpc/parse_commands.h"

#include "control.h"
#include "element_download_list.h"
#include "root.h"

namespace ui {

ElementDownloadList::ElementDownloadList() {
  receive_change_view("main");

  if (m_view == NULL)
    throw torrent::internal_error("View \"main\" must be present to initialize the main display.");

  m_bindings['\x13'] = [this] { receive_command("d.start="); };
  m_bindings['\x04'] = [this] { receive_command("branch=d.state=,d.stop=,d.erase="); };
  m_bindings['\x0B'] = [this] { receive_command("d.ignore_commands.set=1; d.stop=; d.close="); };
  m_bindings['\x12'] = [this] { receive_command("d.complete.set=0; d.check_hash="); };
  m_bindings['\x05'] = [this] { receive_command("f.multicall=,f.set_create_queued=0,f.set_resize_queued=0; print=\"Queued create/resize of files in torrent.\""); };

  m_bindings['+']       = [this] { receive_next_priority(); };
  m_bindings['-']       = [this] { receive_prev_priority(); };
  m_bindings['T' - '@'] = [this] { receive_cycle_throttle(); };
  m_bindings['I']       = [this] { receive_command("branch=d.ignore_commands=,"
                                                   "{d.ignore_commands.set=0, print=\"Torrent set to heed commands.\"},"
                                                   "{d.ignore_commands.set=1, print=\"Torrent set to ignore commands.\"}"); };
  m_bindings['B' - '@'] = [this] { receive_command("branch=d.is_active=,"
                                                   "{print=\"Cannot enable initial seeding on an active download.\"},"
                                                   "{d.connection_seed.set=initial_seed, print=\"Enabled initial seeding for the selected download.\"}"); };

  m_bindings['U'] = [this] { receive_command("d.delete_tied=; print=\"Cleared tied to file association for the selected download.\""); };

  // These should also be commands.
  m_bindings['1'] = [this] { receive_change_view("main"); };
  m_bindings['2'] = [this] { receive_change_view("name"); };
  m_bindings['3'] = [this] { receive_change_view("started"); };
  m_bindings['4'] = [this] { receive_change_view("stopped"); };
  m_bindings['5'] = [this] { receive_change_view("complete"); };
  m_bindings['6'] = [this] { receive_change_view("incomplete"); };
  m_bindings['7'] = [this] { receive_change_view("hashing"); };
  m_bindings['8'] = [this] { receive_change_view("seeding"); };
  m_bindings['9'] = [this] { receive_change_view("leeching"); };
  m_bindings['0'] = [this] { receive_change_view("active"); };

  m_bindings[KEY_UP] = m_bindings[control->ui()->navigation_key(RT_KEY_UP)] = [this] { receive_prev(); };
  m_bindings[KEY_DOWN] = m_bindings[control->ui()->navigation_key(RT_KEY_DOWN)] = [this] { receive_next(); };

  m_bindings[KEY_PPAGE] = m_bindings[control->ui()->navigation_key(RT_KEY_PPAGE)] = [this] { receive_pageprev(); };
  m_bindings[KEY_NPAGE] = m_bindings[control->ui()->navigation_key(RT_KEY_NPAGE)] = [this] { receive_pagenext(); };

  m_bindings[KEY_HOME]  = m_bindings[control->ui()->navigation_key(RT_KEY_HOME)]  = [this] { receive_home(); };
  m_bindings[KEY_END]   = m_bindings[control->ui()->navigation_key(RT_KEY_END)]   = [this] { receive_end(); };

  m_bindings[control->ui()->navigation_key(RT_KEY_TOGGLE_LAYOUT)] = [this] { toggle_layout(); };
}

void
ElementDownloadList::activate(display::Frame* frame, [[maybe_unused]] bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementDownloadList::activate(...) is_active().");

  control->input()->push_back(&m_bindings);

  m_window = new WDownloadList();
  m_window->set_active(true);
  m_window->set_view(m_view);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementDownloadList::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementDownloadList::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;
}

void
ElementDownloadList::set_view(core::View* l) {
  m_view = l;
  m_view->sort();

  if (m_window == NULL)
    return;

  m_window->set_view(l);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_command(const char* cmd) {
  try {
    if (m_view->focus() == m_view->end_visible())
      rpc::parse_command_multiple(rpc::make_target(), cmd, cmd + strlen(cmd));
    else
      rpc::parse_command_multiple(rpc::make_target(*m_view->focus()), cmd, cmd + strlen(cmd));

    m_view->set_last_changed();

  } catch (torrent::input_error& e) {
    lt_log_print(torrent::LOG_WARN, "Command failed: %s", e.what());
    return;
  }
}

void
ElementDownloadList::receive_next() {
  m_view->next_focus();
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_prev() {
  m_view->prev_focus();
  m_view->set_last_changed();
}

int
ElementDownloadList::page_size() {
  int rpc_page_size = rpc::call_command_value("ui.focus.page_size");
  if (rpc_page_size > 0)
    return rpc_page_size;
  int auto_page_size = m_window->page_size() - 1;
  if (auto_page_size > 0)
    return auto_page_size;
  return 50;
}

void
ElementDownloadList::receive_pagenext() {
  m_view->next_focus(page_size());
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_pageprev() {
  m_view->prev_focus(page_size());
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_home() {
  m_view->set_focus(m_view->begin_visible());
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_end() {
  m_view->set_focus(m_view->end_visible() - 1);
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_next_priority() {
  if (m_view->focus() == m_view->end_visible())
    return;

  (*m_view->focus())->set_priority((*m_view->focus())->priority() + 1);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_prev_priority() {
  if (m_view->focus() == m_view->end_visible())
    return;

  (*m_view->focus())->set_priority((*m_view->focus())->priority() - 1);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_cycle_throttle() {
  if (m_view->focus() == m_view->end_visible())
    return;

  core::Download* download = *m_view->focus();
  if (download->is_active()) {
    lt_log_print(torrent::LOG_TORRENT_WARN, "Cannot change throttle on active download.");
    return;
  }

  core::ThrottleMap::const_iterator itr = control->core()->throttles().find(download->bencode()->get_key("rtorrent").get_key_string("throttle_name"));
  if (itr == control->core()->throttles().end())
    itr = control->core()->throttles().begin();
  else
    ++itr;

  download->set_throttle_name(itr == control->core()->throttles().end() ? std::string() : itr->first);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_change_view(const std::string& name) {
  core::ViewManager::iterator itr = control->view_manager()->find(name);

  if (itr == control->view_manager()->end()) {
    control->core()->push_log_std("Could not find view \"" + name + "\".");
    return;
  }

  std::string old_name = view() ? view()->name() : "";
  if (!old_name.empty())
    rpc::commands.call_catch("event.view.hide", rpc::make_target(), name, "View hide event action failed: ");
  set_view(*itr);
  if (!old_name.empty())
    rpc::commands.call_catch("event.view.show", rpc::make_target(), old_name, "View show event action failed: ");
}

void
ElementDownloadList::toggle_layout() {
  const std::string layout_name = rpc::call_command_string("ui.torrent_list.layout");

  if (layout_name == "full") {
    rpc::call_command("ui.torrent_list.layout.set", "compact");
  } else if (layout_name == "compact") {
    rpc::call_command("ui.torrent_list.layout.set", "full");
  }
}
} // namespace ui
