// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/chunk_manager.h>
#include <torrent/connection_manager.h>
#include <torrent/throttle.h>
#include <torrent/torrent.h>
#include <torrent/tracker_list.h>
#include <torrent/data/file_list.h>
#include <torrent/peer/connection_list.h>

#include "core/download.h"
#include "core/manager.h"
#include "input/manager.h"
#include "display/window_title.h"
#include "display/window_download_statusbar.h"

#include "display/text_element_string.h"

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

  m_uiArray[DISPLAY_MENU]->slot_exit(std::bind(&slot_type::operator(), &m_slot_exit));
  m_uiArray[DISPLAY_PEER_LIST]->slot_exit(std::bind(&Download::activate_display_menu, this, DISPLAY_PEER_LIST));
  m_uiArray[DISPLAY_INFO]->slot_exit(std::bind(&Download::activate_display_menu, this, DISPLAY_INFO));
  m_uiArray[DISPLAY_FILE_LIST]->slot_exit(std::bind(&Download::activate_display_menu, this, DISPLAY_FILE_LIST));
  m_uiArray[DISPLAY_TRACKER_LIST]->slot_exit(std::bind(&Download::activate_display_menu, this, DISPLAY_TRACKER_LIST));
  m_uiArray[DISPLAY_CHUNKS_SEEN]->slot_exit(std::bind(&Download::activate_display_menu, this, DISPLAY_CHUNKS_SEEN));
  m_uiArray[DISPLAY_TRANSFER_LIST]->slot_exit(std::bind(&Download::activate_display_menu, this, DISPLAY_TRANSFER_LIST));

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
                     std::bind(&Download::activate_display_focus, this, DISPLAY_PEER_LIST),
                     std::bind(&Download::activate_display_menu, this, DISPLAY_PEER_LIST));
  element->push_back("Info",
                     std::bind(&Download::activate_display_focus, this, DISPLAY_INFO),
                     std::bind(&Download::activate_display_menu, this, DISPLAY_INFO));
  element->push_back("File list",
                     std::bind(&Download::activate_display_focus, this, DISPLAY_FILE_LIST),
                     std::bind(&Download::activate_display_menu, this, DISPLAY_FILE_LIST));
  element->push_back("Tracker list",
                     std::bind(&Download::activate_display_focus, this, DISPLAY_TRACKER_LIST),
                     std::bind(&Download::activate_display_menu, this, DISPLAY_TRACKER_LIST));
  element->push_back("Chunks seen",
                     std::bind(&Download::activate_display_focus, this, DISPLAY_CHUNKS_SEEN),
                     std::bind(&Download::activate_display_menu, this, DISPLAY_CHUNKS_SEEN));
  element->push_back("Transfer list",
                     std::bind(&Download::activate_display_focus, this, DISPLAY_TRANSFER_LIST),
                     std::bind(&Download::activate_display_menu, this, DISPLAY_TRANSFER_LIST));

  element->set_entry(0, false);

  m_bindings['p'] = std::bind(&ElementMenu::set_entry_trigger, element, 0);
  m_bindings['o'] = std::bind(&ElementMenu::set_entry_trigger, element, 1);
  m_bindings['i'] = std::bind(&ElementMenu::set_entry_trigger, element, 2);
  m_bindings['u'] = std::bind(&ElementMenu::set_entry_trigger, element, 3);

  return element;
}

