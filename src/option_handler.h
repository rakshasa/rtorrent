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

#ifndef RTORRENT_OPTION_HANDLER_H
#define RTORRENT_OPTION_HANDLER_H

#include <map>
#include <string>

// No members with dtor's allowed.
struct OptionHandlerBase {
  virtual ~OptionHandlerBase() {}

  virtual void process(const std::string& key, const std::string& arg) = 0;
};

class OptionHandler : private std::map<std::string, OptionHandlerBase*> {
public:
  typedef std::map<std::string, OptionHandlerBase*> Base;
  typedef Base::value_type                          value_type;
  typedef Base::size_type                           size_type;

  typedef Base::iterator                            iterator;
  typedef Base::reverse_iterator                    reverse_iterator;
  typedef Base::const_iterator                      const_iterator;
  typedef Base::const_reverse_iterator              const_reverse_iterator;

  using Base::empty;
  using Base::size;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::find;

  OptionHandler() {}
  ~OptionHandler() { clear(); }

  // We take over ownership of opt.
  void                insert(const std::string& key, OptionHandlerBase* opt);
  void                erase(const std::string& key);

  void                clear();

  void                process(const std::string& key, const std::string& arg) const;
};

#endif
