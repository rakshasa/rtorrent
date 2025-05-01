#ifndef TORRENT_GLOBALS_H
#define TORRENT_GLOBALS_H

#include <rak/timer.h>
#include <rak/priority_queue_default.h>

#include "thread_worker.h"
#include "rpc/ip_table_list.h"

class Control;

// The cachedTime timer should only be updated by the main thread to
// avoid potential problems in timing calculations. Code really should
// be reviewed and fixed in order to avoid any potential problems, and
// then made updates properly sync'ed with memory barriers.

extern rak::priority_queue_default taskScheduler;
extern rak::timer                  cachedTime;
extern rpc::ip_table_list          ip_tables;

extern Control*      control;
extern ThreadWorker* worker_thread;

#endif