inline ElementBase*
Download::create_info() {
  using namespace display::helpers;

  ElementText* element = new ElementText(rpc::make_target(m_download));

  element->set_column(1);
  element->set_interval(1);

  // Get these bindings with some kind of string map.

  element->push_column("Name:",             te_command("d.name="));
  element->push_column("Local id:",         te_command("d.local_id_html="));
  element->push_column("Info hash:",        te_command("d.hash="));
  element->push_column("Created:",          te_command("cat=$convert.date=$d.creation_date=,\" \",$convert.time=$d.creation_date="));

  element->push_back("");
  element->push_column("Directory:",        te_command("d.directory="));
  element->push_column("Base Path:",        te_command("d.base_path="));
  element->push_column("Tied to file:",     te_command("d.tied_to_file="));
  element->push_column("File stats:",       te_command("cat=$if=$d.is_multi_file=\\,multi\\,single,\" \",$d.size_files=,\" files\""));

  element->push_back("");
  element->push_column("Chunks:",           te_command("cat=(d.completed_chunks),\" / \",(d.size_chunks),\" * \",(d.chunk_size),\" (\",(d.wanted_chunks),\")\""));
  element->push_column("Priority:",         te_command("d.priority="));
  element->push_column("Peer exchange:",    te_command("cat=$if=$d.peer_exchange=\\,enabled\\,disabled,\\ ,"
                                                       "$if=$d.is_pex_active=\\,active\\,$d.is_private=\\,private\\,inactive,"
                                                       "\\ (,$d.size_pex=,/,$d.max_size_pex=,)"));

  element->push_column("State changed:",    te_command("convert.elapsed_time=$d.state_changed="));

  element->push_back("");
  element->push_column("Memory usage:",     te_command("cat=$convert.mb=$pieces.memory.current=,\" MB\""));
  element->push_column("Max memory usage:", te_command("cat=$convert.mb=$pieces.memory.max=,\" MB\""));
  element->push_column("Free diskspace:",   te_command("cat=$convert.mb=$d.free_diskspace=,\" MB\""));
  element->push_column("Safe diskspace:",   te_command("cat=$convert.mb=$pieces.sync.safe_free_diskspace=,\" MB\""));

  element->push_back("");
  element->push_column("Connection type:",  te_command("cat=(d.connection_current),\" \",(if,(d.accepting_seeders),"",\"no_seeders\")"));
  element->push_column("Choke heuristic:",  te_command("cat=(d.up.choke_heuristics),\", \",(d.down.choke_heuristics),\", \",(d.group)"));
  element->push_column("Safe sync:",        te_command("if=$pieces.sync.always_safe=,yes,no"));
  element->push_column("Send buffer:",      te_command("cat=$convert.kb=$network.send_buffer.size=,\" KB\""));
  element->push_column("Receive buffer:",   te_command("cat=$convert.kb=$network.receive_buffer.size=,\" KB\""));

  // TODO: Define a custom command for this and use $argument.0 instead of looking up the name multiple times?
  element->push_column("Throttle:",         te_command("branch=d.throttle_name=,\""
                                                              "cat=$d.throttle_name=,\\\"  [Max \\\","
                                                                  "$convert.throttle=$throttle.up.max=$d.throttle_name=,\\\"/\\\","
                                                                  "$convert.throttle=$throttle.down.max=$d.throttle_name=,\\\" KB]  [Rate \\\","
                                                                  "$convert.kb=$throttle.up.rate=$d.throttle_name=,\\\"/\\\","
                                                                  "$convert.kb=$throttle.down.rate=$d.throttle_name=,\\\" KB]\\\"\","
                                                              "cat=\"global\""));

  element->push_back("");
  element->push_column("Upload:",           te_command("cat=$convert.kb=$d.up.rate=,\" KB / \",$convert.xb=$d.up.total="));
  element->push_column("Download:",         te_command("cat=$convert.kb=$d.down.rate=,\" KB / \",$convert.xb=$d.down.total="));
  element->push_column("Skipped:",          te_command("cat=$convert.kb=$d.skip.rate=,\" KB / \",$convert.xb=$d.skip.total="));
  element->push_column("Preload:",          te_command("cat=$pieces.preload.type=,\" / \",$pieces.stats_preloaded=,\" / \",$pieces.stats_preloaded="));

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
  default: control->ui()->window_title()->set_title(m_download->info()->name()); break;
  }

  control->display()->adjust_layout();
}

void
Download::receive_max_uploads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_uploads_max(std::max<int32_t>(m_download->download()->uploads_max() + t, 0));
}

void
Download::receive_min_uploads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_uploads_min(std::max<int32_t>(m_download->download()->uploads_min() + t, 0));
}

void
Download::receive_max_downloads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_downloads_max(std::max<int32_t>(m_download->download()->downloads_max() + t, 0));
}

void
Download::receive_min_downloads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_downloads_min(std::max<int32_t>(m_download->download()->downloads_min() + t, 0));
}

void
Download::receive_min_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->connection_list()->set_min_size(std::max<int32_t>(m_download->download()->connection_list()->min_size() + t, (int32_t)5));
}

