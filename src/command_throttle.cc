#include "config.h"

#include <limits>
#include <torrent/throttle.h>
#include <torrent/rate.h>
#include <torrent/download/resource_manager.h>

#include "core/manager.h"
#include "ui/root.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
apply_throttle(const torrent::Object::list_type& args, bool up) {
  auto arg_itr = args.begin();

  if (arg_itr == args.end())
    throw torrent::input_error("Missing throttle name.");

  const std::string& name = arg_itr->as_string();
  if (name.empty() || name == "NULL")
    throw torrent::input_error("Invalid throttle name '" + name + "'.");

  if (++arg_itr == args.end() || arg_itr->as_string().empty())
    throw torrent::input_error("Missing throttle rate for '" + name + "'.");

  int64_t rate;

  if (!rpc::parse_whole_value_nothrow(arg_itr->as_string().c_str(), &rate))
    throw torrent::input_error("Invalid throttle rate for '" + name + "'.");

  if (rate < 0)
    throw torrent::input_error("Throttle rate must be non-negative.");

  if (rate > (std::numeric_limits<int64_t>::max() >> 10))
    throw torrent::input_error("Throttle rate is too large.");

  auto itr = control->core()->throttles().find(name);

  if (itr == control->core()->throttles().end())
    itr = control->core()->throttles().insert(std::make_pair(name, core::ThrottlePair(nullptr, nullptr))).first;

  auto*& throttle = up ? itr->second.first : itr->second.second;

  if (rate != 0 && throttle == nullptr)
    throttle = (up ? torrent::up_throttle_global() : torrent::down_throttle_global())->create_slave();

  if (throttle != nullptr)
    throttle->set_max_rate(rate * 1024);

  return torrent::Object();
}

static const int throttle_info_up   = (1 << 0);
static const int throttle_info_down = (1 << 1);
static const int throttle_info_max  = (1 << 2);
static const int throttle_info_rate = (1 << 3);

torrent::Object
retrieve_throttle_info(const torrent::Object::string_type& name, int flags) {
  auto  itr       = control->core()->throttles().find(name);
  auto  throttles = (itr == control->core()->throttles().end()) ? core::ThrottlePair(nullptr, nullptr) : itr->second;
  auto* throttle  = flags & throttle_info_down ? throttles.second : throttles.first;
  auto* global    = flags & throttle_info_down ? torrent::down_throttle_global() : torrent::up_throttle_global();

  if (throttle == NULL && name.empty())
    throttle = global;

  if (throttle == NULL)
    return flags & throttle_info_rate ? (int64_t)0 : (int64_t)-1;
  else if (!throttle->is_throttled() || !global->is_throttled())
    return (int64_t)0;
  else if (flags & throttle_info_rate)
    return (int64_t)throttle->rate()->rate();
  else
    return (int64_t)throttle->max_rate();
}

torrent::Object
throttle_update(const char* variable, int64_t value) {
  rpc::commands.call_command(variable, value);

  control->ui()->adjust_up_throttle(0);
  control->ui()->adjust_down_throttle(0);
  return torrent::Object();
}

static unsigned int
throttle_rate_to_kb(int64_t rate) {
  if (rate < 0 || rate > std::numeric_limits<unsigned int>::max() - 1)
    throw torrent::input_error("Throttle rate must be between 0 and 4294967294.");

  return static_cast<unsigned int>(rate >> 10);
}

static void
set_up_throttle_i64(ui::Root* root, int64_t rate) {
  root->set_up_throttle(throttle_rate_to_kb(rate));
}

static void
set_down_throttle_i64(ui::Root* root, int64_t rate) {
  root->set_down_throttle(throttle_rate_to_kb(rate));
}

