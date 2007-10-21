// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <sigc++/adaptors/bind.h>
#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/chunk_manager.h>
#include <torrent/connection_manager.h>
#include <torrent/torrent.h>
#include <torrent/tracker_list.h>
#include <torrent/data/file_list.h>

#include "core/download.h"
#include "input/manager.h"
#include "display/window_title.h"
#include "display/window_download_statusbar.h"

#include "display/text_element_lambda.h"
#include "display/text_element_helpers.h"

#include "control.h"
#include "download.h"
#include "root.h"
#include "element_file_list.h"
#include "element_menu.h"
#include "element_text.h"
#include "element_peer_list.h"
#include "element_tracker_list.h"
#include "element_chunks_seen.h"
#include "element_transfer_list.h"

namespace ui {

Download::Download(core::Download* d) :
  m_download(d),
  m_state(DISPLAY_MAX_SIZE),
  m_focusDisplay(false) {

  m_windowDownloadStatus = new WDownloadStatus(d);
  m_windowDownloadStatus->set_bottom(true);

  m_uiArray[DISPLAY_MENU]          = create_menu();
  m_uiArray[DISPLAY_PEER_LIST]     = new ElementPeerList(d);
  m_uiArray[DISPLAY_INFO]          = create_info();
  m_uiArray[DISPLAY_FILE_LIST]     = new ElementFileList(d);
  m_uiArray[DISPLAY_TRACKER_LIST]  = new ElementTrackerList(d);
  m_uiArray[DISPLAY_CHUNKS_SEEN]   = new ElementChunksSeen(d);
  m_uiArray[DISPLAY_TRANSFER_LIST] = new ElementTransferList(d);

  m_uiArray[DISPLAY_MENU]->slot_exit(sigc::mem_fun(&m_slotExit, &slot_type::operator()));
  m_uiArray[DISPLAY_PEER_LIST]->slot_exit(sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_PEER_LIST));
  m_uiArray[DISPLAY_INFO]->slot_exit(sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_INFO));
  m_uiArray[DISPLAY_FILE_LIST]->slot_exit(sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_FILE_LIST));
  m_uiArray[DISPLAY_TRACKER_LIST]->slot_exit(sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_TRACKER_LIST));
  m_uiArray[DISPLAY_CHUNKS_SEEN]->slot_exit(sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_CHUNKS_SEEN));
  m_uiArray[DISPLAY_TRANSFER_LIST]->slot_exit(sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_TRANSFER_LIST));

  bind_keys();
}

Download::~Download() {
  if (is_active())
    throw torrent::internal_error("ui::Download::~Download() called on an active object.");

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowDownloadStatus;
}

inline ElementBase*
Download::create_menu() {
  ElementMenu* element = new ElementMenu;

  element->push_back("Peer list",
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_PEER_LIST),
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_PEER_LIST));
  element->push_back("Info",
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_INFO),
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_INFO));
  element->push_back("File list",
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_FILE_LIST),
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_FILE_LIST));
  element->push_back("Tracker list",
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_TRACKER_LIST),
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_TRACKER_LIST));
  element->push_back("Chunks seen",
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_CHUNKS_SEEN),
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_CHUNKS_SEEN));
  element->push_back("Transfer list",
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_TRANSFER_LIST),
                     sigc::bind(sigc::mem_fun(this, &Download::activate_display_menu), DISPLAY_TRANSFER_LIST));

  element->set_entry(0, false);

  m_bindings['p'] = sigc::bind(sigc::mem_fun(element, &ElementMenu::set_entry_trigger), 0);
  m_bindings['o'] = sigc::bind(sigc::mem_fun(element, &ElementMenu::set_entry_trigger), 1);
  m_bindings['i'] = sigc::bind(sigc::mem_fun(element, &ElementMenu::set_entry_trigger), 2);
  m_bindings['u'] = sigc::bind(sigc::mem_fun(element, &ElementMenu::set_entry_trigger), 3);

  return element;
}

inline ElementBase*
Download::create_info() {
  using namespace display::helpers;

  ElementText* element = new ElementText(rpc::make_target(m_download));

  element->set_column(1);
  element->set_interval(1);

  // Get these bindings with some kind of string map.

  element->push_column("Name:",             te_command("d.get_name="));
  element->push_column("Local id:",         te_command("d.get_local_id_html="));
  element->push_column("Info hash:",        te_command("d.get_hash="));
  element->push_column("Created:",          te_command("cat=$to_date=$d.get_creation_date=,\" \",$to_time=$d.get_creation_date="));

  element->push_back("");
  element->push_column("Directory:",        te_command("d.get_base_path="));
  element->push_column("Tied to file:",     te_command("d.get_tied_to_file="));
  element->push_column("File stats:",       te_command("cat=$if=$d.is_multi_file=\\,multi\\,single,\" \",$d.get_size_files=,\" files\""));

  element->push_back("");
  element->push_column("Chunks:",           te_command("cat=$d.get_completed_chunks=,\" / \",$d.get_size_chunks=,\" * \",$d.get_chunk_size="));
  element->push_column("Priority:",         te_command("d.get_priority="));
  element->push_column("Peer exchange:",    te_command("cat=$if=$d.get_peer_exchange=\\,enabled\\,disabled,\\ ,"
                                                       "$if=$d.is_pex_active=\\,active\\,$d.is_private=\\,private\\,inactive,"
                                                       "\\ (,$d.get_size_pex=,/,$d.get_max_size_pex=,)"));

  element->push_column("State changed:",    te_command("to_elapsed_time=$d.get_state_changed="));

  element->push_back("");
  element->push_column("Memory usage:",     te_command("cat=$to_mb=$get_memory_usage=,\" MB\""));
  element->push_column("Max memory usage:", te_command("cat=$to_mb=$get_max_memory_usage=,\" MB\""));
  element->push_column("Free diskspace:",   te_command("cat=$to_mb=$d.get_free_diskspace=,\" MB\""));
  element->push_column("Safe diskspace:",   te_command("cat=$to_mb=$get_safe_free_diskspace=,\" MB\""));

  element->push_back("");
  element->push_column("Connection type:",  te_command("d.get_connection_current="));
  element->push_column("Safe sync:",        te_command("if=$get_safe_sync=,yes,no"));
  element->push_column("Send buffer:",      te_command("cat=$to_mb=$get_send_buffer_size=,\" KB\""));
  element->push_column("Receive buffer:",   te_command("cat=$to_mb=$get_receive_buffer_size=,\" KB\""));

  element->push_back("");
  element->push_column("Upload:",           te_command("cat=$to_kb=$d.get_up_rate=,\" KB / \",$to_xb=$d.get_up_total="));
  element->push_column("Download:",         te_command("cat=$to_kb=$d.get_down_rate=,\" KB / \",$to_xb=$d.get_down_total="));
  element->push_column("Skipped:",          te_command("cat=$to_kb=$d.get_skip_rate=,\" KB / \",$to_xb=$d.get_skip_total="));
  element->push_column("Preload:",          te_command("cat=$get_preload_type=,\" / \",$get_stats_preloaded=,\" / \",$get_stats_not_preloaded="));

  element->set_column_width(element->column_width() + 1);

  return element;
}

