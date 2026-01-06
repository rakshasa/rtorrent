#ifndef TORRENT_GLOBALS_H
#define TORRENT_GLOBALS_H

#include <torrent/common.h>

#include "rpc/ip_table_list.h"

class Control;

extern rpc::ip_table_list ip_tables;
extern Control*           control;

std::string expand_path(const std::string& path);

namespace rpc {
class SCgi;
}

namespace session {
class SessionManager;
}

namespace scgi_thread {

torrent::utils::Thread* thread();
std::thread::id         thread_id();

void                    callback(void* target, std::function<void ()>&& fn);
void                    cancel_callback(void* target);
void                    cancel_callback_and_wait(void* target);

rpc::SCgi*              scgi();
void                    set_scgi(rpc::SCgi* scgi);
void                    set_rpc_log(const std::string& filename);

} // namespace torrent::scgi_thread


namespace session_thread {

torrent::utils::Thread* thread();
std::thread::id         thread_id();

void                    callback(void* target, std::function<void ()>&& fn);
void                    cancel_callback(void* target);
void                    cancel_callback_and_wait(void* target);

session::SessionManager* manager();
std::string              session_path();

} // namespace torrent::session_thread

#endif
