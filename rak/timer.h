// libTorrent - BitTorrent library
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

#ifndef RAK_TIMER_H
#define RAK_TIMER_H

#include <limits>
#include <inttypes.h>
#include <sys/time.h>

namespace rak {

// Don't convert negative Timer to timeval and then back to Timer, that will bork.
class timer {
 public:
  timer(int64_t usec = 0) : m_time(usec) {}
  timer(timeval tv) : m_time((int64_t)(uint32_t)tv.tv_sec * 1000000 + (int64_t)(uint32_t)tv.tv_usec % 1000000) {}

  bool                is_zero() const                    { return m_time == 0; }
  bool                is_not_zero() const                { return m_time != 0; }

  int32_t             seconds() const                    { return m_time / 1000000; }
  int32_t             seconds_ceiling() const            { return (m_time + 1000000 - 1) / 1000000; }
  int64_t             usec() const                       { return m_time; }

  timer               round_seconds() const              { return (m_time / 1000000) * 1000000; }
  timer               round_seconds_ceiling() const      { return ((m_time + 1000000 - 1) / 1000000) * 1000000; }

  timeval             tval() const {
    timeval val;
    val.tv_sec  = m_time / 1000000;
    val.tv_usec = m_time % 1000000;
    return val;
  }

  static timer        current();
  static int64_t      current_seconds()                   { return current().seconds(); }
  static int64_t      current_usec()                      { return current().usec(); }
  static timer        from_minutes(uint32_t minutes)      { return rak::timer((uint64_t)minutes * 60 * 1000000); }
  static timer        from_seconds(uint32_t seconds)      { return rak::timer((uint64_t)seconds * 1000000); }
  static timer        from_milliseconds(uint32_t msec)    { return rak::timer((uint64_t)msec * 1000); }

  static timer        max()                              { return std::numeric_limits<int64_t>::max(); }

  bool                operator <  (const timer& t) const { return m_time < t.m_time; }
  bool                operator >  (const timer& t) const { return m_time > t.m_time; }
  bool                operator <= (const timer& t) const { return m_time <= t.m_time; }
  bool                operator >= (const timer& t) const { return m_time >= t.m_time; }
  bool                operator == (const timer& t) const { return m_time == t.m_time; }
  bool                operator != (const timer& t) const { return m_time != t.m_time; }

  timer               operator - (const timer& t) const  { return timer(m_time - t.m_time); }
  timer               operator + (const timer& t) const  { return timer(m_time + t.m_time); }
  timer               operator * (int64_t t) const       { return timer(m_time * t); }
  timer               operator / (int64_t t) const       { return timer(m_time / t); }

  timer               operator -= (int64_t t)            { m_time -= t; return *this; }
  timer               operator -= (const timer& t)       { m_time -= t.m_time; return *this; }

  timer               operator += (int64_t t)            { m_time += t; return *this; }
  timer               operator += (const timer& t)       { m_time += t.m_time; return *this; }

 private:
  int64_t             m_time;
};

inline timer
timer::current() {
  timeval t;
  gettimeofday(&t, 0);
  
  return timer(t);
}

}

#endif
