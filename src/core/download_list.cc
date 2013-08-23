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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/data/file.h>
#include <torrent/utils/resume.h>
#include <torrent/exceptions.h>
#include <torrent/download.h>
#include <torrent/hash_string.h>
#include <torrent/object.h>
#include <torrent/object_stream.h>
#include <torrent/torrent.h>
#include <torrent/utils/log.h>

#include "rpc/parse_commands.h"

#include "control.h"
#include "globals.h"
#include "manager.h"
#include "view.h"
#include "view_manager.h"

#include "dht_manager.h"
#include "download.h"
#include "download_list.h"
#include "download_store.h"

#define DL_TRIGGER_EVENT(download, event_name) \
  rpc::commands.call_catch(event_name, rpc::make_target(download), torrent::Object(), "Event '" event_name "' failed: ");

namespace core {

inline void
DownloadList::check_contains(Download* d) {
#ifdef USE_EXTRA_DEBUG
  if (std::find(begin(), end(), d) == end())
    throw torrent::internal_error("DownloadList::check_contains(...) failed.");
#endif
}

void
DownloadList::clear() {
  std::for_each(begin(), end(), std::bind1st(std::mem_fun(&DownloadList::close), this));
  std::for_each(begin(), end(), rak::call_delete<Download>());

  base_type::clear();
}

void
DownloadList::session_save() {
  unsigned int c = std::count_if(begin(), end(), std::bind1st(std::mem_fun(&DownloadStore::save_resume), control->core()->download_store()));

  if (c != size())
    lt_log_print(torrent::LOG_ERROR, "Failed to save session torrents.");

  control->dht_manager()->save_dht_cache();
}

DownloadList::iterator
DownloadList::find(const torrent::HashString& hash) {
  return std::find_if(begin(), end(), rak::equal(hash, rak::on(std::mem_fun(&Download::info), std::mem_fun(&torrent::DownloadInfo::hash))));
}

DownloadList::iterator
DownloadList::find_hex(const char* hash) {
  torrent::HashString key;

  for (torrent::HashString::iterator itr = key.begin(), last = key.end(); itr != last; itr++, hash += 2)
    *itr = (rak::hexchar_to_value(*hash) << 4) + rak::hexchar_to_value(*(hash + 1));

  return std::find_if(begin(), end(), rak::equal(key, rak::on(std::mem_fun(&Download::info), std::mem_fun(&torrent::DownloadInfo::hash))));
}

Download*
DownloadList::find_hex_ptr(const char* hash) {
  iterator itr = find_hex(hash);

  return itr != end() ? *itr : NULL;
}

Download*
DownloadList::create(torrent::Object* obj, bool printLog) {
  torrent::Download download;

  try {
    download = torrent::download_add(obj);

  } catch (torrent::local_error& e) {
    delete obj;

    if (printLog)
      lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not create download: %s", e.what());

    return NULL;
  }

  // There's no non-critical exceptions that should be throwable by
  // the ctor, so don't catch.
  return new Download(download);
}

Download*
DownloadList::create(std::istream* str, bool printLog) {
  torrent::Object* object = new torrent::Object;
  torrent::Download download;

  try {
    *str >> *object;
    
    // Don't throw input_error from here as gcc-3.3.5 produces bad
    // code.
    if (str->fail()) {
      delete object;

      if (printLog)
        lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not create download, the input is not a valid torrent.");

      return NULL;
    }

    download = torrent::download_add(object);

  } catch (torrent::local_error& e) {
    delete object;

    if (printLog)
      lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not create download: %s", e.what());

    return NULL;
  }

  // There's no non-critical exceptions that should be throwable by
  // the ctor, so don't catch.
  return new Download(download);
}

DownloadList::iterator
DownloadList::insert(Download* download) {
  iterator itr = base_type::insert(end(), download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Inserting download.");

  try {
    (*itr)->data()->slot_initial_hash()        = tr1::bind(&DownloadList::hash_done, this, download);
    (*itr)->data()->slot_download_done()       = tr1::bind(&DownloadList::received_finished, this, download);

    // This needs to be separated into two different calls to ensure
    // the download remains in the view.
    std::for_each(control->view_manager()->begin(), control->view_manager()->end(), std::bind2nd(std::mem_fun(&View::insert), download));
    std::for_each(control->view_manager()->begin(), control->view_manager()->end(), std::bind2nd(std::mem_fun(&View::filter_download), download));

    DL_TRIGGER_EVENT(*itr, "event.download.inserted");

  } catch (torrent::local_error& e) {
    // Should perhaps relax this, just print an error and remove the
    // downloads?
    throw torrent::internal_error("Caught during DownloadList::insert(...): " + std::string(e.what()));
  }

  return itr;
}

void
DownloadList::erase_ptr(Download* download) {
  erase(std::find(begin(), end(), download));
}

DownloadList::iterator
DownloadList::erase(iterator itr) {
  if (itr == end())
    throw torrent::internal_error("DownloadList::erase(...) could not find download.");

  lt_log_print_info(torrent::LOG_TORRENT_INFO, (*itr)->info(), "download_list", "Erasing download.");

  // Makes sure close doesn't restart hashing of this download.
  (*itr)->set_hash_failed(true);

  close(*itr);

  control->core()->download_store()->remove(*itr);

  DL_TRIGGER_EVENT(*itr, "event.download.erased");
  std::for_each(control->view_manager()->begin(), control->view_manager()->end(), std::bind2nd(std::mem_fun(&View::erase), *itr));

  torrent::download_remove(*(*itr)->download());
  delete *itr;

  return base_type::erase(itr);
}

bool
DownloadList::open(Download* download) {
  try {

    open_throw(download);

    return true;

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not open download: %s", e.what());
    return false;
  }
}

void
DownloadList::open_throw(Download* download) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Opening download.");

  if (download->download()->info()->is_open())
    return;
  
  int openFlags = download->resume_flags();

  if (rpc::call_command_value("system.file.allocate"))
    openFlags |= torrent::Download::open_enable_fallocate;

  download->download()->open(openFlags);
  DL_TRIGGER_EVENT(download, "event.download.opened");
}

void
DownloadList::close(Download* download) {
  try {
    close_throw(download);

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not close download: %s", e.what());
  }
}

void
DownloadList::close_directly(Download* download) {
  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Closing download directly.");

  if (download->download()->info()->is_active()) {
    download->download()->stop(torrent::Download::stop_skip_tracker);

    if (torrent::resume_check_target_files(*download->download(), download->download()->bencode()->get_key("libtorrent_resume")))
      torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));
  }

  if (download->download()->info()->is_open())
    download->download()->close();
}

