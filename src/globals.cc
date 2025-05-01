#include "config.h"

#include "globals.h"

// rak::priority_queue_default taskScheduler;
// rak::timer                  cachedTime;
rpc::ip_table_list          ip_tables;

Control*                    control = NULL;
ThreadWorker*               worker_thread = NULL;
