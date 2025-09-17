#include "config.h"

#include <cstdio>
#include <torrent/throttle.h>
#include <torrent/rate.h>
#include <torrent/download/resource_manager.h>
#include <torrent/net/socket_address.h>

#include "core/manager.h"
#include "ui/root.h"
#include "rak/address_info.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

std::pair<uint32_t, uint32_t>
parse_address_range(const torrent::Object::list_type& args, torrent::Object::list_type::const_iterator itr) {
  unsigned int prefixWidth, ret;
  char dummy;
  char host[1024];
  rak::address_info* ai;

  ret = std::sscanf(itr->as_string().c_str(), "%1023[^/]/%d%c", host, &prefixWidth, &dummy);

  if (ret < 1 || rak::address_info::get_address_info(host, PF_INET, SOCK_STREAM, &ai) != 0)
    throw torrent::input_error("Could not resolve host.");

  uint32_t begin, end;
  auto sa = torrent::sa_copy(ai->c_addrinfo()->ai_addr);
  auto sa_addr = htonl(reinterpret_cast<sockaddr_in*>(sa.get())->sin_addr.s_addr);

  begin = end = sa_addr;

  rak::address_info::free_address_info(ai);

  if (ret == 2) {
    if (++itr != args.end())
      throw torrent::input_error("Cannot specify both network and range end.");

    uint32_t netmask = std::numeric_limits<uint32_t>::max() << (32 - prefixWidth);

    if (prefixWidth >= 32 || sa_addr & ~netmask)
      throw torrent::input_error("Invalid address/prefix.");

    end = sa_addr | ~netmask;

  } else if (++itr != args.end()) {
    if (rak::address_info::get_address_info(itr->as_string().c_str(), PF_INET, SOCK_STREAM, &ai) != 0)
      throw torrent::input_error("Could not resolve host.");

    sa = torrent::sa_copy(ai->c_addrinfo()->ai_addr);
    sa_addr = htonl(reinterpret_cast<sockaddr_in*>(sa.get())->sin_addr.s_addr);

    rak::address_info::free_address_info(ai);
    end = sa_addr;
  }

  // convert to [begin, end) making sure the end doesn't overflow
  // (this precludes 255.255.255.255 from ever matching, but that's not a real IP anyway)
  return std::make_pair((uint32_t)begin, (uint32_t)std::max(end, end + 1));
}

torrent::Object
apply_throttle(const torrent::Object::list_type& args, bool up) {
  torrent::Object::list_const_iterator argItr = args.begin();

  if (argItr == args.end())
    throw torrent::input_error("Missing throttle name.");

  const std::string& name = argItr->as_string();
  if (name.empty() || name == "NULL")
    throw torrent::input_error("Invalid throttle name '" + name + "'.");

  if (++argItr == args.end() || argItr->as_string().empty())
    throw torrent::input_error("Missing throttle rate for '" + name + "'.");

  int64_t rate;
  rpc::parse_whole_value_nothrow(argItr->as_string().c_str(), &rate);

  if (rate < 0)
    throw torrent::input_error("Throttle rate must be non-negative.");

  core::ThrottleMap::iterator itr = control->core()->throttles().find(name);
  if (itr == control->core()->throttles().end())
    itr = control->core()->throttles().insert(std::make_pair(name, torrent::ThrottlePair(NULL, NULL))).first;

  torrent::Throttle*& throttle = up ? itr->second.first : itr->second.second;
  if (rate != 0 && throttle == NULL)
    throttle = (up ? torrent::up_throttle_global() : torrent::down_throttle_global())->create_slave();

  if (throttle != NULL)
    throttle->set_max_rate(rate * 1024);

  return torrent::Object();
}

static const int throttle_info_up   = (1 << 0);
static const int throttle_info_down = (1 << 1);
static const int throttle_info_max  = (1 << 2);
static const int throttle_info_rate = (1 << 3);

