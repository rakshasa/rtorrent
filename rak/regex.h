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

// This is a hacked up whole string pattern matching. Replace with
// TR1's regex when that becomes widely available. It is intended for
// small strings.

#ifndef RAK_REGEX_H
#define RAK_REGEX_H

#include <sys/types.h>

#include <functional>
#include <string>
#include <list>

namespace rak {

class regex : public std::unary_function<std::string, bool> {
public:
  regex() {}
  regex(const std::string& p) : m_pattern(p) {}

  const std::string& pattern() const { return m_pattern; }

  bool operator () (const std::string& p) const;

private:
  std::string m_pattern;
};

// This isn't optimized, or very clean. A simple hack that should work.
bool
regex::operator () (const std::string& text) const {
  if (m_pattern.empty() ||
      text.empty() ||
      (m_pattern[0] != '*' && m_pattern[0] != text[0]))
    return false;

  // Replace with unordered_vector?
  std::list<unsigned int> paths;
  paths.push_front(0);

  for (std::string::const_iterator itrText = ++text.begin(), lastText = text.end(); itrText != lastText; ++itrText) {
    
    for (std::list<unsigned int>::iterator itrPaths = paths.begin(), lastPaths = paths.end(); itrPaths != lastPaths; ) {

      unsigned int next = *itrPaths + 1;

      if (m_pattern[*itrPaths] != '*')
	itrPaths = paths.erase(itrPaths);
      else
	itrPaths++;

      // When we reach the end of 'm_pattern', we don't have a whole
      // match of 'text'.
      if (next == m_pattern.size())
	continue;

      // Push to the back so that '*' will match zero length strings.
      if (m_pattern[next] == '*')
	paths.push_back(next);

      if (m_pattern[next] == *itrText)
	paths.push_front(next);
    }

    if (paths.empty())
      return false;
  }

  return std::find(paths.begin(), paths.end(), m_pattern.size() - 1) != paths.end();
}

}

#endif
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

// This is a hacked up whole string pattern matching. Replace with
// TR1's regex when that becomes widely available. It is intended for
// small strings.

#ifndef RAK_REGEX_H
#define RAK_REGEX_H

#include <sys/types.h>

#include <functional>
#include <string>
#include <list>

namespace rak {

class regex : public std::unary_function<std::string, bool> {
public:
  regex() {}
  regex(const std::string& p) : m_pattern(p) {}

  const std::string& pattern() const { return m_pattern; }

  bool operator () (const std::string& p) const;

private:
  std::string m_pattern;
};

// This isn't optimized, or very clean. A simple hack that should work.
bool
regex::operator () (const std::string& text) const {
  if (m_pattern.empty() ||
      text.empty() ||
      (m_pattern[0] != '*' && m_pattern[0] != text[0]))
    return false;

  // Replace with unordered_vector?
  std::list<unsigned int> paths;
  paths.push_front(0);

  for (std::string::const_iterator itrText = ++text.begin(), lastText = text.end(); itrText != lastText; ++itrText) {
    
    for (std::list<unsigned int>::iterator itrPaths = paths.begin(), lastPaths = paths.end(); itrPaths != lastPaths; ) {

      unsigned int next = *itrPaths + 1;

      if (m_pattern[*itrPaths] != '*')
	itrPaths = paths.erase(itrPaths);
      else
	itrPaths++;

      // When we reach the end of 'm_pattern', we don't have a whole
      // match of 'text'.
      if (next == m_pattern.size())
	continue;

      // Push to the back so that '*' will match zero length strings.
      if (m_pattern[next] == '*')
	paths.push_back(next);

      if (m_pattern[next] == *itrText)
	paths.push_front(next);
    }

    if (paths.empty())
      return false;
  }

  return std::find(paths.begin(), paths.end(), m_pattern.size() - 1) != paths.end();
}

}

#endif
