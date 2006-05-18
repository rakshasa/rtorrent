// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/object_stream.h>
#include <torrent/torrent.h>

#include "rak/functional.h"

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
    throw torrent::client_error("DownloadList::check_contains(...) failed.");
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

Download*
DownloadList::create(std::istream* str, bool printLog) {
  torrent::Object* object = new torrent::Object;
  torrent::Download download;

  try {
    *str >> *object;
    
    // Catch, delete.
    if (str->fail())
      throw torrent::input_error("Could not create download, the input is not a valid torrent.");

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
    throw torrent::client_error("Caught during DownloadList::insert(...): " + std::string(e.what()));
  }

  return itr;
}

void
DownloadList::erase(Download* download) {
  check_contains(download);

  erase(std::find(begin(), end(), download));
}

DownloadList::iterator
DownloadList::erase(iterator itr) {
  if (itr == end())
    throw torrent::client_error("DownloadList::erase(...) could not find download.");

  // Makes sure close doesn't restart hashing of this download.
  (*itr)->set_hash_failed(true);

  close(*itr);

  control->core()->download_store()->remove(*itr);

  std::for_each(slot_map_erase().begin(), slot_map_erase().end(), download_list_call(*itr));

  torrent::download_remove(*(*itr)->download());
  delete *itr;

  return base_type::erase(itr);
}

// void
// DownloadList::save(Download* d) {
  
// }

