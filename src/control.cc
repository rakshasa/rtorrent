#include "config.h"

#include "control.h"

#include <unistd.h>
#include <sys/stat.h>
#include <torrent/net/http_stack.h>
#include <torrent/net/network_manager.h>
#include <torrent/utils/directory_events.h>

#include "core/dht_manager.h"
#include "core/download_store.h"
#include "core/http_queue.h"
#include "core/manager.h"
#include "core/view_manager.h"
#include "display/canvas.h"
#include "display/window.h"
#include "display/window_http_queue.h"
#include "display/window_input.h"
#include "display/window_statusbar.h"
#include "display/window_title.h"
#include "display/manager.h"
#include "input/manager.h"
#include "input/input_event.h"
#include "rpc/command_scheduler.h"
#include "rpc/lua.h"
#include "rpc/parse_commands.h"
#include "rpc/object_storage.h"
#include "ui/root.h"

Control::Control()
  : m_ui(new ui::Root()),
    m_display(new display::Manager()),
    m_input(new input::Manager()),
    m_inputStdin(new input::InputEvent(STDIN_FILENO)),
    m_commandScheduler(new rpc::CommandScheduler()),
    m_objectStorage(new rpc::object_storage()),
    m_lua_engine(new rpc::LuaEngine()),
    m_directory_events(new torrent::directory_events()) {

  m_core         = std::make_unique<core::Manager>();
  m_view_manager = std::make_unique<core::ViewManager>();
  m_dht_manager  = std::make_unique<core::DhtManager>();

  m_inputStdin->slot_pressed(std::bind(&input::Manager::pressed, m_input.get(), std::placeholders::_1));

  m_task_shutdown.slot() = std::bind(&Control::handle_shutdown, this);

  m_commandScheduler->set_slot_error_message([this](const std::string& msg) { m_core->push_log_std(msg); });
}

Control::~Control() {
  m_view_manager.reset();

  m_ui.reset();
  m_display.reset();
}

void
Control::initialize() {
  worker_thread->start_thread();

  display::Canvas::initialize();
  display::Window::slot_schedule([this](display::Window* w, std::chrono::microseconds t) { m_display->schedule(w, t); });
  display::Window::slot_unschedule([this](display::Window* w) { m_display->unschedule(w); });
  display::Window::slot_adjust([this]() { m_display->adjust_layout(); });

  torrent::net_thread::http_stack()->set_user_agent(USER_AGENT);

  m_core->listen_open();
  m_core->download_store()->enable(rpc::call_command_value("session.use_lock"));
  m_core->set_hashing_view(*m_view_manager->find_throw("hashing"));

  m_ui->init(this);

  if(!display::Canvas::daemon())
    m_inputStdin->insert(torrent::this_thread::poll());
}

void
Control::cleanup() {
  rpc::rpc.cleanup();

  torrent::this_thread::scheduler()->erase(&m_task_shutdown);

  if(!display::Canvas::daemon())
    m_inputStdin->remove(torrent::this_thread::poll());

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
  display::Canvas::cleanup();
}

bool
Control::is_shutdown_completed() {
  if (!m_shutdownQuick || worker_thread->is_active())
    return false;

  // Tracker requests can be disowned, so wait for these to
  // finish. The edge case of torrent http downloads may delay
  // shutdown.

  // TODO: We keep http requests in the queue for a while after, so improve this check to ignore
  // those.
  if (torrent::net_thread::http_stack()->size() != 0 || !core()->http_queue()->empty())
    return false;

  return torrent::is_inactive();
}

void
Control::handle_shutdown() {
  rpc::commands.call_catch("event.system.shutdown", rpc::make_target(), "shutdown", "System shutdown event action failed: ");

  if (!m_shutdownQuick) {
    if (worker_thread->is_active())
      worker_thread->stop_thread_wait();

    torrent::runtime::network_manager()->listen_close();

    m_directory_events->close();
    m_core->shutdown(false);

    if (!m_task_shutdown.is_scheduled())
      torrent::this_thread::scheduler()->wait_for_ceil_seconds(&m_task_shutdown, 5s);

  } else {
    if (worker_thread->is_active())
      worker_thread->stop_thread_wait();

    m_core->shutdown(true);
  }

  m_shutdownQuick = true;
  m_shutdownReceived = false;
}