void
initialize_command_throttle() {
  CMD2_ANY         ("throttle.unchoked_uploads",       std::bind(&torrent::ResourceManager::currently_upload_unchoked, torrent::resource_manager()));
  CMD2_ANY         ("throttle.max_unchoked_uploads",   std::bind(&torrent::ResourceManager::max_upload_unchoked, torrent::resource_manager()));
  CMD2_ANY         ("throttle.unchoked_downloads",     std::bind(&torrent::ResourceManager::currently_download_unchoked, torrent::resource_manager()));
  CMD2_ANY         ("throttle.max_unchoked_downloads", std::bind(&torrent::ResourceManager::max_download_unchoked, torrent::resource_manager()));

  CMD2_VAR_VALUE   ("throttle.min_peers.normal", 100);
  CMD2_VAR_VALUE   ("throttle.max_peers.normal", 200);
  CMD2_VAR_VALUE   ("throttle.min_peers.seed",   -1);
  CMD2_VAR_VALUE   ("throttle.max_peers.seed",   -1);

  CMD2_VAR_VALUE   ("throttle.min_uploads",      0);
  CMD2_VAR_VALUE   ("throttle.max_uploads",      50);
  CMD2_VAR_VALUE   ("throttle.min_downloads",    0);
  CMD2_VAR_VALUE   ("throttle.max_downloads",    50);

  CMD2_VAR_VALUE   ("throttle.max_uploads.div._val",      1);
  CMD2_VAR_VALUE   ("throttle.max_uploads.global._val",   0);
  CMD2_VAR_VALUE   ("throttle.max_downloads.div._val",    1);
  CMD2_VAR_VALUE   ("throttle.max_downloads.global._val", 0);

  CMD2_REDIRECT    ("throttle.max_uploads.div",      "throttle.max_uploads.div._val");
  CMD2_REDIRECT    ("throttle.max_uploads.global",   "throttle.max_uploads.global._val");
  CMD2_REDIRECT    ("throttle.max_downloads.div",    "throttle.max_downloads.div._val");
  CMD2_REDIRECT    ("throttle.max_downloads.global", "throttle.max_downloads.global._val");

  CMD2_ANY_VALUE   ("throttle.max_uploads.div.set",      std::bind(&throttle_update, "throttle.max_uploads.div._val.set", std::placeholders::_2));
  CMD2_ANY_VALUE   ("throttle.max_uploads.global.set",   std::bind(&throttle_update, "throttle.max_uploads.global._val.set", std::placeholders::_2));
  CMD2_ANY_VALUE   ("throttle.max_downloads.div.set",    std::bind(&throttle_update, "throttle.max_downloads.div._val.set", std::placeholders::_2));
  CMD2_ANY_VALUE   ("throttle.max_downloads.global.set", std::bind(&throttle_update, "throttle.max_downloads.global._val.set", std::placeholders::_2));

  // TODO: Move the logic into some libtorrent function.
  CMD2_ANY         ("throttle.global_up.rate",              std::bind(&torrent::Rate::rate, torrent::up_rate()));
  CMD2_ANY         ("throttle.global_up.total",             std::bind(&torrent::Rate::total, torrent::up_rate()));
  CMD2_ANY         ("throttle.global_up.max_rate",          std::bind(&torrent::Throttle::max_rate, torrent::up_throttle_global()));
  CMD2_ANY_VALUE_V ("throttle.global_up.max_rate.set",      std::bind(&set_up_throttle_i64, control->ui(), std::placeholders::_2));
  CMD2_ANY_VALUE_KB("throttle.global_up.max_rate.set_kb",   std::bind(&set_up_throttle_i64, control->ui(), std::placeholders::_2));
  CMD2_ANY         ("throttle.global_down.rate",            std::bind(&torrent::Rate::rate, torrent::down_rate()));
  CMD2_ANY         ("throttle.global_down.total",           std::bind(&torrent::Rate::total, torrent::down_rate()));
  CMD2_ANY         ("throttle.global_down.max_rate",        std::bind(&torrent::Throttle::max_rate, torrent::down_throttle_global()));
  CMD2_ANY_VALUE_V ("throttle.global_down.max_rate.set",    std::bind(&set_down_throttle_i64, control->ui(), std::placeholders::_2));
  CMD2_ANY_VALUE_KB("throttle.global_down.max_rate.set_kb", std::bind(&set_down_throttle_i64, control->ui(), std::placeholders::_2));

  // Temporary names, need to change this to accept real rates rather
  // than kB.
  CMD2_ANY_LIST    ("throttle.up",                          std::bind(&apply_throttle, std::placeholders::_2, true));
  CMD2_ANY_LIST    ("throttle.down",                        std::bind(&apply_throttle, std::placeholders::_2, false));

  CMD2_ANY_STRING  ("throttle.up.max",    std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_up | throttle_info_max));
  CMD2_ANY_STRING  ("throttle.up.rate",   std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_up | throttle_info_rate));
  CMD2_ANY_STRING  ("throttle.down.max",  std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_down | throttle_info_max));
  CMD2_ANY_STRING  ("throttle.down.rate", std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_down | throttle_info_rate));

  rpc::rpc.mark_safe("throttle.unchoked_uploads");
  rpc::rpc.mark_safe("throttle.max_unchoked_uploads");
  rpc::rpc.mark_safe("throttle.unchoked_downloads");
  rpc::rpc.mark_safe("throttle.max_unchoked_downloads");

  rpc::rpc.mark_safe("throttle.min_peers.normal");
  rpc::rpc.mark_safe("throttle.min_peers.normal.set");
  rpc::rpc.mark_safe("throttle.max_peers.normal");
  rpc::rpc.mark_safe("throttle.max_peers.normal.set");
  rpc::rpc.mark_safe("throttle.min_peers.seed");
  rpc::rpc.mark_safe("throttle.min_peers.seed.set");
  rpc::rpc.mark_safe("throttle.max_peers.seed");
  rpc::rpc.mark_safe("throttle.max_peers.seed.set");

  rpc::rpc.mark_safe("throttle.min_uploads");
  rpc::rpc.mark_safe("throttle.min_uploads.set");
  rpc::rpc.mark_safe("throttle.max_uploads");
  rpc::rpc.mark_safe("throttle.max_uploads.set");
  rpc::rpc.mark_safe("throttle.min_downloads");
  rpc::rpc.mark_safe("throttle.min_downloads.set");
  rpc::rpc.mark_safe("throttle.max_downloads");
  rpc::rpc.mark_safe("throttle.max_downloads.set");

  rpc::rpc.mark_safe("throttle.max_uploads.div");
  rpc::rpc.mark_safe("throttle.max_uploads.div.set");
  rpc::rpc.mark_safe("throttle.max_uploads.div._val");
  rpc::rpc.mark_safe("throttle.max_uploads.div._val.set");
  rpc::rpc.mark_safe("throttle.max_uploads.global");
  rpc::rpc.mark_safe("throttle.max_uploads.global.set");
  rpc::rpc.mark_safe("throttle.max_uploads.global._val");
  rpc::rpc.mark_safe("throttle.max_uploads.global._val.set");
  rpc::rpc.mark_safe("throttle.max_downloads.div");
  rpc::rpc.mark_safe("throttle.max_downloads.div.set");
  rpc::rpc.mark_safe("throttle.max_downloads.div._val");
  rpc::rpc.mark_safe("throttle.max_downloads.div._val.set");
  rpc::rpc.mark_safe("throttle.max_downloads.global");
  rpc::rpc.mark_safe("throttle.max_downloads.global.set");
  rpc::rpc.mark_safe("throttle.max_downloads.global._val");
  rpc::rpc.mark_safe("throttle.max_downloads.global._val.set");

  rpc::rpc.mark_safe("throttle.global_up.rate");
  rpc::rpc.mark_safe("throttle.global_up.total");
  rpc::rpc.mark_safe("throttle.global_up.max_rate");
  rpc::rpc.mark_safe("throttle.global_up.max_rate.set");
  rpc::rpc.mark_safe("throttle.global_up.max_rate.set_kb");
  rpc::rpc.mark_safe("throttle.global_down.rate");
  rpc::rpc.mark_safe("throttle.global_down.total");
  rpc::rpc.mark_safe("throttle.global_down.max_rate");
  rpc::rpc.mark_safe("throttle.global_down.max_rate.set");
  rpc::rpc.mark_safe("throttle.global_down.max_rate.set_kb");

  rpc::rpc.mark_safe("throttle.up.max");
  rpc::rpc.mark_safe("throttle.up.rate");
  rpc::rpc.mark_safe("throttle.down.max");
  rpc::rpc.mark_safe("throttle.down.rate");
}