void
DownloadList::open(Download* download) {
  try {

    open_throw(download);

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
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
DownloadList::close_throw(Download* download) {
  check_contains(download);

  if (!download->is_open())
    return;

  // When pause gets called it will clear the initial hash checking. 

  if (download->is_active())
    pause(download);
  
  // Save the torrent on close, this covers shutdown and if a torrent
  // is manually closed which would clear the progress data. For
  // better crash protection, save regulary in addition to this.
  //
  // Used to be in pause, but this was wrong for rehashing etc.
  //
  // Reconsider this save. Should be done explicitly when shutting down.
  //control->core()->download_store()->save(download);

  // FIXME: Urgh, need to do something sane when closing a download
  // that has been queued for hashing. ATM just close the torrent and
  // call the hash_removed slots. This might open and restart the
  // hashing of this torrent.

//   if (download->is_hash_checking()) {
//     download->download()->close();

//     // Hash removed slot must be called after close as we can't atm
//     // stop already started hash checks except through close.
//     std::for_each(slot_map_hash_removed().begin(), slot_map_hash_removed().end(), download_list_call(download));

//   } else {
//     download->download()->close();
//   }

  // But this isn't correct either, i think... ATM do the borking
  // thing, the hash queue won't be updated. Need to properly handle
  // erasing a download.

  // Propably should do its own thingie in erase rather than calling
  // close.

  download->download()->close();

  std::for_each(slot_map_hash_removed().begin(), slot_map_hash_removed().end(), download_list_call(download));
  std::for_each(slot_map_close().begin(), slot_map_close().end(), download_list_call(download));
}

void
DownloadList::start(Download* download) {
  check_contains(download);

  download->variable()->set("state", (int64_t)1);

  resume(download);
}

void
DownloadList::stop(Download* download) {
  check_contains(download);

  download->variable()->set("state", (int64_t)0);

  pause(download);
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
      download->set_hash_failed(false);

      // Set 'hashing' to started if hashing wasn't started, else keep
      // the old value.
      if (download->variable()->get_value("hashing") == Download::variable_hashing_stopped)
	download->variable()->set("hashing", Download::variable_hashing_initial);

      std::for_each(slot_map_hash_queued().begin(), slot_map_hash_queued().end(), download_list_call(download));
      return;
    }

    download->variable()->set("state_changed", cachedTime.seconds());

    open_throw(download);

    if (download->is_done()) {
      download->set_connection_type(download->variable()->get_string("connection_seed"));
    } else {
      download->set_connection_type(download->variable()->get_string("connection_leech"));

      // For the moment, clear the resume data so we force hash-check
      // on non-complete downloads after a crash. This shouldn't be
      // needed, but for some reason linux 2.6 is very lazy about
      // updating mtime.
      download->download()->hash_resume_clear();
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

    // Don't stop if we're doing the final hashing.
    if (download->variable()->get_value("hashing") == Download::variable_hashing_initial) {
      download->variable()->set("hashing", Download::variable_hashing_stopped);

      std::for_each(slot_map_hash_removed().begin(), slot_map_hash_removed().end(), download_list_call(download));
    }

    if (!download->download()->is_active())
      return;

    download->download()->stop();
    download->download()->hash_resume_save();
    
    std::for_each(slot_map_stop().begin(), slot_map_stop().end(), download_list_call(download));

    download->variable()->set("state_changed", cachedTime.seconds());

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

    download->variable()->set("hashing", Download::variable_hashing_rehash);
    hash_clear(download);

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::hash_done(Download* download) {
  check_contains(download);

  if (!download->is_hash_checked() || download->is_hash_checking() || download->is_active())
    throw torrent::client_error("DownloadList::hash_done(...) download in invalid state.");

  // Need to find some sane conditional here. Can we check the total
  // downloaded to ensure something was transferred, thus we didn't
  // just hash an already completed torrent with lacking session data?
  //
  // Perhaps we should use a seperate variable or state, and check
  // that. Thus we can bork the download if the hash check doesn't
  // confirm all the data, avoiding large BW usage on f.ex. the
  // ReiserFS bug with >4GB files.

  int64_t hashing = download->variable()->get_value("hashing");
  download->variable()->set("hashing", Download::variable_hashing_stopped);

  switch (hashing) {
  case Download::variable_hashing_initial:
  case Download::variable_hashing_rehash:
    // Normal re/hashing.

    if (download->is_done())
      download->variable()->set("complete", (int64_t)1);
    
    // Save resume data so we update time-stamps and priorities if
    // they were invalid/changed when loading.
    download->download()->hash_resume_save();

    if (download->variable()->get_value("state") == 1)
      resume(download);

    break;

  case Download::variable_hashing_last:

    if (download->is_done())
      confirm_finished(download);
    else
      download->set_message("Hash check on download completion found bad chunks.");
    
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
DownloadList::hash_clear(Download* download) {
  check_contains(download);

  // Set hash failed so close_throw won't cause the hash queue to
  // reopen this download.
  download->set_hash_failed(true);

  close_throw(download);
  download->download()->hash_resume_clear();

  download->set_hash_failed(false);

  if (download->is_open())
    throw torrent::internal_error("DownloadList::hash_clear(...) download still open.");

  // If any more stuff is added here, make sure resume etc are still
  // correct.
  std::for_each(slot_map_hash_queued().begin(), slot_map_hash_queued().end(), download_list_call(download));
}

void
DownloadList::received_finished(Download* download) {
  check_contains(download);

  if (control->variable()->get_value("check_hash")) {
    // Set some 'checking_finished_thingie' variable to make hash_done
    // trigger correctly, also so it can bork on missing data.

    download->variable()->set("hashing", Download::variable_hashing_last);
    hash_clear(download);

  } else {
    confirm_finished(download);
  }
}

// The download must be open when we call this function.
void
DownloadList::confirm_finished(Download* download) {
  check_contains(download);

  download->variable()->set("complete", (int64_t)1);

  download->set_connection_type(download->variable()->get_string("connection_seed"));
  download->set_priority(download->priority());

  // Do this before the slots are called in case one of them closes
  // the download.
  if (!download->is_active() && control->variable()->get_value("session_on_completion") == 1) {
    download->download()->hash_resume_save();
    control->core()->download_store()->save(download);
  }

  std::for_each(slot_map_finished().begin(), slot_map_finished().end(), download_list_call(download));

  if (!download->is_active() && download->variable()->get_value("state") == 1)
    resume(download);

  download->download()->tracker_list().send_completed();
}

}
