#include "config.h"

#include <sys/types.h>

#include <ctime>
#include <regex>
#include <rak/algorithm.h>
#include <torrent/utils/log.h>

#include "core/manager.h"
#include "core/view_manager.h"
#include "display/canvas.h"
#include "ui/root.h"
#include "ui/download_list.h"
#include "display/color_map.h"
#include "rpc/parse.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

typedef void (core::ViewManager::*view_event_slot)(const std::string&, const torrent::Object&);

torrent::Object
apply_view_filter_on(const torrent::Object::list_type& args) {
  if (args.size() < 1)
    throw torrent::input_error("Too few arguments.");

  const std::string& name = args.front().as_string();

  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  core::ViewManager::filter_args filterArgs;

  for (torrent::Object::list_const_iterator itr = ++args.begin(), last = args.end(); itr != last; itr++)
    filterArgs.push_back(itr->as_string());

  control->view_manager()->set_filter_on(name, filterArgs);

  return torrent::Object();
}

torrent::Object
apply_view_event(view_event_slot viewFilterSlot, const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  (control->view_manager()->*viewFilterSlot)(args.front().as_string(), args.back());
  return torrent::Object();
}

torrent::Object
apply_view_sort(const torrent::Object::list_type& args) {
  if (args.size() <= 0 || args.size() > 2)
    throw torrent::input_error("Wrong argument count.");

  const std::string& name = args.front().as_string();

  if (name.empty())
    throw torrent::input_error("First argument must be a string.");

  int32_t value = 0;

  if (args.size() == 2)
    value = rpc::convert_to_value(args.back());

  control->view_manager()->sort(name, value);

  return torrent::Object();
}

torrent::Object
apply_view_list() {
  torrent::Object rawResult = torrent::Object::create_list();
  torrent::Object::list_type& result = rawResult.as_list();

  for (auto itr : *control->view_manager())
    result.push_back(itr->name());

  return rawResult;
}

torrent::Object
apply_view_set(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  core::ViewManager::iterator itr = control->view_manager()->find(args.back().as_string());

  if (itr == control->view_manager()->end())
    throw torrent::input_error("Could not find view \"" + args.back().as_string() + "\".");

//   if (args.front().as_string() == "main")
//     control->ui()->download_list()->set_view(*itr);
//   else

  throw torrent::input_error("No such target.");
}

torrent::Object
apply_print([[maybe_unused]] rpc::target_type target, const torrent::Object& rawArgs) {
  char buffer[1024];
  rpc::print_object(buffer, buffer + 1024, &rawArgs, 0);

  control->core()->push_log(buffer);
  return torrent::Object();
}

torrent::Object
apply_cat([[maybe_unused]] rpc::target_type target, const torrent::Object& rawArgs) {
  std::string result;

  rpc::print_object_std(&result, &rawArgs, 0);
  return result;
}

torrent::Object
apply_value([[maybe_unused]] rpc::target_type target, const torrent::Object::list_type& args) {
  if (args.size() < 1)
    throw torrent::input_error("'value' takes at least a number argument!");
  if (args.size() > 2)
    throw torrent::input_error("'value' takes at most two arguments!");

  torrent::Object::value_type val = 0;
  if (args.front().is_value()) {
    val = args.front().as_value();
  } else {
    int base = args.size() > 1 ? args.back().is_value() ?
               args.back().as_value() : strtol(args.back().as_string().c_str(), NULL, 10) : 10;
    char* endptr = 0;

    val = strtoll(args.front().as_string().c_str(), &endptr, base);
    while (*endptr == ' ' || *endptr == '\n') ++endptr;
    if (*endptr)
      throw torrent::input_error("Junk at end of number: " + args.front().as_string());
  }

  return val;
}

torrent::Object
apply_try(rpc::target_type target, const torrent::Object& args) {
  try {
    return rpc::call_object(args, target);
  } catch (torrent::input_error& e) {
    lt_log_print(torrent::LOG_RPC_EVENTS, "try command caught input_error: %s", e.what());
  }

  return torrent::Object();
}

// Move these boolean operators to a new file.

