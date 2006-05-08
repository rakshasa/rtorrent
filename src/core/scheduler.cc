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
#include <rak/functional.h>

#include "download.h"
#include "download_list.h"
#include "scheduler.h"
#include "view.h"

namespace core {

// Change to unlimited.
Scheduler::Scheduler(DownloadList* dl) :
  m_view(NULL),
  m_downloadList(dl),

  m_maxActive(2),
  m_cycle(1) {
}

Scheduler::~Scheduler() {
}

void
Scheduler::set_view(View* view) {
  m_view = view;
}

Scheduler::size_type
Scheduler::active() const {
  return std::count_if(m_view->begin_visible(), m_view->end_visible(), std::mem_fun(&Download::is_active));
}

void
Scheduler::update() {
  size_type curActive = active();
  //  size_type curInactive = m_view->size() - curActive;

  // Hmm... Perhaps we should use a more complex sorting thingie.
  m_view->sort();

  // Just a hack for now, need to take into consideration how many
  // inactive we can switch with.
  size_type target = m_maxActive - std::min(m_cycle, m_maxActive);

  for (View::iterator itr = m_view->begin_visible(), last = m_view->end_visible(); curActive > target; ++itr) {
    if (itr == last)
      throw torrent::internal_error("Scheduler::update() loop bork.");

    if ((*itr)->is_active()) {
      m_downloadList->pause(*itr);
      --curActive;
    }      
  }

  m_view->sort();

  for (View::iterator itr = m_view->begin_visible(), last = m_view->end_visible(); curActive < m_maxActive; ++itr) {
    if (itr == last)
      throw torrent::internal_error("Scheduler::update() loop bork.");

    if (!(*itr)->is_active()) {
      m_downloadList->resume(*itr);
      ++curActive;
    }      
  }
}

}
