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
#include "hash_queue.h"
#include "manager.h"

#include "download.h"
#include "download_list.h"
#include "download_store.h"

namespace core {

struct download_list_call {
  download_list_call(Download* d) : m_download(d) {}

  void operator () (const DownloadList::slot_map::value_type& s) {
    s.second(m_download);
  }

  Download* m_download;
};    

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
DownloadList::insert(Download* d) {
  iterator itr = base_type::insert(end(), d);

  try {
    (*itr)->download()->signal_download_done(sigc::bind(sigc::mem_fun(*this, &DownloadList::received_finished), d));
    std::for_each(m_slotMapInsert.begin(), m_slotMapInsert.end(), download_list_call(*itr));

  } catch (torrent::local_error& e) {
    // Should perhaps relax this, just print an error and remove the
    // downloads?
    throw torrent::internal_error("Caught during DownloadList::insert part 2: " + std::string(e.what()));
  }

  return itr;
}

void
DownloadList::erase(Download* d) {
  erase(std::find(begin(), end(), d));
}

DownloadList::iterator
DownloadList::erase(iterator itr) {
  if (itr == end())
    throw torrent::internal_error("DownloadList::erase(...) could not find download.");

  // Make safe to erase active downloads.
  if ((*itr)->download()->is_active())
    throw std::logic_error("DownloadList::erase(...) called on an active download.");

  std::for_each(m_slotMapErase.begin(), m_slotMapErase.end(), download_list_call(*itr));

  torrent::download_remove(*(*itr)->download());
  delete *itr;

  return base_type::erase(itr);
}

void
DownloadList::open(Download* d) {
  try {

    if (!d->download()->is_open())
      std::for_each(m_slotMapOpen.begin(), m_slotMapOpen.end(), download_list_call(d));

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::close(Download* d) {
  try {

    if (d->download()->is_active())
      std::for_each(m_slotMapStop.begin(), m_slotMapStop.end(), download_list_call(d));

    if (d->download()->is_open())
      std::for_each(m_slotMapClose.begin(), m_slotMapClose.end(), download_list_call(d));

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::start(Download* d) {
  d->variable()->set("state", (int64_t)1);

  resume(d);
}

void
DownloadList::stop(Download* d) {
  d->variable()->set("state", (int64_t)0);

  pause(d);
}

void
DownloadList::resume(Download* download) {
  try {

    if (!download->download()->is_open())
      std::for_each(m_slotMapOpen.begin(), m_slotMapOpen.end(), download_list_call(download));

    if (download->download()->is_hash_checked()) {

      if (download->is_done())
	download->set_connection_type(download->variable()->get_string("connection_seed"));
      else
	download->set_connection_type(download->variable()->get_string("connection_leech"));

      // Update the priority to ensure it has the correct
      // seeding/unfinished modifiers.
      download->set_priority(download->priority());
      download->download()->start();

      std::for_each(m_slotMapStart.begin(), m_slotMapStart.end(), download_list_call(download));

    } else {
      // TODO: This can cause infinit looping?
      control->core()->hash_queue()->insert(download);
    }

    download->variable()->set("state_changed", cachedTime.seconds());

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::pause(Download* download) {
  try {

    download->download()->stop();
    download->download()->hash_resume_save();
    
    if (download->download()->is_active())
      std::for_each(m_slotMapStop.begin(), m_slotMapStop.end(), download_list_call(download));

    download->variable()->set("state_changed", cachedTime.seconds());

    // Save the state after all the slots, etc have been called so we
    // include the modifications they may make.
    control->core()->download_store().save(download);

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::clear() {
  std::for_each(begin(), end(), rak::call_delete<Download>());

  base_type::clear();
}

void
DownloadList::check_hash(Download* d) {
  close(d);
  d->download()->hash_resume_clear();
  open(d);

  control->core()->hash_queue()->insert(d);
}

void
DownloadList::hash_done(Download* download) {
  if (!download->download()->is_hash_checked() || download->download()->is_hash_checking())
    throw torrent::internal_error("DownloadList::hash_done(...) download in invalid state.");

  // Need to find some sane conditional here. Can we check the total
  // downloaded to ensure something was transferred, thus we didn't
  // just hash an already completed torrent with lacking session data?
  //
  // Perhaps we should use a seperate variable or state, and check
  // that. Thus we can bork the download if the hash check doesn't
  // confirm all the data, avoiding large BW usage on f.ex. the
  // ReiserFS bug with >4GB files.

  // Use just is_done(), have another if statement inside.
  if (download->is_done() && download->variable()->get_value("complete") == 0) { 

    if (control->variable()->get_value("session_on_completion"))
      control->core()->download_store().save(download);

    // Send a "truly finished message from here.
    confirm_finished(download);
  }

  if (download->variable()->get_value("state") == 1)
    resume(download);
}

void
DownloadList::received_finished(Download* download) {
  if (control->variable()->get_value("check_hash")) {
    // Set some 'checking_finished_thingie' variable to make hash_done
    // trigger correctly, also so it can bork on missing data.

    check_hash(download);

  } else {
    confirm_finished(download);
  }
}

void
DownloadList::confirm_finished(Download* download) {
  // FIXME
  //torrent::download_set_priority(m_download, 2);

  download->variable()->set("complete", (int64_t)1);
  download->set_connection_type(download->variable()->get_string("connection_seed"));

  download->download()->tracker_list().send_completed();

  std::for_each(m_slotMapFinished.begin(), m_slotMapFinished.end(), download_list_call(download));
}

}