inline bool
as_boolean(const torrent::Object& rawArgs) {
  switch (rawArgs.type()) {
  case torrent::Object::TYPE_VALUE:  return rawArgs.as_value();
  case torrent::Object::TYPE_STRING: return !rawArgs.as_string().empty();

  // We need to properly handle argument lists that are single-object
  // at a higher level.
  case torrent::Object::TYPE_LIST:   return !rawArgs.as_list().empty() && as_boolean(rawArgs.as_list().front());
  // case torrent::Object::TYPE_MAP:    return !rawArgs.as_map().empty();
  default: return false;
  }
}

torrent::Object
apply_not(rpc::target_type target, const torrent::Object& rawArgs) {
  bool result;

  if (rawArgs.is_dict_key())
    result = as_boolean(rpc::commands.call_command(rawArgs.as_dict_key().c_str(), rawArgs.as_dict_obj(),
                                                   target));
  // TODO: Thus we should clean this up...
  else if (rawArgs.is_list() && !rawArgs.as_list().empty())
    return apply_not(target, rawArgs.as_list().front());

  else
    result = as_boolean(rawArgs);

  return (int64_t)!result;
}

torrent::Object
apply_false([[maybe_unused]] rpc::target_type target, [[maybe_unused]] const torrent::Object& rawArgs) {
  return (int64_t)0;
}

torrent::Object
apply_and(rpc::target_type target, const torrent::Object& rawArgs) {
  if (rawArgs.type() != torrent::Object::TYPE_LIST)
    return as_boolean(rawArgs);

  for (const auto& itr : rawArgs.as_list())
    if (itr.is_dict_key()) {
      if (!as_boolean(rpc::commands.call_command(itr.as_dict_key().c_str(), itr.as_dict_obj(), target)))
        return (int64_t)false;

    } else if (itr.is_value()) {
      if (!itr.as_value())
        return (int64_t)false;

    } else {
      // TODO: Switch to new versions that only accept the new command syntax.
      if (!as_boolean(rpc::parse_command_single(target, itr.as_string())))
        return (int64_t)false;
    }

  return (int64_t)true;
}

torrent::Object
apply_or(rpc::target_type target, const torrent::Object& rawArgs) {
  if (rawArgs.type() != torrent::Object::TYPE_LIST)
    return as_boolean(rawArgs);

  for (const auto& itr : rawArgs.as_list())
    if (itr.is_dict_key()) {
      if (as_boolean(rpc::commands.call_command(itr.as_dict_key().c_str(), itr.as_dict_obj(), target)))
        return (int64_t)true;

    } else if (itr.is_value()) {
      if (itr.as_value())
        return (int64_t)true;

    } else {
      if (as_boolean(rpc::parse_command_single(target, itr.as_string())))
        return (int64_t)true;
    }

  return (int64_t)false;
}

torrent::Object
apply_cmp(rpc::target_type target, const torrent::Object::list_type& args) {
  // We only need to check if empty() since if size() == 1 it calls
  // the same command for both, or if size() == 2 then each side of
  // the comparison has different commands.
  if (args.empty())
    throw torrent::input_error("Wrong argument count.");

  // This really should be converted to using args flagged as
  // commands, so that we can compare commands and statics values.

  torrent::Object result1;
  torrent::Object result2;

  rpc::target_type target1 = rpc::is_target_pair(target) ? rpc::get_target_left(target) : target;
  rpc::target_type target2 = rpc::is_target_pair(target) ? rpc::get_target_right(target) : target;

  if (args.front().is_dict_key())
    result1 = rpc::commands.call_command(args.front().as_dict_key().c_str(), args.front().as_dict_obj(), target1);
  else
    result1 = rpc::parse_command_single(target1, args.front().as_string());

  if (args.back().is_dict_key())
    result2 = rpc::commands.call_command(args.back().as_dict_key().c_str(), args.back().as_dict_obj(), target2);
  else
    result2 = rpc::parse_command_single(target2, args.back().as_string());

  if (result1.type() != result2.type())
    throw torrent::input_error("Type mismatch.");

  switch (result1.type()) {
  case torrent::Object::TYPE_VALUE:  return result1.as_value() - result2.as_value();
  case torrent::Object::TYPE_STRING: return result1.as_string().compare(result2.as_string());
  default: return torrent::Object();
  }
}