void
DownloadList::close_quick(Download* download) {
  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Closing download quickly.");
  close(download);
  
  // Make sure we cancel any tracker requests. This should rather be
  // handled by some parameter to the close function, or some other
  // way of giving the client more control of when STOPPED requests
  // are sent.
  download->download()->manual_cancel();
}

void
DownloadList::close_throw(Download* download) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Closing download with throw.");

  // When pause gets called it will clear the initial hash check state
  // and set hash failed. This should ensure hashing doesn't restart
  // until resume gets called.
  pause(download);

  // Check for is_open after pause due to hashing.
  if (!download->is_open())
    return;

  // Save the torrent on close, this covers shutdown and if a torrent
  // is manually closed which would clear the progress data. For
  // better crash protection, save regulary in addition to this.
  //
  // Used to be in pause, but this was wrong for rehashing etc.
  //
  // Reconsider this save. Should be done explicitly when shutting down.
  //control->core()->download_store()->save(download);

  download->download()->close();

  if (!download->is_hash_failed() && rpc::call_command_value("d.hashing", rpc::make_target(download)) != Download::variable_hashing_stopped)
    throw torrent::internal_error("DownloadList::close_throw(...) called but we're going into a hashing loop.");

  DL_TRIGGER_EVENT(download, "event.download.hash_removed");
  DL_TRIGGER_EVENT(download, "event.download.closed");
}

