// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
#include "curl_stack.h"

namespace core {

CurlStack::CurlStack() :
  m_handle((void*)curl_multi_init()),
  m_size(0) {
}

CurlStack::~CurlStack() {
  while (!m_getList.empty())
    m_getList.front()->close();

  curl_multi_cleanup((CURLM*)m_handle);
}

void
CurlStack::perform() {
  int s;
  CURLMcode code;

  do {
    code = curl_multi_perform((CURLM*)m_handle, &s);

    if (code > 0)
      throw torrent::local_error("Error calling curl_multi_perform");

    if (s != m_size) {
      // Done with some handles.
      int t;

      do {
	CURLMsg* msg = curl_multi_info_read((CURLM*)m_handle, &t);

	CurlGetList::iterator itr = std::find_if(m_getList.begin(), m_getList.end(),
						 rak::equal(msg->easy_handle, std::mem_fun(&CurlGet::handle)));

	if (itr == m_getList.end())
	  throw torrent::client_error("Could not find CurlGet with the right easy_handle");
	
	(*itr)->perform(msg);
      } while (t);
    }

  } while (code == CURLM_CALL_MULTI_PERFORM);
}

void
CurlStack::fdset(fd_set* readfds, fd_set* writefds, fd_set* exceptfds, int* maxFd) {
  if (curl_multi_fdset((CURLM*)m_handle, readfds, writefds, exceptfds, maxFd) > 0)
    throw torrent::local_error("Error calling curl_multi_fdset");
}

void
CurlStack::add_get(CurlGet* get) {
  CURLMcode code;

  if ((code = curl_multi_add_handle((CURLM*)m_handle, get->handle())) > 0)
    throw torrent::local_error("curl_multi_add_handle \"" + std::string(curl_multi_strerror(code)));

  m_size++;
  m_getList.push_back(get);
}

void
CurlStack::remove_get(CurlGet* get) {
  if (curl_multi_remove_handle((CURLM*)m_handle, get->handle()) > 0)
    throw torrent::local_error("Error calling curl_multi_remove_handle");

  CurlGetList::iterator itr = std::find(m_getList.begin(), m_getList.end(), get);

  if (itr == m_getList.end())
    throw torrent::client_error("Could not find CurlGet when calling CurlStack::remove");

  m_size--;
  m_getList.erase(itr);
}

void
CurlStack::init() {
  curl_global_init(CURL_GLOBAL_ALL);
}

void
CurlStack::cleanup() {
  curl_global_cleanup();
}

}