void
Download::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::Download::activate() called on an already activated object.");

  control->input()->push_back(&m_bindings);

  m_frame = frame;
  m_frame->initialize_row(2);

  m_frame->frame(1)->initialize_window(m_windowDownloadStatus);
  m_windowDownloadStatus->set_active(true);

  activate_display_menu(DISPLAY_PEER_LIST);
}

void
Download::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::Download::disable() called on an already disabled object.");

  control->input()->erase(&m_bindings);

  activate_display_focus(DISPLAY_MAX_SIZE);

  m_windowDownloadStatus->set_active(false);
  m_frame->clear();

  m_frame = NULL;
}

void
Download::activate_display(Display displayType, bool focusDisplay) {
  if (!is_active())
    throw torrent::internal_error("ui::Download::activate_display(...) !is_active().");

  if (displayType > DISPLAY_MAX_SIZE)
    throw torrent::internal_error("ui::Download::activate_display(...) out of bounds");

  if (focusDisplay == m_focusDisplay && displayType == m_state)
    return;

  display::Frame* frame = m_frame->frame(0);

  // Cleanup previous state.
  switch (m_state) {
  case DISPLAY_PEER_LIST:
  case DISPLAY_INFO:
  case DISPLAY_FILE_LIST:
  case DISPLAY_TRACKER_LIST:
  case DISPLAY_CHUNKS_SEEN:
  case DISPLAY_TRANSFER_LIST:
    m_uiArray[DISPLAY_MENU]->disable();
    m_uiArray[m_state]->disable();

    frame->clear();
    break;

  case DISPLAY_MENU:
  case DISPLAY_MAX_SIZE:
    break;
  }

  m_state = displayType;
  m_focusDisplay = focusDisplay;

  // Initialize new state.
  switch (displayType) {
  case DISPLAY_PEER_LIST:
  case DISPLAY_INFO:
  case DISPLAY_FILE_LIST:
  case DISPLAY_TRACKER_LIST:
  case DISPLAY_CHUNKS_SEEN:
  case DISPLAY_TRANSFER_LIST:
    frame->initialize_column(2);

    m_uiArray[DISPLAY_MENU]->activate(frame->frame(0), !focusDisplay);
    m_uiArray[displayType]->activate(frame->frame(1), focusDisplay);
    break;

  case DISPLAY_MENU:
  case DISPLAY_MAX_SIZE:
    break;
  }

  // Set title.
  switch (displayType) {
  case DISPLAY_MAX_SIZE: break;
  default: control->ui()->window_title()->set_title(m_download->download()->name()); break;
  }

  control->display()->adjust_layout();
}

void
Download::receive_max_uploads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_uploads_max(std::max(m_download->download()->uploads_max() + t, (uint32_t)2));
}

void
Download::receive_min_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_peers_min(std::max(m_download->download()->peers_min() + t, (uint32_t)5));
}

void
Download::receive_max_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_peers_max(std::max(m_download->download()->peers_max() + t, (uint32_t)5));
}

void
Download::receive_next_priority() {
  m_download->set_priority((m_download->priority() + 1) % 4);
}

void
Download::receive_prev_priority() {
  m_download->set_priority((m_download->priority() - 1) % 4);
}

void
Download::bind_keys() {
  m_bindings['1'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_uploads), -1);
  m_bindings['2'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_uploads), 1);
  m_bindings['3'] = sigc::bind(sigc::mem_fun(this, &Download::receive_min_peers), -5);
  m_bindings['4'] = sigc::bind(sigc::mem_fun(this, &Download::receive_min_peers), 5);
  m_bindings['5'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_peers), -5);
  m_bindings['6'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_peers), 5);
  m_bindings['+'] = sigc::mem_fun(this, &Download::receive_next_priority);
  m_bindings['-'] = sigc::mem_fun(this, &Download::receive_prev_priority);

  m_bindings['t'] = sigc::bind(sigc::mem_fun(m_download->tracker_list(), &torrent::TrackerList::manual_request), false);
  m_bindings['T'] = sigc::bind(sigc::mem_fun(m_download->tracker_list(), &torrent::TrackerList::manual_request), true);
}

}