torrent::Object apply_less(rpc::target_type target, const torrent::Object::list_type& args) {
  torrent::Object result = apply_cmp(target, args);
  return result.is_value() ? result.as_value() <  0 : (int64_t)false;
}

torrent::Object apply_greater(rpc::target_type target, const torrent::Object::list_type& args) {
  torrent::Object result = apply_cmp(target, args);
  return result.is_value() ? result.as_value() >  0 : (int64_t)false;
}

torrent::Object apply_equal(rpc::target_type target, const torrent::Object::list_type& args) {
  torrent::Object result = apply_cmp(target, args);
  return result.is_value() ? result.as_value() == 0 : (int64_t)false;
}

torrent::Object
apply_compare(rpc::target_type target, const torrent::Object::list_type& args) {
  if (!rpc::is_target_pair(target))
    throw torrent::input_error("Can only compare a target pair.");

  if (args.size() < 2)
    throw torrent::input_error("Need at least order and one field.");

  torrent::Object::list_const_iterator itr = args.begin();
  std::string order = (itr++)->as_string();
  const char* current = order.c_str();

  torrent::Object result1;
  torrent::Object result2;

  for (torrent::Object::list_const_iterator last = args.end(); itr != last; itr++) {
    std::string field = itr->as_string();
    result1 = rpc::parse_command_single(rpc::get_target_left(target), field);
    result2 = rpc::parse_command_single(rpc::get_target_right(target), field);

    if (result1.type() != result2.type())
      throw torrent::input_error(std::string("Type mismatch in compare of ") + field);

    bool descending = *current == 'd' || *current == 'D' || *current == '-';
    if (*current) {
      if (!descending && !(*current == 'a' || *current == 'A' || *current == '+'))
        throw torrent::input_error(std::string("Bad order '") + *current + "' in " + order);
      ++current;
    }

    switch (result1.type()) {
      case torrent::Object::TYPE_VALUE:
        if (result1.as_value() != result2.as_value())
          return (int64_t) (descending ^ (result1.as_value() < result2.as_value()));
        break;

      case torrent::Object::TYPE_STRING:
        if (result1.as_string() != result2.as_string())
          return (int64_t) (descending ^ (result1.as_string() < result2.as_string()));
        break;

      default:
        break; // treat unknown types as equal
    }
  }

  // if all else is equal, ensure stable sort order based on memory location
  return (int64_t) (target.second < target.third);
}

// Regexp based 'match' function.
// arg1: the text to match.
// arg2: the regexp pattern.
// eg: match{d.name=,.*linux.*iso}
torrent::Object apply_match(rpc::target_type target, const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count for 'match': 2 arguments needed.");

  // This really should be converted to using args flagged as
  // commands, so that we can compare commands and statics values.

  torrent::Object result1;
  torrent::Object result2;

  rpc::target_type target1 = rpc::is_target_pair(target) ? rpc::get_target_left(target) : target;
  rpc::target_type target2 = rpc::is_target_pair(target) ? rpc::get_target_right(target) : target;

  if (args.front().is_dict_key())
    result1 = rpc::commands.call_command(args.front().as_dict_key().c_str(), args.front().as_dict_obj(), target1);
  else
    result1 = rpc::parse_command_single(target1, args.front().as_string());

  if (args.back().is_dict_key())
    result2 = rpc::commands.call_command(args.back().as_dict_key().c_str(), args.back().as_dict_obj(), target2);
  else
    result2 = args.back().as_string();

  if (result1.type() != result2.type())
    throw torrent::input_error("Type mismatch for 'match' arguments.");

  std::string text = result1.as_string();
  std::string pattern = result2.as_string();

  std::transform(text.begin(), text.end(), text.begin(), ::tolower);
  std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::tolower);

  bool isAMatch = false;
  try {
    std::regex re(pattern);
    isAMatch = std::regex_match(text, re);
  } catch (const std::regex_error& exc) {
    control->core()->push_log_std("regex_error: " + std::string(exc.what()));
  }

  return isAMatch ? (int64_t)true : (int64_t)false;
}

