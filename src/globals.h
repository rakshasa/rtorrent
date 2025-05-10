#ifndef TORRENT_GLOBALS_H
#define TORRENT_GLOBALS_H

#include "thread_worker.h"
#include "rpc/ip_table_list.h"

class Control;

extern rpc::ip_table_list          ip_tables;

extern Control*      control;
extern ThreadWorker* worker_thread;

#endif
