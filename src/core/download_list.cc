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

#include <algorithm>
#include <iostream>
#include <sigc++/bind.h>
#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/download.h>
#include <torrent/hash_string.h>
#include <torrent/object.h>
#include <torrent/object_stream.h>
#include <torrent/resume.h>
#include <torrent/torrent.h>

#include "rpc/parse_commands.h"

#include "control.h"
#include "globals.h"
#include "manager.h"

#include "download.h"
#include "download_list.h"
#include "download_store.h"

namespace core {

inline void
DownloadList::check_contains(Download* d) {
#ifdef USE_EXTRA_DEBUG
  if (std::find(begin(), end(), d) == end())
    throw torrent::internal_error("DownloadList::check_contains(...) failed.");
#endif
}

struct download_list_call {
  download_list_call(Download* d) : m_download(d) {}

  void operator () (const DownloadList::slot_map::value_type& s) {
    s.second(m_download);
  }

  Download* m_download;
};    

void
DownloadList::clear() {
  std::for_each(begin(), end(), std::bind1st(std::mem_fun(&DownloadList::close), this));
  std::for_each(begin(), end(), rak::call_delete<Download>());

  base_type::clear();
}

void
DownloadList::session_save() {
  std::for_each(begin(), end(), std::bind1st(std::mem_fun(&DownloadStore::save), control->core()->download_store()));
}

DownloadList::iterator
DownloadList::find(const torrent::HashString& hash) {
  return std::find_if(begin(), end(), rak::equal(hash, rak::on(std::mem_fun(&Download::download), std::mem_fun(&torrent::Download::info_hash))));
}

DownloadList::iterator
DownloadList::find_hex(const char* hash) {
  torrent::HashString key;

  for (torrent::HashString::iterator itr = key.begin(), last = key.end(); itr != last; itr++, hash += 2)
    *itr = (rak::hexchar_to_value(*hash) << 4) + rak::hexchar_to_value(*(hash + 1));

  return std::find_if(begin(), end(), rak::equal(key, rak::on(std::mem_fun(&Download::download), std::mem_fun(&torrent::Download::info_hash))));
}

Download*
DownloadList::find_hex_ptr(const char* hash) {
  iterator itr = find_hex(hash);

  return itr != end() ? *itr : NULL;
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
        control->core()->push_log("Could not create download, the input is not a valid torrent.");

      return NULL;
    }

    download = torrent::download_add(object);

  } catch (torrent::local_error& e) {
    delete object;

    if (printLog)
      control->core()->push_log(e.what());

    return NULL;
  }

  // There's no non-critical exceptions that should be throwable by
  // the ctor, so don't catch.
  return new Download(download);
}

DownloadList::iterator
DownloadList::insert(Download* download) {
  iterator itr = base_type::insert(end(), download);

  try {
    (*itr)->download()->signal_download_done(sigc::bind(sigc::mem_fun(*this, &DownloadList::received_finished), download));
    (*itr)->download()->signal_hash_done(sigc::bind(sigc::mem_fun(*this, &DownloadList::hash_done), download));

    std::for_each(slot_map_insert().begin(), slot_map_insert().end(), download_list_call(*itr));

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

  // Makes sure close doesn't restart hashing of this download.
  (*itr)->set_hash_failed(true);

  close(*itr);

  control->core()->download_store()->remove(*itr);

  std::for_each(slot_map_erase().begin(), slot_map_erase().end(), download_list_call(*itr));

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
    control->core()->push_log(e.what());
    return false;
  }
}

void
DownloadList::open_throw(Download* download) {
  check_contains(download);

  if (download->download()->is_open())
    return;
  
  download->download()->open();

  std::for_each(slot_map_open().begin(), slot_map_open().end(), download_list_call(download));
}

