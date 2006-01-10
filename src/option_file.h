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

#ifndef RTORRENT_OPTION_FILE_H
#define RTORRENT_OPTION_FILE_H

#include <string>
#include <sigc++/slot.h>

class OptionFile {
public:
  static const int max_size_key = 128;
  static const int max_size_opt = 1024;
  static const int max_size_line = max_size_key + max_size_opt + 64;

  typedef sigc::slot2<void, const std::string&, const std::string&> SlotStringPair;
  
  // Returns false when the file doesn't exist or cannot be opened.
  bool                process_file(const std::string& filename);

  void                slot_option(const SlotStringPair& s) { m_slotOption = s; }

private:
  void                parse_line(const char* line);

  static char*        fill_buffer(int fd, char* buffer, char* first, char* last);

  SlotStringPair      m_slotOption;
};

#endif
