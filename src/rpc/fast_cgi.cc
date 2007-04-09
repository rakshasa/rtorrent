// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

#ifdef HAVE_FASTCGI
#include <memory>
#include <fcgiapp.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rak/functional_fun.h>
#include <torrent/exceptions.h>

#include "globals.h"
#include "control.h"
#include "core/manager.h"

#endif

#include "fast_cgi.h"

namespace rpc {

bool FastCgi::m_initialized = false;

#ifdef HAVE_FASTCGI

FastCgi::FastCgi(const std::string& path) : m_path(path) {
  if (!m_initialized)
    if (FCGX_Init() != 0)
      throw torrent::resource_error("Could not initialize FastCGI library.");

  // Register the fd with torrent::ConnectionManager.

  // Set non-blocking as per
  // http://www.fastcgi.com/archives/fastcgi-developers/2004-January/003136.html?
  if ((m_fileDesc = FCGX_OpenSocket(m_path.c_str(), 5)) == -1)
    throw torrent::resource_error("FastCGI could not open socket.");

  m_request = new FCGX_Request;

  // Need FCGI_FAIL_ON_INTR flag?
  if (fcntl(m_fileDesc, F_SETFL, O_NONBLOCK) != 0 ||
      FCGX_InitRequest(m_request, m_fileDesc, 0) != 0) {
    ::close(m_fileDesc);
    delete m_request;

    throw torrent::resource_error("FastCGI could not initialize request.");
  }

  // Meh.
  control->core()->get_poll_manager()->get_torrent_poll()->open(this);
  control->core()->get_poll_manager()->get_torrent_poll()->insert_read(this);
}

FastCgi::~FastCgi() {
  if (m_request == NULL)
    return;

  control->core()->get_poll_manager()->get_torrent_poll()->remove_read(this);
  control->core()->get_poll_manager()->get_torrent_poll()->close(this);

  FCGX_Free(m_request, true);

  m_fileDesc = 0;
  m_request  = NULL;

  // Also unlink the socket file.
}

// This is fundementally wrong as it blocks. We can live with it as
// the http server will usually buffer the whole requests before
// opening the connection.

void
FastCgi::event_read() {
  if (FCGX_Accept_r(m_request) != 0) {
    control->core()->push_log("FastCGI accept failed.");
    return;
  }

  int length;
  char* endPtr;
  char* buffer = NULL;
  slot_write slotWrite;

  const char* contentLength = FCGX_GetParam("CONTENT_LENGTH", m_request->envp);

  if (contentLength == NULL || (length = strtol(contentLength, &endPtr, 10)) < 0 || *endPtr != '\0') {
    control->core()->push_log("FastCGI invalid content length.");
    goto event_read_exit;
  }
  
  buffer = new char[length];

  if (FCGX_GetStr(buffer, length, m_request->in) != length) {
    control->core()->push_log("FastCGI could not read sufficient data.");
    goto event_read_exit;
  }

  slotWrite.set(rak::mem_fn(this, &FastCgi::receive_write));

  if (!m_slotProcess(buffer, length, slotWrite) ||
      FCGX_FFlush(m_request->out) == -1) {
    control->core()->push_log("FastCGI could not write data.");
    goto event_read_exit;
  }

  control->core()->push_log("Processed an XMLRPC call.");

event_read_exit:
  FCGX_Finish_r(m_request);
  delete buffer;
}

void
FastCgi::event_write() {
}

void
FastCgi::event_error() {
  throw torrent::internal_error("FastCgi::event_error().");
}

bool
FastCgi::receive_write(const char* buffer, uint32_t length) {
  return
    FCGX_FPrintF(m_request->out,
                 "Content-type: text/plain\r\n"
                 "Content-length: %i\r\n\r\n", length) != -1 &&
    FCGX_PutStr(buffer, length, m_request->out) != -1;
}

#else

FastCgi::FastCgi(const std::string& path) { throw torrent::resource_error("FastCGI not supported."); }
FastCgi::~FastCgi() {}
void FastCgi::event_read() {}
void FastCgi::event_write() {}
void FastCgi::event_error() {}

void FastCgi::receive_write(const char* buffer, uint32_t length);

#endif

}