void
DownloadList::resume(Download* download, int flags) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Resuming download: flags:%0x.", flags);

  try {

    if (download->download()->info()->is_active())
      return;

    rpc::parse_command_single(rpc::make_target(download), "view.set_visible=active");

    // We need to make sure the flags aren't reset if someone decideds
    // to call resume() while it is hashing, etc.
    if (download->resume_flags() == ~uint32_t())
      download->set_resume_flags(flags);

    // Manual or end-of-download rehashing clears the resume data so
    // we can just start the hashing again without clearing it again.
    //
    // It is also assumed the is_hash_checked flag gets cleared when
    // 'hashing' was set.
    if (!download->is_hash_checked()) {
      // If the hash failed flag wasn't cleared then hashing won't be
      // initiated.
      if (download->is_hash_failed())
        return;

      if (rpc::call_command_value("d.hashing", rpc::make_target(download)) == Download::variable_hashing_stopped)
        rpc::call_command("d.hashing.set", Download::variable_hashing_initial, rpc::make_target(download));

      DL_TRIGGER_EVENT(download, "event.download.hash_queued");
      return;
    }

    // This will never actually do anything due to the above hash check.
    // open_throw(download);

    rpc::call_command("d.state_changed.set", cachedTime.seconds(), rpc::make_target(download));
    rpc::call_command("d.state_counter.set", rpc::call_command_value("d.state_counter", rpc::make_target(download)) + 1, rpc::make_target(download));

    if (download->is_done()) {
      torrent::Object conn_current = rpc::call_command("d.connection_seed", torrent::Object(), rpc::make_target(download));
      torrent::Object choke_up     = rpc::call_command("d.up.choke_heuristics.seed", torrent::Object(), rpc::make_target(download));
      torrent::Object choke_down   = rpc::call_command("d.down.choke_heuristics.seed", torrent::Object(), rpc::make_target(download));

      if (conn_current.is_string_empty()) conn_current = rpc::call_command("protocol.connection.seed", torrent::Object(), rpc::make_target(download));
      if (choke_up.is_string_empty())     choke_up     = rpc::call_command("protocol.choke_heuristics.up.seed", torrent::Object(), rpc::make_target(download));
      if (choke_down.is_string_empty())   choke_down   = rpc::call_command("protocol.choke_heuristics.down.seed", torrent::Object(), rpc::make_target(download));

      rpc::call_command("d.connection_current.set",    conn_current, rpc::make_target(download));
      rpc::call_command("d.up.choke_heuristics.set",   choke_up, rpc::make_target(download));
      rpc::call_command("d.down.choke_heuristics.set", choke_down, rpc::make_target(download));

    } else {
      torrent::Object conn_current = rpc::call_command("d.connection_leech", torrent::Object(), rpc::make_target(download));
      torrent::Object choke_up     = rpc::call_command("d.up.choke_heuristics.leech", torrent::Object(), rpc::make_target(download));
      torrent::Object choke_down   = rpc::call_command("d.down.choke_heuristics.leech", torrent::Object(), rpc::make_target(download));

      if (conn_current.is_string_empty()) conn_current = rpc::call_command("protocol.connection.leech", torrent::Object(), rpc::make_target(download));
      if (choke_up.is_string_empty())     choke_up     = rpc::call_command("protocol.choke_heuristics.up.leech", torrent::Object(), rpc::make_target(download));
      if (choke_down.is_string_empty())   choke_down   = rpc::call_command("protocol.choke_heuristics.down.leech", torrent::Object(), rpc::make_target(download));

      rpc::call_command("d.connection_current.set",    conn_current, rpc::make_target(download));
      rpc::call_command("d.up.choke_heuristics.set",   choke_up, rpc::make_target(download));
      rpc::call_command("d.down.choke_heuristics.set", choke_down, rpc::make_target(download));

      // For the moment, clear the resume data so we force hash-check
      // on non-complete downloads after a crash. This shouldn't be
      // needed, but for some reason linux 2.6 is very lazy about
      // updating mtime.
      //
      // Disabling this due to the new resume code.
      //      torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"), true);
    }

    // If the DHT server is set to auto, start it now.
    if (!download->download()->info()->is_private())
      control->dht_manager()->auto_start();

    // Update the priority to ensure it has the correct
    // seeding/unfinished modifiers.
    download->set_priority(download->priority());
    download->download()->start(download->resume_flags());

    download->set_resume_flags(~uint32_t());

    DL_TRIGGER_EVENT(download, "event.download.resumed");

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not resume download: %s", e.what());
  }
}

