#include "config.h"

#include <sys/types.h>

#include "core/manager.h"
#include "core/download.h"
#include "core/download_list.h"
#include "core/view.h"
#include "core/view_manager.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

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
  int64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if ((int64_t)viewActive->size_visible() >= maxActive)
    return torrent::Object();

  // The 'started' view contains all the views we may choose amongst.
  core::View* viewStarted = *control->view_manager()->find("started");

  for (core::View::iterator itr = viewStarted->begin_visible(), last = viewStarted->end_visible(); itr != last; itr++) {
    if ((*itr)->is_active())
      continue;

    control->core()->download_list()->resume(*itr);
  }

  return torrent::Object();
}

torrent::Object
cmd_scheduler_simple_update([[maybe_unused]] core::Download* download) {
  core::View* viewActive = *control->view_manager()->find("active");
  core::View* viewStarted = *control->view_manager()->find("started");

  unsigned int numActive = viewActive->size_visible();
  uint64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if (viewActive->size_visible() < maxActive) {
    for (core::View::iterator itr = viewStarted->begin_visible(), last = viewStarted->end_visible(); itr != last; itr++) {
      if ((*itr)->is_active())
        continue;

      control->core()->download_list()->resume(*itr);

      if (++numActive >= maxActive)
        break;
    }

  } else {
    while (viewActive->size_visible() > maxActive)
      control->core()->download_list()->pause(*viewActive->begin_visible());
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