torrent::Object
apply_to_time(const torrent::Object& rawArgs, int flags) {
  std::tm *u;
  time_t t = (uint64_t)rawArgs.as_value();

  if (flags & 0x1)
    u = std::localtime(&t);
  else
    u = std::gmtime(&t);

  if (u == NULL)
    return torrent::Object();

  char buffer[11];

  if (flags & 0x2)
    snprintf(buffer, 11, "%02u/%02u/%04u", u->tm_mday, (u->tm_mon + 1), (1900 + u->tm_year));
  else
    snprintf(buffer, 9, "%2d:%02d:%02d", u->tm_hour, u->tm_min, u->tm_sec);

  return std::string(buffer);
}

torrent::Object
apply_to_elapsed_time(const torrent::Object& rawArgs) {
  auto cached_seconds = torrent::this_thread::cached_seconds().count();

  uint64_t arg = cached_seconds - rawArgs.as_value();

  char buffer[48];
  snprintf(buffer, 48, "%2d:%02d:%02d", (int)(arg / 3600), (int)((arg / 60) % 60), (int)(arg % 60));

  return std::string(buffer);
}

torrent::Object
apply_to_kb(const torrent::Object& rawArgs) {
  char buffer[32];
  snprintf(buffer, 32, "%5.1f", (double)rawArgs.as_value() / (1 << 10));

  return std::string(buffer);
}

torrent::Object
apply_to_mb(const torrent::Object& rawArgs) {
  char buffer[32];
  snprintf(buffer, 32, "%8.1f", (double)rawArgs.as_value() / (1 << 20));

  return std::string(buffer);
}

torrent::Object
apply_to_xb(const torrent::Object& rawArgs) {
  char buffer[48];
  int64_t arg = rawArgs.as_value();

  if (arg < (int64_t(1000) << 10))
    snprintf(buffer, 48, "%5.1f KB", (double)arg / (int64_t(1) << 10));
  else if (arg < (int64_t(1000) << 20))
    snprintf(buffer, 48, "%5.1f MB", (double)arg / (int64_t(1) << 20));
  else if (arg < (int64_t(1000) << 30))
    snprintf(buffer, 48, "%5.1f GB", (double)arg / (int64_t(1) << 30));
  else
    snprintf(buffer, 48, "%5.1f TB", (double)arg / (int64_t(1) << 40));

  return std::string(buffer);
}

torrent::Object
apply_to_throttle(const torrent::Object& rawArgs) {
  int64_t arg = rawArgs.as_value();
  if (arg < 0)
    return "---";
  else if (arg == 0)
    return "off";

  char buffer[32];
  snprintf(buffer, 32, "%3d", (int)(arg / (1 << 10)));
  return std::string(buffer);
}

// A series of if/else statements. Every even arguments are
// conditionals and odd arguments are branches to be executed, except
// the last one which is always a branch.
//
// if (cond1) { branch1 }
// <cond1>,<branch1>
//
// if (cond1) { branch1 } else if (cond2) { branch2 } else { branch3 }
// <cond1>,<branch1>,<cond2>,<branch2>,<branch3>
torrent::Object
apply_if(rpc::target_type target, const torrent::Object& rawArgs, int flags) {
  const torrent::Object::list_type& args = rawArgs.as_list();
  torrent::Object::list_const_iterator itr = args.begin();

  while (itr != args.end() && itr != --args.end()) {
    torrent::Object tmp;
    const torrent::Object* conditional;

    if (flags & 0x1 && itr->is_string())
      conditional = &(tmp = rpc::parse_command(target, itr->as_string().c_str(), itr->as_string().c_str() + itr->as_string().size()).first);
    else if (flags & 0x1 && itr->is_dict_key())
      conditional = &(tmp = rpc::commands.call_command(itr->as_dict_key().c_str(), itr->as_dict_obj(), target));
    else
      conditional = &*itr;

    bool result;

    switch (conditional->type()) {
    case torrent::Object::TYPE_STRING:
      result = !conditional->as_string().empty();
      break;
    case torrent::Object::TYPE_VALUE:
      result = conditional->as_value();
      break;
    case torrent::Object::TYPE_NONE:
      result = false;
      break;
    default:
      throw torrent::input_error("Type not supported by 'if'.");
    };

    itr++;

    if (result)
      break;

    itr++;
  }

  if (itr == args.end())
    return torrent::Object();

  if (flags & 0x1 && itr->is_string()) {
    return rpc::parse_command(target, itr->as_string().c_str(), itr->as_string().c_str() + itr->as_string().size()).first;

  } else if (flags & 0x1 && itr->is_dict_key()) {
    return rpc::commands.call_command(itr->as_dict_key().c_str(), itr->as_dict_obj(), target);

  } else if (flags & 0x1 && itr->is_list()) {
    // Move this into a special function or something. Also, might be
    // nice to have a parse_command function that takes list
    // iterator...

    for (const auto& cmdItr : itr->as_list())
      if (cmdItr.is_string())
        rpc::parse_command(target, cmdItr.as_string().c_str(), cmdItr.as_string().c_str() + cmdItr.as_string().size());

    return torrent::Object();

  } else {
    return *itr;
  }
}