void
DownloadList::pause(Download* download, int flags) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Pausing download: flags:%0x.", flags);

  try {

    download->set_resume_flags(~uint32_t());

    rpc::parse_command_single(rpc::make_target(download), "view.set_not_visible=active");

    // Always clear hashing on pause. When a hashing request is added,
    // it should have cleared the hash resume data.
    if (rpc::call_command_value("d.hashing", rpc::make_target(download)) != Download::variable_hashing_stopped) {
      download->download()->hash_stop();
      rpc::call_command_set_value("d.hashing.set", Download::variable_hashing_stopped, rpc::make_target(download));

      DL_TRIGGER_EVENT(download, "event.download.hash_removed");
    }

    if (!download->download()->info()->is_active())
      return;

    download->download()->stop(flags);
    torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));
    
    // TODO: This is actually for pause, not stop... And doesn't get
    // called when the download isn't active, but was in the 'started'
    // view.
    DL_TRIGGER_EVENT(download, "event.download.paused");

    rpc::call_command("d.state_changed.set", cachedTime.seconds(), rpc::make_target(download));
    rpc::call_command("d.state_counter.set", rpc::call_command_value("d.state_counter", rpc::make_target(download)), rpc::make_target(download));

    // If initial seeding is complete, don't try it again when restarting.
    if (download->is_done() &&
        rpc::call_command("d.connection_current", torrent::Object(), rpc::make_target(download)).as_string() == "initial_seed")
      rpc::call_command("d.connection_seed.set", rpc::call_command("d.connection_current", torrent::Object(), rpc::make_target(download)), rpc::make_target(download));

    // Save the state after all the slots, etc have been called so we
    // include the modifications they may make.
    //control->core()->download_store()->save(download);

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not pause download: %s", e.what());
  }
}

void
DownloadList::check_hash(Download* download) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Checking hash.");

  try {
    if (rpc::call_command_value("d.hashing", rpc::make_target(download)) != Download::variable_hashing_stopped)
      return;

    hash_queue(download, Download::variable_hashing_rehash);

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not check hash: %s", e.what());
  }
}