void
DownloadList::close(Download* download) {
  try {

    close_throw(download);

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::close_quick(Download* download) {
  close(download);
  
  // Make sure we cancel any tracker requests. This should rather be
  // handled by some parameter to the close function, or some other
  // way of giving the client more control of when STOPPED requests
  // are sent.
  download->download()->tracker_list().manual_cancel();
}

void
DownloadList::close_throw(Download* download) {
  check_contains(download);

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

  if (!download->is_hash_failed() && rpc::call_command_d_value("get_d_hashing", download) != Download::variable_hashing_stopped)
    throw torrent::internal_error("DownloadList::close_throw(...) called but we're going into a hashing loop.");

  std::for_each(slot_map_hash_removed().begin(), slot_map_hash_removed().end(), download_list_call(download));
  std::for_each(slot_map_close().begin(), slot_map_close().end(), download_list_call(download));
}

void
DownloadList::start_normal(Download* download) {
  check_contains(download);

  // Clear hash failed as we're doing a manual start and want to try
  // hashing again.
  download->set_hash_failed(false);
  rpc::call_command_d("set_d_state", download, (int64_t)1);

  resume(download);
}

bool
DownloadList::start_try(Download* download) {
  check_contains(download);

  // Also don't start if the state is one of those that indicate we
  // were manually stopped?

  if (download->is_hash_failed() || rpc::call_command_d_value("get_d_ignore_commands", download) != 0)
    return false;

  // Don't clear the hash failed as this function is used by scripts,
  // etc.
  rpc::call_command_d("set_d_state", download, (int64_t)1);

  resume(download);
  return true;
}

void
DownloadList::stop_normal(Download* download) {
  check_contains(download);

  rpc::call_command_d("set_d_state", download, (int64_t)0);

  pause(download);
}

bool
DownloadList::stop_try(Download* download) {
  check_contains(download);

  if (rpc::call_command_d_value("get_d_ignore_commands", download) != 0)
    return false;

  rpc::call_command_d("set_d_state", download, (int64_t)0);

  pause(download);
  return true;
}

void
DownloadList::resume(Download* download) {
  check_contains(download);

  try {

    if (download->download()->is_active())
      return;

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

      if (rpc::call_command_d_value("get_d_hashing", download) == Download::variable_hashing_stopped)
        rpc::call_command_d("set_d_hashing", download, Download::variable_hashing_initial);

      std::for_each(slot_map_hash_queued().begin(), slot_map_hash_queued().end(), download_list_call(download));
      return;
    }

    // This will never actually do anything due to the above hash check.
    // open_throw(download);

    rpc::call_command_d("set_d_state_changed", download, cachedTime.seconds());

    if (download->is_done()) {
      rpc::call_command_d("set_d_connection_current", download, rpc::call_command_d_void("get_d_connection_seed", download));
    } else {
      rpc::call_command_d("set_d_connection_current", download, rpc::call_command_d_void("get_d_connection_leech", download));

      // For the moment, clear the resume data so we force hash-check
      // on non-complete downloads after a crash. This shouldn't be
      // needed, but for some reason linux 2.6 is very lazy about
      // updating mtime.
      torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"), true);
    }

    // Update the priority to ensure it has the correct
    // seeding/unfinished modifiers.
    download->set_priority(download->priority());
    download->download()->start();

    std::for_each(slot_map_start().begin(), slot_map_start().end(), download_list_call(download));

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::pause(Download* download) {
  check_contains(download);

  try {

    // Always clear hashing on pause. When a hashing request is added,
    // it should have cleared the hash resume data.
    if (rpc::call_command_d_value("get_d_hashing", download) != Download::variable_hashing_stopped) {
      download->download()->hash_stop();
      rpc::call_command_d_set_value("set_d_hashing", download, Download::variable_hashing_stopped);

      std::for_each(slot_map_hash_removed().begin(), slot_map_hash_removed().end(), download_list_call(download));
    }

    if (!download->download()->is_active())
      return;

    download->download()->stop();
    torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));
    
    std::for_each(slot_map_stop().begin(), slot_map_stop().end(), download_list_call(download));

    rpc::call_command_d("set_d_state_changed", download, cachedTime.seconds());

    // Save the state after all the slots, etc have been called so we
    // include the modifications they may make.
    //control->core()->download_store()->save(download);

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::check_hash(Download* download) {
  check_contains(download);

  try {

    if (rpc::call_command_d_value("get_d_hashing", download) != Download::variable_hashing_stopped)
      return;

    hash_queue(download, Download::variable_hashing_rehash);

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::hash_done(Download* download) {
  check_contains(download);

  if (download->is_hash_checking() || download->is_active())
    throw torrent::internal_error("DownloadList::hash_done(...) download in invalid state.");

  if (!download->is_hash_checked()) {
    download->set_hash_failed(true);
    
    std::for_each(slot_map_hash_done().begin(), slot_map_hash_done().end(), download_list_call(download));
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

  int64_t hashing = rpc::call_command_d_value("get_d_hashing", download);
  rpc::call_command_d_set_value("set_d_hashing", download, Download::variable_hashing_stopped);

  switch (hashing) {
  case Download::variable_hashing_initial:
  case Download::variable_hashing_rehash:
    // Normal re/hashing.

    // If the download was previously completed but the files were
    // f.ex deleted, then we clear the state and complete.
    if (rpc::call_command_d_value("get_d_complete", download) && !download->is_done()) {
      rpc::call_command_d("set_d_state", download, (int64_t)0);
      download->set_message("Download registered as completed, but hash check returned unfinished chunks.");
    }

    // Save resume data so we update time-stamps and priorities if
    // they were invalid/changed while loading/hashing.
    rpc::call_command_d("set_d_complete", download, (int64_t)download->is_done());
    torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));

    if (rpc::call_command_d_value("get_d_state", download) == 1)
      resume(download);

    break;

  case Download::variable_hashing_last:

    if (download->is_done()) {
      confirm_finished(download);
    } else {
      download->set_message("Hash check on download completion found bad chunks, consider using \"safe_sync\".");
      control->core()->push_log("Hash check on download completion found bad chunks, consider using \"safe_sync\".");
    }
    
    break;

  case Download::variable_hashing_stopped:
  default:
    // Either an error or someone wrote to the hashing variable...
    download->set_message("Hash check completed but the \"hashing\" variable is in an invalid state.");
    return;
  }

  std::for_each(slot_map_hash_done().begin(), slot_map_hash_done().end(), download_list_call(download));
}

void
DownloadList::hash_queue(Download* download, int type) {
  check_contains(download);

  if (rpc::call_command_d_value("get_d_hashing", download) != Download::variable_hashing_stopped)
    throw torrent::internal_error("DownloadList::hash_queue(...) hashing already queued.");

  close_throw(download);
  torrent::resume_clear_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));

  download->set_hash_failed(false);
  rpc::call_command_d_set_value("set_d_hashing", download, type);

  if (download->is_open())
    throw torrent::internal_error("DownloadList::hash_clear(...) download still open.");

  // If any more stuff is added here, make sure resume etc are still
  // correct.
  std::for_each(slot_map_hash_queued().begin(), slot_map_hash_queued().end(), download_list_call(download));
}

void
DownloadList::received_finished(Download* download) {
  check_contains(download);

  if (rpc::call_command_value("get_check_hash")) {
    // Set some 'checking_finished_thingie' variable to make hash_done
    // trigger correctly, also so it can bork on missing data.
    hash_queue(download, Download::variable_hashing_last);

  } else {
    confirm_finished(download);
  }
}

// The download must be open when we call this function.
void
DownloadList::confirm_finished(Download* download) {
  check_contains(download);

  rpc::call_command_d("set_d_complete", download, (int64_t)1);

  rpc::call_command_d("set_d_connection_current", download, rpc::call_command_d_void("get_d_connection_seed", download));
  download->set_priority(download->priority());

  if (rpc::call_command_d_value("get_d_min_peers", download) == rpc::call_command_value("get_min_peers") && rpc::call_command_value("get_min_peers_seed") >= 0)
    rpc::call_command_d("set_d_min_peers", download, rpc::call_command_void("get_min_peers_seed"));

  if (rpc::call_command_d_value("get_d_max_peers", download) == rpc::call_command_value("get_max_peers") && rpc::call_command_value("get_max_peers_seed") >= 0)
    rpc::call_command_d("set_d_max_peers", download, rpc::call_command_void("get_max_peers_seed"));

  // Do this before the slots are called in case one of them closes
  // the download.
  if (!download->is_active() && rpc::call_command_value("get_session_on_completion") != 0) {
    torrent::resume_save_progress(*download->download(), download->download()->bencode()->get_key("libtorrent_resume"));
    control->core()->download_store()->save(download);
  }

  // Send the completed request before resuming so we don't reset the
  // up/downloaded baseline.
  download->download()->tracker_list().send_completed();

  std::for_each(slot_map_finished().begin(), slot_map_finished().end(), download_list_call(download));

  if (!download->is_active() && rpc::call_command_d_value("get_d_state", download) == 1)
    resume(download);
}

}