torrent::Object
cmd_view_size(const torrent::Object::string_type& args) {
  return (*control->view_manager()->find_throw(args))->size_visible();
}

torrent::Object
cmd_view_size_not_visible(const torrent::Object::string_type& args) {
  return (*control->view_manager()->find_throw(args))->size_not_visible();
}

torrent::Object
cmd_view_persistent(const torrent::Object::string_type& args) {
  core::View* view = *control->view_manager()->find_throw(args);

  if (!view->get_filter().is_empty() || !view->event_added().is_empty() || !view->event_removed().is_empty())
    throw torrent::input_error("Cannot set modified views as persitent.");

  view->set_filter("d.views.has=" + args);
  view->set_event_added("d.views.push_back_unique=" + args);
  view->set_event_removed("d.views.remove=" + args);

  return torrent::Object();
}

// TODO: These don't need wrapper functions anymore...
torrent::Object
cmd_ui_set_view(const torrent::Object::string_type& args) {
  control->ui()->download_list()->set_current_view(args);
  return torrent::Object();
}

torrent::Object
cmd_ui_current_view() {
  return control->ui()->download_list()->current_view()->name();
}

torrent::Object
cmd_ui_unfocus_download(core::Download* download) {
  control->ui()->download_list()->unfocus_download(download);

  return torrent::Object();
}

torrent::Object
cmd_view_filter_download(core::Download* download, const torrent::Object::string_type& args) {
  (*control->view_manager()->find_throw(args))->filter_download(download);

  return torrent::Object();
}

torrent::Object
cmd_view_set_visible(core::Download* download, const torrent::Object::string_type& args) {
  (*control->view_manager()->find_throw(args))->set_visible(download);

  return torrent::Object();
}

torrent::Object
cmd_view_set_not_visible(core::Download* download, const torrent::Object::string_type& args) {
  (*control->view_manager()->find_throw(args))->set_not_visible(download);

  return torrent::Object();
}

torrent::Object
apply_elapsed_less(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  int64_t start_time = rpc::convert_to_value(args.front());
  auto cached_seconds = torrent::this_thread::cached_seconds().count();

  return (int64_t)(start_time != 0 && cached_seconds - start_time < rpc::convert_to_value(args.back()));
}

torrent::Object
apply_elapsed_greater(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Wrong argument count.");

  int64_t start_time = rpc::convert_to_value(args.front());
  auto cached_seconds = torrent::this_thread::cached_seconds().count();

  return (int64_t)(start_time != 0 && cached_seconds - start_time > rpc::convert_to_value(args.back()));
}

inline std::vector<int64_t>
as_vector(const torrent::Object::list_type& args) {
  if (args.size() == 0)
    throw torrent::input_error("Wrong argument count in as_vector.");

  std::vector<int64_t> result;

  for (const auto& arg : args) {

    if (arg.is_value()) {
      result.push_back(arg.as_value());
    } else if (arg.is_string()) {
      result.push_back(rpc::convert_to_value(arg.as_string()));
    } else if (arg.is_list()) {
      std::vector<int64_t> subResult = as_vector(arg.as_list());
      result.insert(result.end(), subResult.begin(), subResult.end());
    } else {
      throw torrent::input_error("Wrong type supplied to as_vector.");
    }
  }

  return result;
}