void
DownloadList::hash_done(Download* download) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Hash done.");

  if (download->is_hash_checking() || download->is_active())
    throw torrent::internal_error("DownloadList::hash_done(...) download in invalid state.");

  if (!download->is_hash_checked()) {
    download->set_hash_failed(true);
    
    DL_TRIGGER_EVENT(download, "event.download.hash_failed");
    return;
  }

  // Need to find some sane conditional here. Can we check the total
  // downloaded to ensure something was transferred, thus we didn't
  // just hash an already completed torrent with lacking session data?
  //
  // Perhaps we should use a seperate variable or state, and check
  // that. Thus we can bork the download if the hash check doesn't
  // confirm all the data, avoiding large BW usage on f.ex. the
  // ReiserFS bug with >4GB files.

  int64_t hashing = rpc::call_command_value("d.hashing", rpc::make_target(download));
  rpc::call_command_set_value("d.hashing.set", Download::variable_hashing_stopped, rpc::make_target(download));

  if (download->is_done() && download->download()->info()->is_meta_download())
    return process_meta_download(download);

  switch (hashing) {
  case Download::variable_hashing_initial:
  case Download::variable_hashing_rehash:
    // Normal re/hashing.

    // If the download was previously completed but the files were
    // f.ex deleted, then we clear the state and complete.
    if (rpc::call_command_value("d.complete", rpc::make_target(download)) && !download->is_done()) {
      rpc::call_command("d.state.set", (int64_t)0, rpc::make_target(download));
      download->set_message("Download registered as completed, but hash check returned unfinished chunks.");
    }

    // Save resume data so we update time-stamps and priorities if
    // they were invalid/changed while loading/hashing.
    rpc::call_command("d.complete.set", (int64_t)download->is_done(), rpc::make_target(download));
    torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));

    if (rpc::call_command_value("d.state", rpc::make_target(download)) == 1)
      resume(download, download->resume_flags());

    break;

  case Download::variable_hashing_last:

    if (download->is_done()) {
      confirm_finished(download);
    } else {
      download->set_message("Hash check on download completion found bad chunks, consider using \"safe_sync\".");
      lt_log_print(torrent::LOG_TORRENT_ERROR, "Hash check on download completion found bad chunks, consider using \"safe_sync\".");
      DL_TRIGGER_EVENT(download, "event.download.hash_final_failed");
    }

    // TODO: Should we skip the 'hash_done' event here?
    return;

  case Download::variable_hashing_stopped:
  default:
    // Either an error or someone wrote to the hashing variable...
    download->set_message("Hash check completed but the \"hashing\" variable is in an invalid state.");
    return;
  }

  DL_TRIGGER_EVENT(download, "event.download.hash_done");
}

void
DownloadList::hash_queue(Download* download, int type) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Hash queue.");

  if (rpc::call_command_value("d.hashing", rpc::make_target(download)) != Download::variable_hashing_stopped)
    throw torrent::internal_error("DownloadList::hash_queue(...) hashing already queued.");

  // HACK
  if (download->is_open()) {
    pause(download, torrent::Download::stop_skip_tracker);
    download->download()->close();

    DL_TRIGGER_EVENT(download, "event.download.hash_removed");
    DL_TRIGGER_EVENT(download, "event.download.closed");
  }

  torrent::resume_clear_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));

  download->set_hash_failed(false);
  rpc::call_command_set_value("d.hashing.set", type, rpc::make_target(download));

  if (download->is_open())
    throw torrent::internal_error("DownloadList::hash_clear(...) download still open.");

  // If any more stuff is added here, make sure resume etc are still
  // correct.
  DL_TRIGGER_EVENT(download, "event.download.hash_queued");
}

void
DownloadList::received_finished(Download* download) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Received finished.");

  if (rpc::call_command_value("pieces.hash.on_completion"))
    // Set some 'checking_finished_thingie' variable to make hash_done
    // trigger correctly, also so it can bork on missing data.
    hash_queue(download, Download::variable_hashing_last);
  else
    confirm_finished(download);
}

