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

namespace core {

struct download_list_call {
  download_list_call(Download* d) : m_download(d) {}

  void operator () (const DownloadList::SlotMap::value_type& s) {
    s.second(m_download);
  }

  Download* m_download;
};    

DownloadList::iterator
DownloadList::insert(std::istream* str, bool printLog) {
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

    return end();
  }

  iterator itr = Base::insert(end(), new Download(download));

  try {
    (*itr)->download()->signal_download_done(sigc::bind(sigc::mem_fun(*this, &DownloadList::finished), *itr));
    std::for_each(m_slotMapInsert.begin(), m_slotMapInsert.end(), download_list_call(*itr));

  } catch (torrent::local_error& e) {
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

  return Base::erase(itr);
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
  d->variable()->set("state", "started");

  resume(d);
}

void
DownloadList::stop(Download* d) {
  d->variable()->set("state", "stopped");

  pause(d);
}

void
DownloadList::resume(Download* d) {
  try {
    if (!d->download()->is_open())
      std::for_each(m_slotMapOpen.begin(), m_slotMapOpen.end(), download_list_call(d));
      
    if (d->download()->is_hash_checked())
      std::for_each(m_slotMapStart.begin(), m_slotMapStart.end(), download_list_call(d));
    else
      // TODO: This can cause infinit looping?
      control->core()->hash_queue().insert(d, sigc::bind(sigc::mem_fun(*this, &DownloadList::resume), d));

    d->variable()->set("state_changed", cachedTime.seconds());

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::pause(Download* d) {
  try {

    if (d->download()->is_active())
      std::for_each(m_slotMapStop.begin(), m_slotMapStop.end(), download_list_call(d));

    d->variable()->set("state_changed", cachedTime.seconds());

  } catch (torrent::local_error& e) {
    control->core()->push_log(e.what());
  }
}

void
DownloadList::clear() {
  std::for_each(begin(), end(), rak::call_delete<Download>());

  Base::clear();
}

void
DownloadList::finished(Download* d) {
  std::for_each(m_slotMapFinished.begin(), m_slotMapFinished.end(), download_list_call(d));
}

}
