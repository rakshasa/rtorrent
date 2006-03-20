// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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

// Parts of this seems ugly in an attempt to avoid copying
// data. Propably need to rewrite torrent::Object.

#ifndef RTORRENT_UTILS_VARIABLE_GENERIC_H
#define RTORRENT_UTILS_VARIABLE_GENERIC_H

#include <cstdio>
#include <string>
#include <limits>
#include <inttypes.h>
#include <rak/functional_fun.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>

#include "variable.h"

namespace utils {

class VariableAny : public Variable {
public:
  VariableAny(const torrent::Object& v = torrent::Object()) :
    m_variable(v) {}
  virtual ~VariableAny();

  virtual const torrent::Object&  get();
  virtual void                    set(const torrent::Object& arg);

private:
  torrent::Object    m_variable;
};

class VariableValue : public Variable {
public:
  VariableValue(int64_t v) : m_variable(v) {}
  virtual ~VariableValue();

  virtual const torrent::Object& get();
  virtual void                    set(const torrent::Object& arg);

private:
  torrent::Object    m_variable;
};

class VariableBool : public Variable {
public:
  VariableBool(bool state) : m_variable(state ? (int64_t)1 : (int64_t)0) {}
  VariableBool(const torrent::Object& v = torrent::Object((int64_t)0)) { set(v); }
  virtual ~VariableBool();

  virtual const torrent::Object& get();
  virtual void        set(const torrent::Object& arg);

private:
  torrent::Object    m_variable;
};

class VariableObject : public Variable {
public:
  typedef torrent::Object::type_type Type;

  VariableObject(torrent::Object* b,
		  const std::string& root,
		  const std::string& key,
		  Type t = torrent::Object::TYPE_NONE) :
    m_bencode(b), m_root(root), m_key(key), m_type(t) {}
  virtual ~VariableObject();

  virtual const torrent::Object& get();
  virtual void        set(const torrent::Object& arg);

private:
  torrent::Object*   m_bencode;
  std::string         m_root;
  std::string         m_key;
  Type                m_type;
};

template <typename Get = std::string, typename Set = const std::string&>
class VariableSlotString : public Variable {
public:
  typedef rak::function0<Get>       SlotGet;
  typedef rak::function1<void, Set> SlotSet;

  VariableSlotString(typename SlotGet::base_type* slotGet, typename SlotSet::base_type* slotSet) {
    m_slotGet.set(slotGet);
    m_slotSet.set(slotSet);
  }

  virtual ~VariableSlotString() {}

  virtual const torrent::Object& get() {
    m_cache = m_slotGet();

    if (!m_cache.is_string())
      throw torrent::internal_error("VariableSlotString::get() got wrong type.");

    return m_cache;
  }

  virtual void set(const torrent::Object& arg) {
    switch (arg.type()) {
    case torrent::Object::TYPE_STRING:
      m_slotSet(arg.as_string());
      break;
    case torrent::Object::TYPE_NONE:
      m_slotSet("");
      break;
    default:
      throw torrent::internal_error("VariableSlotString::set(...) got wrong type.");
    }    
  }

private:
  SlotGet             m_slotGet;
  SlotSet             m_slotSet;

  // Store the cache here to avoid unnessesary copying and such. This
  // should not result in any unresonable memory usage since few
  // strings will be very large.
  torrent::Object    m_cache;
};

template <typename Get, typename Set>
class VariableSlotValue : public Variable {
public:
  typedef rak::function0<Get>         SlotGet;
  typedef rak::function1<void, Set>   SlotSet;
  typedef std::pair<int64_t, int64_t> Range;

  VariableSlotValue(typename SlotGet::base_type* slotGet,
		    typename SlotSet::base_type* slotSet,
		    const char* pattern,
		    Range range = Range(std::numeric_limits<int64_t>::min(),
					std::numeric_limits<int64_t>::max())) {
    m_slotGet.set(slotGet);
    m_slotSet.set(slotSet);
    m_pattern = pattern;
    m_range = range;
  }

  virtual ~VariableSlotValue() {}

  virtual const torrent::Object& get() {
    m_cache = m_slotGet();

    // Need this?
    if (!m_cache.is_value())
      throw torrent::internal_error("VariableSlotValue::get() got wrong type.");

    return m_cache;
  }

  virtual void set(const torrent::Object& arg) {
    if (arg.is_string()) {
      Set v;

      if (std::sscanf(arg.as_string().c_str(), m_pattern, &v) != 1)
	throw torrent::input_error("Not a value.");
      
      m_slotSet(v);

    } else if (arg.is_value()) {
      m_slotSet(arg.as_value());

    } else {
      throw torrent::input_error("Not a value");
    }
  }

private:
  SlotGet             m_slotGet;
  SlotSet             m_slotSet;

  const char*         m_pattern;
  Range               m_range;

  // Store the cache here to avoid unnessesary copying and such. This
  // should not result in any unresonable memory usage since few
  // strings will be very large.
  torrent::Object    m_cache;
};

}

#endif
