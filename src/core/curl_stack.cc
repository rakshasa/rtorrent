// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include "config.h"

#include <algorithm>
#include <curl/multi.h>
#include <torrent/exceptions.h>

#include "rak/functional.h"
#include "curl_get.h"
#include "curl_socket.h"
#include "curl_stack.h"

namespace core {

CurlStack::CurlStack() :
  m_handle((void*)curl_multi_init()),
  m_active(0),
  m_maxActive(32),
  m_ssl_verify_host(true),
  m_ssl_verify_peer(true),
  m_dns_timeout(60) {

  m_taskTimeout.slot() = std::bind(&CurlStack::receive_timeout, this);

#if (LIBCURL_VERSION_NUM >= 0x071000)
  curl_multi_setopt((CURLM*)m_handle, CURLMOPT_TIMERDATA, this);
  curl_multi_setopt((CURLM*)m_handle, CURLMOPT_TIMERFUNCTION, &CurlStack::set_timeout);
#endif
  curl_multi_setopt((CURLM*)m_handle, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt((CURLM*)m_handle, CURLMOPT_SOCKETFUNCTION, &CurlSocket::receive_socket);
}

CurlStack::~CurlStack() {
  while (!empty())
    front()->close();

  curl_multi_cleanup((CURLM*)m_handle);
  priority_queue_erase(&taskScheduler, &m_taskTimeout);
}

CurlGet*
CurlStack::new_object() {
  return new CurlGet(this);
}

CurlSocket*
CurlStack::new_socket(int fd) {
  CurlSocket* socket = new CurlSocket(fd, this);
  curl_multi_assign((CURLM*)m_handle, fd, socket);
  return socket;
}

void
CurlStack::receive_action(CurlSocket* socket, int events) {
  CURLMcode code;

  do {
    int count;
#if (LIBCURL_VERSION_NUM >= 0x071003)
    code = curl_multi_socket_action((CURLM*)m_handle,
                                    socket != NULL ? socket->file_descriptor() : CURL_SOCKET_TIMEOUT,
                                    events,
                                    &count);
#else
    code = curl_multi_socket((CURLM*)m_handle,
                             socket != NULL ? socket->file_descriptor() : CURL_SOCKET_TIMEOUT,
                             &count);
#endif

    if (code > 0)
      throw torrent::internal_error("Error calling curl_multi_socket_action.");

    // Socket might be removed when cleaning handles below, future
    // calls should not use it.
    socket = NULL;
    events = 0;

    if ((unsigned int)count != size()) {
      while (process_done_handle())
        ; // Do nothing.

      if (empty())
        priority_queue_erase(&taskScheduler, &m_taskTimeout);
    }

  } while (code == CURLM_CALL_MULTI_PERFORM);
}

bool
CurlStack::process_done_handle() {
  int remaining_msgs = 0;
  CURLMsg* msg = curl_multi_info_read((CURLM*)m_handle, &remaining_msgs);

  if (msg == NULL)
    return false;

  if (msg->msg != CURLMSG_DONE)
    throw torrent::internal_error("CurlStack::receive_action() msg->msg != CURLMSG_DONE.");

  transfer_done(msg->easy_handle,
                msg->data.result == CURLE_OK ? NULL : curl_easy_strerror(msg->data.result));

  return remaining_msgs != 0;
}

void
CurlStack::transfer_done(void* handle, const char* msg) {
  iterator itr = std::find_if(begin(), end(), rak::equal(handle, std::mem_fun(&CurlGet::handle)));

  if (itr == end())
    throw torrent::internal_error("Could not find CurlGet with the right easy_handle.");

  if (msg == NULL)
    (*itr)->trigger_done();
  else
    (*itr)->trigger_failed(msg);
}

void
CurlStack::receive_timeout() {
  receive_action(NULL, 0);

  // Sometimes libcurl forgets to reset the timeout. Try to poll the value in that case, or use 10 seconds.
  if (!empty() && !m_taskTimeout.is_queued()) {
    long timeout;
    curl_multi_timeout((CURLM*)m_handle, &timeout);
    priority_queue_insert(&taskScheduler, &m_taskTimeout, 
                          cachedTime + rak::timer::from_milliseconds(std::max<unsigned long>(timeout, 10000)));
  }
}

void
CurlStack::add_get(CurlGet* get) {
  if (!m_userAgent.empty())
    curl_easy_setopt(get->handle(), CURLOPT_USERAGENT, m_userAgent.c_str());

  if (!m_httpProxy.empty())
    curl_easy_setopt(get->handle(), CURLOPT_PROXY, m_httpProxy.c_str());

  if (!m_bindAddress.empty())
    curl_easy_setopt(get->handle(), CURLOPT_INTERFACE, m_bindAddress.c_str());

  if (!m_httpCaPath.empty())
    curl_easy_setopt(get->handle(), CURLOPT_CAPATH, m_httpCaPath.c_str());

  if (!m_httpCaCert.empty())
    curl_easy_setopt(get->handle(), CURLOPT_CAINFO, m_httpCaCert.c_str());

  curl_easy_setopt(get->handle(), CURLOPT_SSL_VERIFYHOST, (long)(m_ssl_verify_host ? 2 : 0));
  curl_easy_setopt(get->handle(), CURLOPT_SSL_VERIFYPEER, (long)(m_ssl_verify_peer ? 1 : 0));
  curl_easy_setopt(get->handle(), CURLOPT_DNS_CACHE_TIMEOUT, m_dns_timeout);

  base_type::push_back(get);

  if (m_active >= m_maxActive)
    return;

  m_active++;
  get->set_active(true);
  
  if (curl_multi_add_handle((CURLM*)m_handle, get->handle()) > 0)
    throw torrent::internal_error("Error calling curl_multi_add_handle.");

#if (LIBCURL_VERSION_NUM < 0x071000)
  receive_timeout();
#endif
}

void
CurlStack::remove_get(CurlGet* get) {
  iterator itr = std::find(begin(), end(), get);

  if (itr == end())
    throw torrent::internal_error("Could not find CurlGet when calling CurlStack::remove.");

  base_type::erase(itr);

  // The CurlGet object was never activated, so we just skip this one.
  if (!get->is_active())
    return;

  get->set_active(false);

  if (curl_multi_remove_handle((CURLM*)m_handle, get->handle()) > 0)
    throw torrent::internal_error("Error calling curl_multi_remove_handle.");

  if (m_active == m_maxActive &&
      (itr = std::find_if(begin(), end(), std::not1(std::mem_fun(&CurlGet::is_active)))) != end()) {
    (*itr)->set_active(true);

    if (curl_multi_add_handle((CURLM*)m_handle, (*itr)->handle()) > 0)
      throw torrent::internal_error("Error calling curl_multi_add_handle.");

  } else {
    m_active--;
  }
}

void
CurlStack::global_init() {
  curl_global_init(CURL_GLOBAL_ALL);
}

void
CurlStack::global_cleanup() {
  curl_global_cleanup();
}

// TODO: Is this function supposed to set a per-handle timeout, or is
// it the shortest timeout amongst all handles?
int
CurlStack::set_timeout(void* handle, long timeout_ms, void* userp) {
  CurlStack* stack = (CurlStack*)userp;

  priority_queue_erase(&taskScheduler, &stack->m_taskTimeout);
  priority_queue_insert(&taskScheduler, &stack->m_taskTimeout, cachedTime + rak::timer::from_milliseconds(timeout_ms));

  return 0;
}

}
