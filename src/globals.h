#ifndef TORRENT_GLOBALS_H
#define TORRENT_GLOBALS_H

#include <torrent/common.h>

#include "rpc/ip_table_list.h"

class Control;
class ThreadWorker;

extern rpc::ip_table_list ip_tables;

extern Control*      control;

// TODO: Update to new thread model.
extern ThreadWorker* worker_thread;

namespace session {

class SessionManager;

} // namespace session


namespace session_thread {

torrent::utils::Thread* thread();
std::thread::id         thread_id();

void                    callback(void* target, std::function<void ()>&& fn);
void                    cancel_callback(void* target);
void                    cancel_callback_and_wait(void* target);

session::SessionManager* manager();

} // namespace torrent::session_thread

#endif
