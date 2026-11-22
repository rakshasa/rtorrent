#include "config.h"

#include <sys/types.h>
#include <torrent/exceptions.h>

#include "core/manager.h"
#include "core/download.h"
#include "core/download_list.h"
#include "core/view.h"
#include "core/view_manager.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

static void
resume_started(core::View* started, core::View* active, uint64_t max_active) {
  for (core::View::size_type i = 0; i < started->size_visible(); i++) {
    auto download = (started->begin_visible() + i)->get();

    if (download == nullptr || download->is_active())
      continue;

    auto lifetime = download->lifetime();
    auto size     = started->size_visible();

    control->core()->download_list()->resume(download);

    // A download that failed to start has left 'active' again, so count the view.
    if (active->size_visible() >= max_active)
      break;

    if (lifetime.expired() || started->size_visible() != size || (started->begin_visible() + i)->get() != download)
      throw torrent::input_error("Scheduler stopped: an event handler changed the started view.");
  }
}

torrent::Object
cmd_scheduler_simple_added(core::Download* download) {
  unsigned int numActive = (*control->view_manager()->find("active"))->size_visible();
  int64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if (numActive < (uint64_t)maxActive)
    control->core()->download_list()->resume(download);

  return torrent::Object();
}

torrent::Object
cmd_scheduler_simple_removed(core::Download* download) {
  control->core()->download_list()->pause(download);

  core::View* viewActive = *control->view_manager()->find("active");
  unsigned int numActive = viewActive->size_visible();
  int64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if ((int64_t)numActive >= maxActive)
    return torrent::Object();

  // The 'started' view contains all the views we may choose amongst.
  core::View* viewStarted = *control->view_manager()->find("started");

  resume_started(viewStarted, viewActive, (uint64_t)maxActive);

  return torrent::Object();
}

torrent::Object
cmd_scheduler_simple_update([[maybe_unused]] core::Download* download) {
  core::View* viewActive = *control->view_manager()->find("active");
  core::View* viewStarted = *control->view_manager()->find("started");

  uint64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if (viewActive->size_visible() < maxActive) {
    resume_started(viewStarted, viewActive, maxActive);

  } else {
    while (viewActive->size_visible() > maxActive) {
      auto size = viewActive->size_visible();

      control->core()->download_list()->pause(*viewActive->begin_visible());

      if (viewActive->size_visible() >= size)
        throw torrent::input_error("Scheduler stopped: an event handler prevented the active view from shrinking.");
    }
  }

  return torrent::Object();
}

void
initialize_command_scheduler() {
  CMD2_VAR_VALUE("scheduler.max_active", int64_t(-1));

  CMD2_DL("scheduler.simple.added",   std::bind(&cmd_scheduler_simple_added, std::placeholders::_1));
  CMD2_DL("scheduler.simple.removed", std::bind(&cmd_scheduler_simple_removed, std::placeholders::_1));
  CMD2_DL("scheduler.simple.update",  std::bind(&cmd_scheduler_simple_update, std::placeholders::_1));
}