void
Download::receive_max_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->connection_list()->set_max_size(std::max<int32_t>(m_download->download()->connection_list()->max_size() + t, (int32_t)5));
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
Download::adjust_down_throttle(int throttle) {
  core::ThrottleMap::iterator itr = control->core()->throttles().find(m_download->bencode()->get_key("rtorrent").get_key_string("throttle_name"));

  if (itr == control->core()->throttles().end() || itr->second.second == NULL || itr->first == "NULL")
    control->ui()->adjust_down_throttle(throttle);
  else
    itr->second.second->set_max_rate(std::max<int>((itr->second.second->is_throttled() ? itr->second.second->max_rate() : 0) + throttle * 1024, 0));

  if (m_uiArray[DISPLAY_INFO]->is_active())
    m_uiArray[DISPLAY_INFO]->mark_dirty();
}

void
Download::adjust_up_throttle(int throttle) {
  core::ThrottleMap::iterator itr = control->core()->throttles().find(m_download->bencode()->get_key("rtorrent").get_key_string("throttle_name"));

  if (itr == control->core()->throttles().end() || itr->second.first == NULL || itr->first == "NULL")
    control->ui()->adjust_up_throttle(throttle);
  else
    itr->second.first->set_max_rate(std::max<int>((itr->second.first->is_throttled() ? itr->second.first->max_rate() : 0) + throttle * 1024, 0));

  if (m_uiArray[DISPLAY_INFO]->is_active())
    m_uiArray[DISPLAY_INFO]->mark_dirty();
}

void
Download::bind_keys() {
  m_bindings['1'] = std::bind(&Download::receive_min_uploads, this, -1);
  m_bindings['2'] = std::bind(&Download::receive_min_uploads, this, 1);
  m_bindings['3'] = std::bind(&Download::receive_max_uploads, this, -1);
  m_bindings['4'] = std::bind(&Download::receive_max_uploads, this, 1);
  m_bindings['!'] = std::bind(&Download::receive_min_downloads, this, -1);
  m_bindings['@'] = std::bind(&Download::receive_min_downloads, this, 1);
  m_bindings['#'] = std::bind(&Download::receive_max_downloads, this, -1);
  m_bindings['$'] = std::bind(&Download::receive_max_downloads, this, 1);
  m_bindings['5'] = std::bind(&Download::receive_min_peers, this, -5);
  m_bindings['6'] = std::bind(&Download::receive_min_peers, this, 5);
  m_bindings['7'] = std::bind(&Download::receive_max_peers, this, -5);
  m_bindings['8'] = std::bind(&Download::receive_max_peers, this, 5);
  m_bindings['+'] = std::bind(&Download::receive_next_priority, this);
  m_bindings['-'] = std::bind(&Download::receive_prev_priority, this);

  m_bindings['t'] = std::bind(&torrent::Download::manual_request, m_download->download(), false);
  m_bindings['T'] = std::bind(&torrent::Download::manual_request, m_download->download(), true);

  const char* keys = control->ui()->get_throttle_keys();

  m_bindings[keys[ 0]] = std::bind(&Download::adjust_up_throttle, this, 1);
  m_bindings[keys[ 1]] = std::bind(&Download::adjust_up_throttle, this, -1);
  m_bindings[keys[ 2]] = std::bind(&Download::adjust_down_throttle, this, 1);
  m_bindings[keys[ 3]] = std::bind(&Download::adjust_down_throttle, this, -1);

  m_bindings[keys[ 4]] = std::bind(&Download::adjust_up_throttle, this, 5);
  m_bindings[keys[ 5]] = std::bind(&Download::adjust_up_throttle, this, -5);
  m_bindings[keys[ 6]] = std::bind(&Download::adjust_down_throttle, this, 5);
  m_bindings[keys[ 7]] = std::bind(&Download::adjust_down_throttle, this, -5);

  m_bindings[keys[ 8]] = std::bind(&Download::adjust_up_throttle, this, 50);
  m_bindings[keys[ 9]] = std::bind(&Download::adjust_up_throttle, this, -50);
  m_bindings[keys[10]] = std::bind(&Download::adjust_down_throttle, this, 50);
  m_bindings[keys[11]] = std::bind(&Download::adjust_down_throttle, this, -50);
}

}
