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

#include "log.h"
#include "rak/functional.h"

namespace core {

void
Log::push_front(const std::string& msg) {
  Base::push_front(Type(utils::Timer::cache(), msg));

  if (size() > 50)
    Base::pop_back();

  m_signalUpdate.emit();
}

Log::iterator
Log::find_older(utils::Timer t) {
  return std::find_if(begin(), end(), rak::on(rak::mem_ptr_ref(&Type::first), std::bind2nd(std::less_equal<utils::Timer>(), t)));
}

}
