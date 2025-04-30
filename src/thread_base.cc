#include "config.h"

#include "thread_base.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <rak/error_number.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/utils/chrono.h>
#include <torrent/utils/log.h>
#include <unistd.h>

#include "globals.h"

std::chrono::microseconds
ThreadBase::next_timeout() {
  return std::chrono::microseconds(10min);
}

void
ThreadBase::call_events() {
  cachedTime = rak::timer::current();

  process_callbacks();
}