// The download must be open when we call this function.
void
DownloadList::confirm_finished(Download* download) {
  check_contains(download);

  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Confirming finished.");

  if (download->download()->info()->is_meta_download())
    return process_meta_download(download);

  rpc::call_command("d.complete.set", (int64_t)1, rpc::make_target(download));

  // Clean up these settings:
  torrent::Object conn_current = rpc::call_command("d.connection_seed", torrent::Object(), rpc::make_target(download));
  torrent::Object choke_up     = rpc::call_command("d.up.choke_heuristics.seed", torrent::Object(), rpc::make_target(download));
  torrent::Object choke_down   = rpc::call_command("d.down.choke_heuristics.seed", torrent::Object(), rpc::make_target(download));

  if (conn_current.is_string_empty()) conn_current = rpc::call_command("protocol.connection.seed", torrent::Object(), rpc::make_target(download));
  if (choke_up.is_string_empty())     choke_up     = rpc::call_command("protocol.choke_heuristics.up.seed", torrent::Object(), rpc::make_target(download));
  if (choke_down.is_string_empty())   choke_down   = rpc::call_command("protocol.choke_heuristics.down.seed", torrent::Object(), rpc::make_target(download));

  rpc::call_command("d.connection_current.set",    conn_current, rpc::make_target(download));
  rpc::call_command("d.up.choke_heuristics.set",   choke_up, rpc::make_target(download));
  rpc::call_command("d.down.choke_heuristics.set", choke_down, rpc::make_target(download));

  download->set_priority(download->priority());

  if (rpc::call_command_value("d.peers_min", rpc::make_target(download)) == rpc::call_command_value("throttle.min_peers.normal") &&
      rpc::call_command_value("throttle.min_peers.seed") >= 0)
    rpc::call_command("d.peers_min.set", rpc::call_command("throttle.min_peers.seed"), rpc::make_target(download));

  if (rpc::call_command_value("d.peers_max", rpc::make_target(download)) == rpc::call_command_value("throttle.max_peers.normal") &&
      rpc::call_command_value("throttle.max_peers.seed") >= 0)
    rpc::call_command("d.peers_max.set", rpc::call_command("throttle.max_peers.seed"), rpc::make_target(download));

  // Do this before the slots are called in case one of them closes
  // the download.
  //
  // Obsolete.
  if (!download->is_active() && rpc::call_command_value("session.on_completion") != 0) {
    //    torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));
    control->core()->download_store()->save_resume(download);
  }

  // Send the completed request before resuming so we don't reset the
  // up/downloaded baseline.
  download->download()->send_completed();

  // Save the hash in case the finished event erases it.
  torrent::HashString infohash = download->info()->hash();

  DL_TRIGGER_EVENT(download, "event.download.finished");

  if (find(infohash) == end())
    return;
      
//   if (download->resume_flags() != ~uint32_t())
//     throw torrent::internal_error("DownloadList::confirm_finished(...) download->resume_flags() != ~uint32_t().");

  // See #1292.
  //
  // Just reset the value for the moment. If a torrent finishes while
  // others are hashing, or some other situtation that causes resume
  // flag to change could cause the state to be invalid.
  //
  // TODO: Add a check when setting the flags to see if the torrent is
  // being hashed.
  download->set_resume_flags(~uint32_t());

  if (!download->is_active() && rpc::call_command_value("d.state", rpc::make_target(download)) == 1)
    resume(download,
           torrent::Download::start_no_create |
           torrent::Download::start_skip_tracker |
           torrent::Download::start_keep_baseline);
}

void
DownloadList::process_meta_download(Download* download) {
  lt_log_print_info(torrent::LOG_TORRENT_INFO, download->info(), "download_list", "Processing meta download.");

  rpc::call_command("d.stop", torrent::Object(), rpc::make_target(download));
  rpc::call_command("d.close", torrent::Object(), rpc::make_target(download));

  std::string metafile = (*download->file_list()->begin())->frozen_path();
  std::fstream file(metafile.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not read download metadata.");
    return;
  }

  torrent::Object* bencode = new torrent::Object(torrent::Object::create_map());
  file >> bencode->insert_key("info", torrent::Object());
  if (file.fail()) {
    delete bencode;
    lt_log_print(torrent::LOG_TORRENT_ERROR, "Could not create download, the input is not a valid torrent.");
    return;
  }
  file.close();

  // Steal the keys we still need. The old download has no use for them.
  bencode->insert_key("rtorrent_meta_download", torrent::Object()).swap(download->bencode()->get_key("rtorrent_meta_download"));
  if (download->bencode()->has_key("announce"))
    bencode->insert_key("announce", torrent::Object()).swap(download->bencode()->get_key("announce"));
  if (download->bencode()->has_key("announce-list"))
    bencode->insert_key("announce-list", torrent::Object()).swap(download->bencode()->get_key("announce-list"));

  erase_ptr(download);
  control->core()->try_create_download_from_meta_download(bencode, metafile);
}

}
