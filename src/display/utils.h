#ifndef RTORRENT_DISPLAY_UTILS_H
#define RTORRENT_DISPLAY_UTILS_H

#include <ctime>
#include <cstdio>
#include <string>
#include <vector>

namespace core {
  class Download;
}

namespace utils {
  class Timer;
}

namespace torrent {
  class ClientInfo;
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
char*       print_download_info_full(char* first, char* last, core::Download* d);
char*       print_download_status(char* first, char* last, core::Download* d);

char*       print_download_column_compact(char* first, char* last);
char*       print_download_info_compact(char* first, char* last, core::Download* d);

char*       print_download_time_left(char* first, char* last, core::Download* d);
char*       print_download_percentage_done(char* first, char* last, core::Download* d);

char*       print_client_version(char* first, char* last, const torrent::ClientInfo& clientInfo);

char*       print_entry_tags(char* first, char* last);
char*       print_entry_file(char* first, char* last, const torrent::Entry& entry);

char*       print_status_throttle_limit(char* first, char* last, bool up, const std::vector<std::string>& throttle_names);
char*       print_status_throttle_rate(char* first, char* last, bool up, const std::vector<std::string>& throttle_names, const double& global_rate);

char*       print_status_info(char* first, char* last);
char*       print_status_extra(char* first, char* last);

inline char*
print_buffer(char* first, char* last, const char* format) {
  if (first >= last)
    return first;

  // Adding 'i' format to suppress a GCC warning.
  int s = snprintf(first, last - first, format, 0);

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
