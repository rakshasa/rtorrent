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

#ifndef RTORRENT_OPTION_HANDLER_RULES_H
#define RTORRENT_OPTION_HANDLER_RULES_H

#include <cstdio>
#include <stdexcept>
#include <sigc++/bind.h>

#include "option_handler.h"
#include "core/download_slot_map.h"

namespace ui {
  class Control;
}

bool validate_ip(const std::string& arg);
bool validate_directory(const std::string& arg);
bool validate_port_range(const std::string& arg);
bool validate_yes_no(const std::string& arg);

bool validate_download_peers(int arg);
bool validate_rate(int arg);
bool validate_read_ahead(int arg);
bool validate_fd(int arg);

bool validate_throttle_interval(int arg);

void apply_download_min_peers(ui::Control* m, int arg);
void apply_download_max_peers(ui::Control* m, int arg);
void apply_download_max_uploads(ui::Control* m, int arg);

void apply_download_directory(ui::Control* m, const std::string& arg);

void apply_global_download_rate(ui::Control* m, int arg);
void apply_global_upload_rate(ui::Control* m, int arg);

void apply_hash_read_ahead(ui::Control* m, int arg);
void apply_max_open_files(ui::Control* m, int arg);
void apply_throttle_interval(ui::Control* m, int arg);

void apply_ip(ui::Control* m, const std::string& arg);
void apply_bind(ui::Control* m, const std::string& arg);
void apply_port_range(ui::Control* m, const std::string& arg);
void apply_port_random(ui::Control* m, const std::string& arg);
void apply_tracker_dump(ui::Control* m, const std::string& arg);
void apply_check_hash(ui::Control* m, const std::string& arg);

void apply_session_directory(ui::Control* m, const std::string& arg);

class OptionHandlerInt : public OptionHandlerBase {
public:
  typedef bool (*Validate)(int);
  typedef void (*Apply)(ui::Control*, int);

  OptionHandlerInt(ui::Control* c, Apply a, Validate v) :
    m_control(c), m_apply(a), m_validate(v) {}

  virtual void process(const std::string& key, const std::string& arg) {
    int a;
    
    if (std::sscanf(arg.c_str(), "%i", &a) != 1 ||
	!m_validate(a))
      throw std::runtime_error("Invalid argument for \"" + key + "\": \"" + arg + "\"");
    
    m_apply(m_control, a);
  }

private:
  ui::Control* m_control;
  Apply        m_apply;
  Validate     m_validate;
};

class OptionHandlerString : public OptionHandlerBase {
public:
  typedef bool (*Validate)(const std::string&);
  typedef void (*Apply)(ui::Control*, const std::string&);

  OptionHandlerString(ui::Control* c, Apply a, Validate v) :
    m_control(c), m_apply(a), m_validate(v) {}

  virtual void process(const std::string& key, const std::string& arg) {
    if (!m_validate(arg))
      throw std::runtime_error("Invalid argument for \"" + key + "\": \"" + arg + "\"");
    
    m_apply(m_control, arg);
  }

private:
  ui::Control* m_control;
  Apply        m_apply;
  Validate     m_validate;
};

#endif
