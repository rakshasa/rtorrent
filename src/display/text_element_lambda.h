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

#ifndef RTORRENT_DISPLAY_TEXT_ELEMENT_LAMBDA_H
#define RTORRENT_DISPLAY_TEXT_ELEMENT_LAMBDA_H

#include "text_element.h"

namespace display {

template <typename slot_type>
class TextElementBranchVoid : public TextElement {
public:
  typedef typename slot_type::result_type   result_type;
  
  TextElementBranchVoid(const slot_type& slot, TextElement* branch1, TextElement* branch2) :
    m_slot(slot), m_branch1(branch1), m_branch2(branch2) {}
  ~TextElementBranchVoid() { delete m_branch1; delete m_branch2; }

  virtual char* print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
    if (m_slot())
      return m_branch1 != NULL ? m_branch1->print(first, last, attributes, target) : first;
    else
      return m_branch2 != NULL ? m_branch2->print(first, last, attributes, target) : first;
  }

  virtual extent_type max_length() {
    return std::max(m_branch1 != NULL ? m_branch1->max_length() : 0,
                    m_branch2 != NULL ? m_branch2->max_length() : 0);
  }

private:
  slot_type           m_slot;

  TextElement*        m_branch1;
  TextElement*        m_branch2;
};

template <typename slot_type>
class TextElementBranch : public TextElement {
public:
  typedef typename slot_type::argument_type arg1_type;
  typedef typename slot_type::result_type   result_type;
  
  TextElementBranch(const slot_type& slot, TextElement* branch1, TextElement* branch2) :
    m_slot(slot), m_branch1(branch1), m_branch2(branch2) {}
  ~TextElementBranch() { delete m_branch1; delete m_branch2; }

  virtual char* print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
    if (target.second == NULL)
      return first;

    if (m_slot(reinterpret_cast<arg1_type>(target.second)))
      return m_branch1 != NULL ? m_branch1->print(first, last, attributes, target) : first;
    else
      return m_branch2 != NULL ? m_branch2->print(first, last, attributes, target) : first;
  }

  virtual extent_type max_length() {
    return std::max(m_branch1 != NULL ? m_branch1->max_length() : 0,
                    m_branch2 != NULL ? m_branch2->max_length() : 0);
  }

private:
  slot_type           m_slot;

  TextElement*        m_branch1;
  TextElement*        m_branch2;
};

template <typename slot_type>
class TextElementBranch3 : public TextElement {
public:
  typedef typename slot_type::argument_type arg1_type;
  typedef typename slot_type::result_type   result_type;
  
  TextElementBranch3(const slot_type& slot1, TextElement* branch1, const slot_type& slot2, TextElement* branch2, TextElement* branch3) :
    m_slot1(slot1), m_slot2(slot2), m_branch1(branch1), m_branch2(branch2), m_branch3(branch3) {}
  ~TextElementBranch3() { delete m_branch1; delete m_branch2; delete m_branch3; }

  virtual char* print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
    if (target.second == NULL)
      return first;

    if (m_slot1(reinterpret_cast<arg1_type>(target.second)))
      return m_branch1 != NULL ? m_branch1->print(first, last, attributes, target) : first;
    else if (m_slot2(reinterpret_cast<arg1_type>(target.second)))
      return m_branch2 != NULL ? m_branch2->print(first, last, attributes, target) : first;
    else
      return m_branch3 != NULL ? m_branch3->print(first, last, attributes, target) : first;
  }

  virtual extent_type max_length() {
    return std::max(m_branch1 != NULL ? m_branch1->max_length() : 0,
                    std::max(m_branch2 != NULL ? m_branch2->max_length() : 0,
                             m_branch3 != NULL ? m_branch3->max_length() : 0));
  }

private:
  slot_type           m_slot1;
  slot_type           m_slot2;

  TextElement*        m_branch1;
  TextElement*        m_branch2;
  TextElement*        m_branch3;
};

template <typename slot_type>
inline TextElementBranchVoid<slot_type>*
text_element_branch_void(const slot_type& slot, TextElement* branch1, TextElement* branch2) {
  return new TextElementBranchVoid<slot_type>(slot, branch1, branch2);
}

template <typename slot_type>
inline TextElementBranch<slot_type>*
text_element_branch(const slot_type& slot, TextElement* branch1, TextElement* branch2) {
  return new TextElementBranch<slot_type>(slot, branch1, branch2);
}

template <typename slot_type>
inline TextElementBranch3<slot_type>*
text_element_branch3(const slot_type& slot1, TextElement* branch1, const slot_type& slot2, TextElement* branch2, TextElement* branch3) {
  return new TextElementBranch3<slot_type>(slot1, branch1, slot2, branch2, branch3);
}

}

#endif
