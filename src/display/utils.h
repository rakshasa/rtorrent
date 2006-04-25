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

#ifndef RTORRENT_DISPLAY_UTILS_H
#define RTORRENT_DISPLAY_UTILS_H

#include <ctime>
#include <string>

namespace core {
  class Download;
}

namespace utils {
  class Timer;
}

namespace torrent {
  class Entry;
}

class Control;

namespace display {

char*       print_string(char* first, char* last, char* str);

char*       print_hhmmss(char* first, char* last, time_t t);
char*       print_hhmmss_local(char* first, char* last, time_t t);
char*       print_ddhhmm(char* first, char* last, time_t t);
char*       print_ddmmyyyy(char* first, char* last, time_t t);

char*       print_download_title(char* first, char* last, core::Download* d);
char*       print_download_info(char* first, char* last, core::Download* d);
char*       print_download_status(char* first, char* last, core::Download* d);
char*       print_download_time_left(char* first, char* last, core::Download* d);
char*       print_download_percentage_done(char* first, char* last, core::Download* d);

char*       print_entry_tags(char* first, char* last);
char*       print_entry_file(char* first, char* last, const torrent::Entry& entry);

char*       print_status_info(char* first, char* last);
char*       print_status_extra(char* first, char* last, Control* c);

inline char*
print_buffer(char* first, char* last, const char* format) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

template <typename Arg1>
inline char*
print_buffer(char* first, char* last, const char* format, const Arg1& arg1) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format, arg1);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

template <typename Arg1, typename Arg2>
inline char*
print_buffer(char* first, char* last, const char* format, const Arg1& arg1, const Arg2& arg2) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format, arg1, arg2);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

template <typename Arg1, typename Arg2, typename Arg3>
inline char*
print_buffer(char* first, char* last, const char* format, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format, arg1, arg2, arg3);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
inline char*
print_buffer(char* first, char* last, const char* format, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format, arg1, arg2, arg3, arg4);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
inline char*
print_buffer(char* first, char* last, const char* format, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format, arg1, arg2, arg3, arg4, arg5);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
inline char*
print_buffer(char* first, char* last, const char* format, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6) {
  if (first >= last)
    return first;

  int s = snprintf(first, last - first, format, arg1, arg2, arg3, arg4, arg5, arg6);

  if (s < 0)
    return first;
  else
    return std::min(first + s, last);
}

}

#endif