template <typename Comp>
int64_t
apply_math_basic(const char* name, Comp op, const torrent::Object::list_type& args) {
  int64_t val = 0, rhs = 0;
  bool divides = !strcmp(name, "math.div") || !strcmp(name, "math.mod");

  if (args.size() == 0)
    throw torrent::input_error(std::string(name) + ": No arguments provided!");

  for (torrent::Object::list_const_iterator itr = args.begin(), last = args.end(); itr != last; itr++) {

    if (itr->is_value()) {
      rhs = itr->as_value();
    } else if (itr->is_string()) {
      rhs = rpc::convert_to_value(itr->as_string());
    } else if (itr->is_list()) {
      rhs = apply_math_basic(name, op, itr->as_list());
    } else {
      throw torrent::input_error(std::string(name) + ": Wrong argument type");
    }

    if (divides && !rhs && itr != args.begin())
      throw torrent::input_error(std::string(name) + ": Division by zero!");

    val = itr == args.begin() ? rhs : op(val, rhs);

  }

  return val;
}

template <typename Comp>
int64_t
apply_arith_basic(Comp op, const torrent::Object::list_type& args) {
  if (args.size() == 0)
    throw torrent::input_error("Wrong argument count in apply_arith_basic.");

  int64_t val = 0;

  for (torrent::Object::list_const_iterator itr = args.begin(), last = args.end(); itr != last; itr++) {

    if (itr->is_value()) {
      val = itr == args.begin() ? itr->as_value() : (op(val, itr->as_value()) ? val : itr->as_value());
    } else if (itr->is_string()) {
      int64_t cval = rpc::convert_to_value(itr->as_string());
      val = itr == args.begin() ? cval : (op(val, cval) ? val : cval);
    } else if (itr->is_list()) {
      int64_t fval = apply_arith_basic(op, itr->as_list());
      val = itr == args.begin() ? fval : (op(val, fval) ? val : fval);
    } else {
      throw torrent::input_error("Wrong type supplied to apply_arith_basic.");
    }

  }

  return val;
}

int64_t
apply_arith_count(const torrent::Object::list_type& args) {
  if (args.size() == 0)
    throw torrent::input_error("Wrong argument count in apply_arith_count.");

  int64_t val = 0;

  for (const auto& arg : args) {

    switch (arg.type()) {
    case torrent::Object::TYPE_VALUE:
    case torrent::Object::TYPE_STRING:
      val++;
      break;
    case torrent::Object::TYPE_LIST:
      val += apply_arith_count(arg.as_list());
      break;
    default:
      throw torrent::input_error("Wrong type supplied to apply_arith_count.");
    }
  }

  return val;
}

int64_t
apply_arith_other(const char* op, const torrent::Object::list_type& args) {
  if (args.size() == 0)
    throw torrent::input_error("Wrong argument count in apply_arith_other.");

  if (strcmp(op, "average") == 0) {
    return (int64_t)(apply_math_basic(op, std::plus<int64_t>(), args) / apply_arith_count(args));
  } else if (strcmp(op, "median") == 0) {
    std::vector<int64_t> result = as_vector(args);
    return (int64_t)rak::median(result.begin(), result.end());
  } else {
    throw torrent::input_error("Wrong operation supplied to apply_arith_other.");
  }
}

torrent::Object
cmd_status_throttle_names(bool up, const torrent::Object::list_type& args) {
  if (args.size() == 0)
    return torrent::Object();

  std::vector<std::string> throttle_name_list;

  for (const auto& arg : args) {
    if (arg.is_string())
      throttle_name_list.push_back(arg.as_string());
  }

  if (up)
    control->ui()->set_status_throttle_up_names(throttle_name_list);
  else
    control->ui()->set_status_throttle_down_names(throttle_name_list);

  return torrent::Object();
}

torrent::Object
apply_set_color(int color_id, const torrent::Object::string_type& color_str) {
  control->object_storage()->set_str_string(display::color_vars[color_id], color_str);

  display::Canvas::build_colors();
  return torrent::Object();
}

