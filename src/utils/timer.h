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

#ifndef RTORRENT_UTILS_TIMER_H
#define RTORRENT_UTILS_TIMER_H

#include <limits>
#include <inttypes.h>
#include <sys/time.h>

namespace utils {

// Don't convert negative Timer to timeval.
class Timer {
 public:
  Timer() : m_time(0) {}
  Timer(int64_t usec) : m_time(usec) {}
  Timer(timeval tv) : m_time((int64_t)(uint32_t)tv.tv_sec * 1000000 + (int64_t)(uint32_t)tv.tv_usec % 1000000) {}

  int64_t   usec() const                   { return m_time; }
  int32_t   sec() const                    { return m_time / 1000000; }
  timeval   tval() const                   { return (timeval) { m_time / 1000000, m_time % 1000000}; }

  Timer     round_seconds() const          { return (m_time / 1000000) * 1000000; }

  static Timer current() {
    timeval t;
    gettimeofday(&t, 0);

    return Timer(t);
  }

  // Cached time, updated in the beginning of torrent::work call.
  // Don't use outside socket_base read/write/except or Service::service.
  
  // TODO: Find out if it's worth it. The kernel is supposed to cache the
  // time. Though system calls would be more expensive than we can afford.
  static Timer cache()                     { return Timer(m_cache); }

  // TODO: Create sd::numeric_limits for these?
  static Timer min()                       { return 0; }
  static Timer max()                       { return (int64_t)std::numeric_limits<time_t>::max() * 1000000; }

  static void  update()                    { m_cache = Timer::current().usec(); }

  Timer operator - (const Timer& t) const  { return Timer(m_time - t.m_time); }
  Timer operator + (const Timer& t) const  { return Timer(m_time + t.m_time); }

  Timer operator -= (int64_t t)            { m_time -= t; return *this; }
  Timer operator -= (const Timer& t)       { m_time -= t.m_time; return *this; }

  Timer operator += (int64_t t)            { m_time += t; return *this; }
  Timer operator += (const Timer& t)       { m_time += t.m_time; return *this; }

  bool  operator <  (const Timer& t) const { return m_time < t.m_time; }
  bool  operator >  (const Timer& t) const { return m_time > t.m_time; }
  bool  operator <= (const Timer& t) const { return m_time <= t.m_time; }
  bool  operator >= (const Timer& t) const { return m_time >= t.m_time; }
  bool  operator == (const Timer& t) const { return m_time == t.m_time; }

 private:
  int64_t m_time;

  // Instantiated in torrent.cc
  static int64_t m_cache;
};

}

#endif // LIBTORRENT_TIMER_H
