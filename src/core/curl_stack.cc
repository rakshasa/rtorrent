// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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
#include <sigc++/adaptors/bind.h>
#include <torrent/exceptions.h>

#include "rak/functional.h"
#include "curl_get.h"
#include "curl_stack.h"

namespace core {

CurlStack::CurlStack() :
  m_handle((void*)curl_multi_init()),
  m_active(0),
  m_maxActive(32) {
}

CurlStack::~CurlStack() {
  while (!empty())
    front()->close();

  curl_multi_cleanup((CURLM*)m_handle);
}

CurlGet*
CurlStack::new_object() {
  return new CurlGet(this);
}

void
CurlStack::perform() {
  CURLMcode code;

  do {
    int count;
    code = curl_multi_perform((CURLM*)m_handle, &count);

    if (code > 0)
      throw torrent::internal_error("Error calling curl_multi_perform.");

    if ((unsigned int)count != size()) {
      // Done with some handles.
      int t;
      CURLMsg* msg;

      while ((msg = curl_multi_info_read((CURLM*)m_handle, &t)) != NULL) {
        if (msg->msg != CURLMSG_DONE)
          throw torrent::internal_error("CurlStack::perform() msg->msg != CURLMSG_DONE.");

        iterator itr = std::find_if(begin(), end(), rak::equal(msg->easy_handle, std::mem_fun(&CurlGet::handle)));

        if (itr == end())
          throw torrent::internal_error("Could not find CurlGet with the right easy_handle.");
        
        if (msg->data.result == CURLE_OK)
          (*itr)->signal_done().emit();
        else
          (*itr)->signal_failed().emit(curl_easy_strerror(msg->data.result));
      }
    }

  } while (code == CURLM_CALL_MULTI_PERFORM);
}

unsigned int
CurlStack::fdset(fd_set* readfds, fd_set* writefds, fd_set* exceptfds) {
  int maxFd = 0;

  if (curl_multi_fdset((CURLM*)m_handle, readfds, writefds, exceptfds, &maxFd) != 0)
    throw torrent::internal_error("Error calling curl_multi_fdset.");

  return std::max(maxFd, 0);
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

  base_type::push_back(get);

  if (m_active >= m_maxActive)
    return;

  m_active++;
  get->set_active(true);
  
  if (curl_multi_add_handle((CURLM*)m_handle, get->handle()) > 0)
    throw torrent::internal_error("Error calling curl_multi_add_handle.");
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

}
