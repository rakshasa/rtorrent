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

#ifndef RTORRENT_INPUT_PATH_INPUT_H
#define RTORRENT_INPUT_PATH_INPUT_H

#include <sigc++/signal.h>

#include "utils/directory.h"
#include "text_input.h"

namespace input {

class PathInput : public TextInput {
public:
  typedef std::pair<utils::Directory::iterator, utils::Directory::iterator>           Range;
  typedef sigc::signal0<void>                                                         Signal;
  typedef sigc::signal2<void, utils::Directory::iterator, utils::Directory::iterator> SignalShowRange;

  PathInput();
  virtual ~PathInput() {}

  Signal&             signal_show_next()          { return m_signalShowNext; }
  SignalShowRange&    signal_show_range()         { return m_signalShowRange; }

  virtual bool        pressed(int key);

private:
  void                receive_do_complete();

  size_type           find_last_delim();
  Range               find_incomplete(utils::Directory& d, const std::string& f);

  bool                m_showNext;

  Signal              m_signalShowNext;
  SignalShowRange     m_signalShowRange;
};

}

#endif