torrent::Object
retrieve_throttle_info(const torrent::Object::string_type& name, int flags) {
  core::ThrottleMap::iterator itr = control->core()->throttles().find(name);
  torrent::ThrottlePair throttles = itr == control->core()->throttles().end() ? torrent::ThrottlePair(NULL, NULL) : itr->second;
  torrent::Throttle* throttle = flags & throttle_info_down ? throttles.second : throttles.first;
  torrent::Throttle* global = flags & throttle_info_down ? torrent::down_throttle_global() : torrent::up_throttle_global();

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
apply_address_throttle(const torrent::Object::list_type& args) {
  if (args.size() < 2 || args.size() > 3)
    throw torrent::input_error("Incorrect number of arguments.");

  std::pair<uint32_t, uint32_t> range = parse_address_range(args, ++args.begin());
  core::ThrottleMap::iterator throttleItr = control->core()->throttles().find(args.begin()->as_string().c_str());
  if (throttleItr == control->core()->throttles().end())
    throw torrent::input_error("Throttle not found.");

  control->core()->set_address_throttle(range.first, range.second, throttleItr->second);
  return torrent::Object();
}

torrent::Object
throttle_update(const char* variable, int64_t value) {
  rpc::commands.call_command(variable, value);

  control->ui()->adjust_up_throttle(0);
  control->ui()->adjust_down_throttle(0);
  return torrent::Object();
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

  CMD2_REDIRECT_GENERIC("throttle.max_uploads.div",      "throttle.max_uploads.div._val");
  CMD2_REDIRECT_GENERIC("throttle.max_uploads.global",   "throttle.max_uploads.global._val");
  CMD2_REDIRECT_GENERIC("throttle.max_downloads.div",    "throttle.max_downloads.div._val");
  CMD2_REDIRECT_GENERIC("throttle.max_downloads.global", "throttle.max_downloads.global._val");

  CMD2_ANY_VALUE   ("throttle.max_uploads.div.set",      std::bind(&throttle_update, "throttle.max_uploads.div._val.set", std::placeholders::_2));
  CMD2_ANY_VALUE   ("throttle.max_uploads.global.set",   std::bind(&throttle_update, "throttle.max_uploads.global._val.set", std::placeholders::_2));
  CMD2_ANY_VALUE   ("throttle.max_downloads.div.set",    std::bind(&throttle_update, "throttle.max_downloads.div._val.set", std::placeholders::_2));
  CMD2_ANY_VALUE   ("throttle.max_downloads.global.set", std::bind(&throttle_update, "throttle.max_downloads.global._val.set", std::placeholders::_2));

  // TODO: Move the logic into some libtorrent function.
  CMD2_ANY         ("throttle.global_up.rate",              std::bind(&torrent::Rate::rate, torrent::up_rate()));
  CMD2_ANY         ("throttle.global_up.total",             std::bind(&torrent::Rate::total, torrent::up_rate()));
  CMD2_ANY         ("throttle.global_up.max_rate",          std::bind(&torrent::Throttle::max_rate, torrent::up_throttle_global()));
  CMD2_ANY_VALUE_V ("throttle.global_up.max_rate.set",      std::bind(&ui::Root::set_up_throttle_i64, control->ui(), std::placeholders::_2));
  CMD2_ANY_VALUE_KB("throttle.global_up.max_rate.set_kb",   std::bind(&ui::Root::set_up_throttle_i64, control->ui(), std::placeholders::_2));
  CMD2_ANY         ("throttle.global_down.rate",            std::bind(&torrent::Rate::rate, torrent::down_rate()));
  CMD2_ANY         ("throttle.global_down.total",           std::bind(&torrent::Rate::total, torrent::down_rate()));
  CMD2_ANY         ("throttle.global_down.max_rate",        std::bind(&torrent::Throttle::max_rate, torrent::down_throttle_global()));
  CMD2_ANY_VALUE_V ("throttle.global_down.max_rate.set",    std::bind(&ui::Root::set_down_throttle_i64, control->ui(), std::placeholders::_2));
  CMD2_ANY_VALUE_KB("throttle.global_down.max_rate.set_kb", std::bind(&ui::Root::set_down_throttle_i64, control->ui(), std::placeholders::_2));

  // Temporary names, need to change this to accept real rates rather
  // than kB.
  CMD2_ANY_LIST    ("throttle.up",                          std::bind(&apply_throttle, std::placeholders::_2, true));
  CMD2_ANY_LIST    ("throttle.down",                        std::bind(&apply_throttle, std::placeholders::_2, false));
  CMD2_ANY_LIST    ("throttle.ip",                          std::bind(&apply_address_throttle, std::placeholders::_2));

  CMD2_ANY_STRING  ("throttle.up.max",    std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_up | throttle_info_max));
  CMD2_ANY_STRING  ("throttle.up.rate",   std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_up | throttle_info_rate));
  CMD2_ANY_STRING  ("throttle.down.max",  std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_down | throttle_info_max));
  CMD2_ANY_STRING  ("throttle.down.rate", std::bind(&retrieve_throttle_info, std::placeholders::_2, throttle_info_down | throttle_info_rate));
}
