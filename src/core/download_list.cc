// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <algorithm>
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "rak/functional.h"

#include "download.h"
#include "download_list.h"

namespace core {

DownloadList::iterator
DownloadList::insert(std::istream* str) {
  torrent::Download d = torrent::download_create(str);

  iterator itr = Base::insert(end(), new Download);
  (*itr)->set_download(d);
  (*itr)->get_download().signal_download_done(sigc::bind(sigc::mem_fun(*this, &DownloadList::finished), *itr));

  m_slotMapInsert.for_each(*itr);

  return itr;
}

DownloadList::iterator
DownloadList::erase(iterator itr) {
  m_slotMapErase.for_each(*itr);

  (*itr)->release_download();
  torrent::download_remove((*itr)->get_hash());
  delete *itr;

  return Base::erase(itr);
}

void
DownloadList::open(Download* d) {
  if (d->get_download().is_open())
    return;

  m_slotMapOpen.for_each(d);
}

void
DownloadList::close(Download* d) {
  if (!d->get_download().is_open())
    return;

  stop(d);
  m_slotMapClose.for_each(d);
}

void
DownloadList::start(Download* d) {
  if (d->get_download().is_active())
    return;

  open(d);
  m_slotMapStart.for_each(d);
}

void
DownloadList::stop(Download* d) {
  if (!d->get_download().is_active())
    return;

  m_slotMapStop.for_each(d);
}

void
DownloadList::clear() {
  std::for_each(begin(), end(), rak::call_delete<Download>());

  Base::clear();
}

void
DownloadList::finished(Download* d) {
  m_slotMapFinished.for_each(d);
}

}
