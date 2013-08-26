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

#ifndef RTORRENT_INPUT_PATH_INPUT_H
#define RTORRENT_INPUT_PATH_INPUT_H

#include <list>
#include <tr1/functional>

#include "utils/directory.h"
#include "text_input.h"

namespace input {

class PathInput : public TextInput {
public:
  typedef utils::Directory::iterator              directory_itr;
  typedef std::pair<directory_itr, directory_itr> range_type;

  typedef std::tr1::function<void ()>                             slot_void;
  typedef std::tr1::function<void (directory_itr, directory_itr)> slot_itr_itr;
  typedef std::list<slot_void>                                    signal_void;
  typedef std::list<slot_itr_itr>                                 signal_itr_itr;

  PathInput();
  virtual ~PathInput() {}

  signal_void&        signal_show_next()  { return m_signal_show_next; }
  signal_itr_itr&     signal_show_range() { return m_signal_show_range; }

  virtual bool        pressed(int key);

private:
  void                receive_do_complete();

  size_type           find_last_delim();
  range_type          find_incomplete(utils::Directory& d, const std::string& f);

  bool                m_showNext;

  signal_void         m_signal_show_next;
  signal_itr_itr      m_signal_show_range;
};

}

#endif