void
initialize_command_ui() {
  CMD2_VAR_STRING("keys.layout", "qwerty");

  CMD2_ANY_STRING("view.add", object_convert_void(std::bind(&core::ViewManager::insert_throw, control->view_manager(), std::placeholders::_2)));

  CMD2_ANY_L   ("view.list",          std::bind(&apply_view_list));
  CMD2_ANY_LIST("view.set",           std::bind(&apply_view_set, std::placeholders::_2));

  CMD2_ANY_LIST  ("view.filter",      std::bind(&apply_view_event, &core::ViewManager::set_filter, std::placeholders::_2));
  CMD2_ANY_LIST  ("view.filter_on",   std::bind(&apply_view_filter_on, std::placeholders::_2));
  CMD2_ANY_LIST  ("view.filter.temp", std::bind(&apply_view_event, &core::ViewManager::set_filter_temp, std::placeholders::_2));
  CMD2_VAR_STRING("view.filter.temp.excluded", "default,started,stopped");
  CMD2_VAR_BOOL  ("view.filter.temp.log", 0);

  CMD2_ANY_LIST("view.sort",          std::bind(&apply_view_sort, std::placeholders::_2));
  CMD2_ANY_LIST("view.sort_new",      std::bind(&apply_view_event, &core::ViewManager::set_sort_new, std::placeholders::_2));
  CMD2_ANY_LIST("view.sort_current",  std::bind(&apply_view_event, &core::ViewManager::set_sort_current, std::placeholders::_2));

  CMD2_ANY_LIST("view.event_added",   std::bind(&apply_view_event, &core::ViewManager::set_event_added, std::placeholders::_2));
  CMD2_ANY_LIST("view.event_removed", std::bind(&apply_view_event, &core::ViewManager::set_event_removed, std::placeholders::_2));

  // Cleanup and add . to view.

  CMD2_ANY_STRING("view.size",              std::bind(&cmd_view_size, std::placeholders::_2));
  CMD2_ANY_STRING("view.size_not_visible",  std::bind(&cmd_view_size_not_visible, std::placeholders::_2));
  CMD2_ANY_STRING("view.persistent",        std::bind(&cmd_view_persistent, std::placeholders::_2));

  CMD2_ANY_STRING_V("view.filter_all",      std::bind(&core::View::filter, std::bind(&core::ViewManager::find_ptr_throw, control->view_manager(), std::placeholders::_2)));

  CMD2_DL_STRING ("view.filter_download", std::bind(&cmd_view_filter_download, std::placeholders::_1, std::placeholders::_2));
  CMD2_DL_STRING ("view.set_visible",     std::bind(&cmd_view_set_visible,     std::placeholders::_1, std::placeholders::_2));
  CMD2_DL_STRING ("view.set_not_visible", std::bind(&cmd_view_set_not_visible, std::placeholders::_1, std::placeholders::_2));

  // Commands that affect the default rtorrent UI.
  CMD2_DL        ("ui.unfocus_download",   std::bind(&cmd_ui_unfocus_download, std::placeholders::_1));
  CMD2_ANY       ("ui.current_view",       std::bind(&cmd_ui_current_view));
  CMD2_ANY_STRING("ui.current_view.set",   std::bind(&cmd_ui_set_view, std::placeholders::_2));

  CMD2_ANY        ("ui.input.history.size",     std::bind(&ui::Root::get_input_history_size, control->ui()));
  CMD2_ANY_VALUE_V("ui.input.history.size.set", std::bind(&ui::Root::set_input_history_size, control->ui(), std::placeholders::_2));
  CMD2_ANY_V      ("ui.input.history.clear",    std::bind(&ui::Root::clear_input_history, control->ui()));

  CMD2_VAR_VALUE ("ui.throttle.global.step.small",  5);
  CMD2_VAR_VALUE ("ui.throttle.global.step.medium", 50);
  CMD2_VAR_VALUE ("ui.throttle.global.step.large",  500);

  CMD2_VAR_VALUE ("ui.focus.page_size", 0);

  CMD2_ANY_LIST  ("ui.status.throttle.up.set",   std::bind(&cmd_status_throttle_names, true, std::placeholders::_2));
  CMD2_ANY_LIST  ("ui.status.throttle.down.set", std::bind(&cmd_status_throttle_names, false, std::placeholders::_2));

  CMD2_ANY         ("ui.keymap.style",     std::bind(&ui::Root::keymap_style, control->ui()));
  CMD2_ANY_STRING_V("ui.keymap.style.set", std::bind(&ui::Root::set_keymap_style, control->ui(), std::placeholders::_2));

  // TODO: Add 'option_string' for rtorrent-specific options.
  CMD2_VAR_STRING("ui.torrent_list.layout", "full");

  // Move.
  CMD2_ANY("print", &apply_print);
  CMD2_ANY("cat",   &apply_cat);
  CMD2_ANY_LIST("value", &apply_value);
  CMD2_ANY("try",   &apply_try);
  CMD2_ANY("if",    std::bind(&apply_if, std::placeholders::_1, std::placeholders::_2, 0));
  CMD2_ANY("not",   &apply_not);
  CMD2_ANY("false", &apply_false);
  CMD2_ANY("and",   &apply_and);
  CMD2_ANY("or",    &apply_or);

  // A temporary command for handling stuff until we get proper
  // support for seperation of commands and literals.
  CMD2_ANY("branch", std::bind(&apply_if, std::placeholders::_1, std::placeholders::_2, 1));

  CMD2_ANY_LIST("less",    &apply_less);
  CMD2_ANY_LIST("greater", &apply_greater);
  CMD2_ANY_LIST("equal",   &apply_equal);
  CMD2_ANY_LIST("compare", &apply_compare);
  CMD2_ANY_LIST("match",   &apply_match);

  CMD2_ANY_VALUE("convert.gm_time",      std::bind(&apply_to_time, std::placeholders::_2, 0));
  CMD2_ANY_VALUE("convert.gm_date",      std::bind(&apply_to_time, std::placeholders::_2, 0x2));
  CMD2_ANY_VALUE("convert.time",         std::bind(&apply_to_time, std::placeholders::_2, 0x1));
  CMD2_ANY_VALUE("convert.date",         std::bind(&apply_to_time, std::placeholders::_2, 0x1 | 0x2));
  CMD2_ANY_VALUE("convert.elapsed_time", std::bind(&apply_to_elapsed_time, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.kb",           std::bind(&apply_to_kb, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.mb",           std::bind(&apply_to_mb, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.xb",           std::bind(&apply_to_xb, std::placeholders::_2));
  CMD2_ANY_VALUE("convert.throttle",     std::bind(&apply_to_throttle, std::placeholders::_2));

  CMD2_ANY_LIST("math.add",              [](auto, const auto& args) { return apply_math_basic("math.add", std::plus(), args); });
  CMD2_ANY_LIST("math.sub",              [](auto, const auto& args) { return apply_math_basic("math.sub", std::minus(), args); });
  CMD2_ANY_LIST("math.mul",              [](auto, const auto& args) { return apply_math_basic("math.mul", std::multiplies(), args); });
  CMD2_ANY_LIST("math.div",              [](auto, const auto& args) { return apply_math_basic("math.div", std::divides(), args); });
  CMD2_ANY_LIST("math.mod",              [](auto, const auto& args) { return apply_math_basic("math.mod", std::modulus(), args); });
  CMD2_ANY_LIST("math.min",              [](auto, const auto& args) { return apply_arith_basic(std::less(), args); });
  CMD2_ANY_LIST("math.max",              [](auto, const auto& args) { return apply_arith_basic(std::greater(), args); });
  CMD2_ANY_LIST("math.cnt",              std::bind(&apply_arith_count, std::placeholders::_2));
  CMD2_ANY_LIST("math.avg",              std::bind(&apply_arith_other, "average", std::placeholders::_2));
  CMD2_ANY_LIST("math.med",              std::bind(&apply_arith_other, "median", std::placeholders::_2));

  CMD2_ANY_LIST ("elapsed.less",         std::bind(&apply_elapsed_less, std::placeholders::_2));
  CMD2_ANY_LIST ("elapsed.greater",      std::bind(&apply_elapsed_greater, std::placeholders::_2));

  // Build set/get methods for all color definitions
  for (int color_id = 1; color_id < display::RCOLOR_MAX; color_id++) {
    control->object_storage()->insert_str(display::color_vars[color_id], "", rpc::object_storage::flag_string_type);

    CMD2_ANY_STRING(std::string(display::color_vars[color_id]) + ".set", [color_id](const auto&, const auto& arg) {
      return apply_set_color(color_id, arg);
    });

    CMD2_ANY(display::color_vars[color_id], [color_id](const auto&, const auto&) {
      return control->object_storage()->get_str(display::color_vars[color_id]);
    });
  }
}
