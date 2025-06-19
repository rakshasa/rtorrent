#include "config.h"

#include <algorithm>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "globals.h"
#include "control.h"
#include "rpc/parse_commands.h"

#include "download.h"
#include "download_list.h"
#include "manager.h"
#include "view.h"
#include "view_manager.h"

namespace core {

void
ViewManager::clear() {
  for (auto v : *this)
    delete v;

  base_type::clear();
}

ViewManager::iterator
ViewManager::insert(const std::string& name) {
  if (name.empty())
    throw torrent::input_error("View with empty name not supported.");

  if (find(name) != end())
    throw torrent::input_error("View with same name already inserted.");

  View* view = new View();
  view->initialize(name);

  return base_type::insert(end(), view);
}

ViewManager::iterator
ViewManager::find(const std::string& name) {
  return std::find_if(begin(), end(), [name](View* v){ return name == v->name(); });
}

ViewManager::iterator
ViewManager::find_throw(const std::string& name) {
  iterator itr = std::find_if(begin(), end(), [name](View* v){ return name == v->name(); });

  if (itr == end())
    throw torrent::input_error("Could not find view: " + name);

  return itr;
}

void
ViewManager::sort(const std::string& name, uint32_t timeout) {
  iterator viewItr = find_throw(name);

  if ((*viewItr)->last_changed() + std::chrono::seconds(timeout) > torrent::this_thread::cached_time())
    return;

  // Should we rename sort, or add a seperate function?
  (*viewItr)->filter();
  (*viewItr)->sort();
}

void
ViewManager::set_filter(const std::string& name, const torrent::Object& cmd) {
  iterator viewItr = find_throw(name);

  (*viewItr)->set_filter(cmd);
  (*viewItr)->filter();
}

void
ViewManager::set_filter_temp(const std::string& name, const torrent::Object& cmd) {
  iterator viewItr = find_throw(name);

  (*viewItr)->set_filter_temp(cmd);
  (*viewItr)->filter();
}

void
ViewManager::set_filter_on(const std::string& name, const filter_args& args) {
  iterator viewItr = find_throw(name);

  (*viewItr)->clear_filter_on();

  // TODO: Ensure the filter keys are rlookup.

  for (const auto& arg : args)
    (*viewItr)->set_filter_on_event(arg);
}

}
