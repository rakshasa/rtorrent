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

#ifndef RTORRENT_DISPLAY_WINDOW_TEXT_H
#define RTORRENT_DISPLAY_WINDOW_TEXT_H

#include <vector>

#include "text_element.h"
#include "window.h"

namespace display {

class WindowText : public Window, public std::vector<TextElement*> {
public:
  typedef std::vector<TextElement*>   base_type;

  typedef base_type::value_type       value_type;
  typedef base_type::reference        reference;
  typedef base_type::iterator         iterator;
  typedef base_type::const_iterator   const_iterator;
  typedef base_type::reverse_iterator reverse_iterator;

  using base_type::empty;
  using base_type::size;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  WindowText(rpc::target_type target = rpc::make_target(), extent_type margin = 0);
  ~WindowText() { clear(); }

  void                clear();

  rpc::target_type    target() const                      { return m_target; }
  void                set_target(rpc::target_type target) { m_target = target; }

  uint32_t            interval() const         { return m_interval; }
  void                set_interval(uint32_t i) { m_interval = i; }

  void                push_back(TextElement* element);

  // Set an error handler if targets pointing to nullptr elements should
  // be handled separately to avoid throwing errors.
  void                set_error_handler(TextElement* element) { m_errorHandler = element; }

  virtual void        redraw();

private:
  rpc::target_type    m_target;
  TextElement*        m_errorHandler;

  extent_type         m_margin;
  uint32_t            m_interval;
};

}

#endif
