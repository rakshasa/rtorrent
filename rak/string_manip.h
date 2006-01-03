// rak - Rakshasa's toolbox
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

#ifndef RAK_STRING_MANIP_H
#define RAK_STRING_MANIP_H

#include <algorithm>
#include <cctype>

namespace rak {

// Use these trim functions until n1872 is widely supported.

template <typename Sequence>
Sequence trim_begin(const Sequence& seq) {
  if (seq.empty() || !std::isspace(*seq.begin()))
    return seq;

  typename Sequence::size_type pos = 0;

  while (pos != seq.length() && std::isspace(seq[pos]))
    pos++;

  return seq.substr(pos, seq.length() - pos);
}

template <typename Sequence>
Sequence trim_end(const Sequence& seq) {
  if (seq.empty() || !std::isspace(*(--seq.end())))
    return seq;

  typename Sequence::size_type pos = seq.size();

  while (pos != 0 && std::isspace(seq[pos - 1]))
    pos--;

  return seq.substr(0, pos);
}

template <typename Sequence>
Sequence trim(const Sequence& seq) {
  return trim_begin(trim_end(seq));
}

// Consider rewritting such that m_seq is replaced by first/last.
template <typename Sequence>
class split_iterator_t {
public:
  typedef typename Sequence::const_iterator const_iterator;
  typedef typename Sequence::value_type     value_type;

  split_iterator_t() {}

  split_iterator_t(const Sequence& seq, value_type delim) :
    m_seq(&seq),
    m_delim(delim),
    m_pos(seq.begin()),
    m_next(std::find(seq.begin(), seq.end(), delim)) {
  }

  Sequence operator * () { return Sequence(m_pos, m_next); }

  split_iterator_t& operator ++ () {
    m_pos = m_next;

    if (m_pos == m_seq->end())
      return *this;

    m_pos++;
    m_next = std::find(m_pos, m_seq->end(), m_delim);

    return *this;
  }

  bool operator == (const split_iterator_t& itr) const { return m_pos == m_seq->end(); }
  bool operator != (const split_iterator_t& itr) const { return m_pos != m_seq->end(); }

private:
  const Sequence* m_seq;
  value_type      m_delim;
  const_iterator  m_pos;
  const_iterator  m_next;
};

template <typename Sequence>
inline split_iterator_t<Sequence>
split_iterator(const Sequence& seq, typename Sequence::value_type delim) {
  return split_iterator_t<Sequence>(seq, delim);
}

template <typename Sequence>
inline split_iterator_t<Sequence>
split_iterator(const Sequence& seq) {
  return split_iterator_t<Sequence>();
}

}

#endif
