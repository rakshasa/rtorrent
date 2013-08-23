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

#include <unistd.h>
#include <sys/stat.h>
#include <torrent/connection_manager.h>

#include "core/manager.h"
#include "core/download_store.h"
#include "core/view_manager.h"
#include "core/dht_manager.h"
#include "core/http_queue.h"

#include "display/canvas.h"
#include "display/window.h"
#include "display/manager.h"
#include "input/manager.h"
#include "input/input_event.h"
#include "rpc/command_scheduler.h"
#include "rpc/parse_commands.h"
#include "rpc/scgi.h"
#include "rpc/object_storage.h"
#include "ui/root.h"

#include "control.h"

Control::Control() :
  m_ui(new ui::Root()),
  m_display(new display::Manager()),
  m_input(new input::Manager()),
  m_inputStdin(new input::InputEvent(STDIN_FILENO)),

  m_commandScheduler(new rpc::CommandScheduler()),
  m_objectStorage(new rpc::object_storage()),

  m_tick(0),
  m_shutdownReceived(false),
  m_shutdownQuick(false) {

  m_core        = new core::Manager();
  m_viewManager = new core::ViewManager();
  m_dhtManager  = new core::DhtManager();

  m_inputStdin->slot_pressed(std::tr1::bind(&input::Manager::pressed, m_input, std::tr1::placeholders::_1));

  m_taskShutdown.slot() = std::tr1::bind(&Control::handle_shutdown, this);

  m_commandScheduler->set_slot_error_message(rak::mem_fn(m_core, &core::Manager::push_log_std));
}

Control::~Control() {
  delete m_inputStdin;
  delete m_input;

  delete m_viewManager;

  delete m_ui;
  delete m_display;
  delete m_core;
  delete m_dhtManager;

  delete m_commandScheduler;
  delete m_objectStorage;
}

void
Control::initialize() {
  display::Canvas::initialize();
  display::Window::slot_schedule(rak::make_mem_fun(m_display, &display::Manager::schedule));
  display::Window::slot_unschedule(rak::make_mem_fun(m_display, &display::Manager::unschedule));
  display::Window::slot_adjust(rak::make_mem_fun(m_display, &display::Manager::adjust_layout));

  m_core->http_stack()->set_user_agent(USER_AGENT);

  m_core->initialize_second();
  m_core->listen_open();
  m_core->download_store()->enable(rpc::call_command_value("session.use_lock"));

  m_core->set_hashing_view(*m_viewManager->find_throw("hashing"));

  m_ui->init(this);

  m_inputStdin->insert(torrent::main_thread()->poll());
}

void
Control::cleanup() {
  //  delete m_scgi; m_scgi = NULL;
  rpc::xmlrpc.cleanup();

  priority_queue_erase(&taskScheduler, &m_taskShutdown);

  m_inputStdin->remove(torrent::main_thread()->poll());

  m_core->download_store()->disable();

  m_ui->cleanup();
  m_core->cleanup();
  
  display::Canvas::erase_std();
  display::Canvas::refresh_std();
  display::Canvas::do_update();
  display::Canvas::cleanup();
}

void
Control::cleanup_exception() {
  //  delete m_scgi; m_scgi = NULL;

  display::Canvas::cleanup();
}

bool
Control::is_shutdown_completed() {
  if (!m_shutdownQuick || worker_thread->is_active())
    return false;

  // Tracker requests can be disowned, so wait for these to
  // finish. The edge case of torrent http downloads may delay
  // shutdown.
  if (!core()->http_stack()->empty() || !core()->http_queue()->empty())
    return false;

  return torrent::is_inactive();
}

void
Control::handle_shutdown() {
  if (!m_shutdownQuick) {
    // Temporary hack:
    if (worker_thread->is_active())
      worker_thread->queue_item(&ThreadBase::stop_thread);

    torrent::connection_manager()->listen_close();
    m_core->shutdown(false);

    if (!m_taskShutdown.is_queued())
      priority_queue_insert(&taskScheduler, &m_taskShutdown, cachedTime + rak::timer::from_seconds(5));

  } else {
    // Temporary hack:
    if (worker_thread->is_active())
      worker_thread->queue_item(&ThreadBase::stop_thread);

    m_core->shutdown(true);
  }

  m_shutdownQuick = true;
  m_shutdownReceived = false;
}

