#include "config.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "rpc/parse.h"
#include "rpc/rpc_manager.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

namespace {

const std::string whitespace_characters = " \t\n\r\f\v";

// The byte offset of every utf-8 character in 'text', terminated by the offset
// past the last character. Bytes that are not valid utf-8 lead bytes are
// treated as single characters.
std::vector<size_t>
utf8_offsets(const std::string& text) {
  std::vector<size_t> offsets;

  for (size_t i = 0; i < text.size(); i++)
    if (i == 0 || (static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
      offsets.push_back(i);

  offsets.push_back(text.size());
  return offsets;
}

int64_t
utf8_length(const std::string& text) {
  int64_t result = 0;

  for (size_t i = 0; i < text.size(); i++)
    if (i == 0 || (static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
      result++;

  return result;
}

std::string
utf8_substr(const std::string& text, const std::vector<size_t>& offsets, size_t first, size_t last) {
  return text.substr(offsets[first], offsets[last] - offsets[first]);
}

std::set<std::string>
utf8_character_set(const std::string& text) {
  auto offsets = utf8_offsets(text);
  std::set<std::string> result;

  for (size_t i = 0; i + 1 < offsets.size(); i++)
    result.insert(utf8_substr(text, offsets, i, i + 1));

  return result;
}

std::string
ascii_lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
  return text;
}

// A 'max_count' of zero means the command accepts any number of arguments.
void
verify_argument_count(const char* name, const torrent::Object::list_type& args, size_t min_count, size_t max_count) {
  if (args.size() < min_count || (max_count != 0 && args.size() > max_count))
    throw torrent::input_error(std::string(name) + ": invalid number of arguments.");
}

const torrent::Object&
argument_at(const torrent::Object::list_type& args, size_t index) {
  return *std::next(args.begin(), index);
}

void
flatten_argument(const torrent::Object& arg, std::vector<std::string>* dest) {
  if (!arg.is_list()) {
    dest->push_back(rpc::convert_to_string(arg));
    return;
  }

  for (const auto& child : arg.as_list())
    flatten_argument(child, dest);
}

// The {old, new} pairs shared by 'string.map' and 'string.replace'.
std::pair<std::string, std::string>
argument_to_pair(const char* name, const torrent::Object& arg) {
  if (!arg.is_list() || arg.as_list().size() != 2)
    throw torrent::input_error(std::string(name) + ": arguments after the text must be {old, new} pairs.");

  return {rpc::convert_to_string(arg.as_list().front()), rpc::convert_to_string(arg.as_list().back())};
}

// Compares the first argument against every remaining argument, returning true
// as soon as one of them matches.
torrent::Object
apply_string_predicate(const char* name, const torrent::Object::list_type& args, bool (*predicate)(const std::string&, const std::string&)) {
  verify_argument_count(name, args, 2, 0);

  auto text = rpc::convert_to_string(args.front());

  for (auto itr = std::next(args.begin()); itr != args.end(); itr++)
    if (predicate(text, rpc::convert_to_string(*itr)))
      return int64_t(1);

  return int64_t(0);
}

bool
text_equals(const std::string& text, const std::string& other) {
  return text == other;
}

bool
text_starts_with(const std::string& text, const std::string& prefix) {
  return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool
text_ends_with(const std::string& text, const std::string& tail) {
  return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

bool
text_contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

bool
text_contains_i(const std::string& text, const std::string& needle) {
  return ascii_lowercase(text).find(ascii_lowercase(needle)) != std::string::npos;
}

torrent::Object
apply_string_length(const torrent::Object::list_type& args) {
  verify_argument_count("string.length", args, 1, 1);

  return utf8_length(rpc::convert_to_string(args.front()));
}

torrent::Object
apply_string_substr(const torrent::Object::list_type& args) {
  verify_argument_count("string.substr", args, 1, 4);

  auto text        = rpc::convert_to_string(args.front());
  auto offsets     = utf8_offsets(text);
  auto text_length = static_cast<int64_t>(offsets.size() - 1);

  auto position = args.size() > 1 ? rpc::convert_to_value(argument_at(args, 1)) : 0;
  auto fallback = args.size() > 3 ? rpc::convert_to_string(argument_at(args, 3)) : std::string();

  // Negative positions are relative to the end of the string.
  if (position < 0)
    position += text_length;

  if (position < 0 || position > text_length)
    return fallback;

  auto last = text_length;

  if (args.size() > 2) {
    auto count = rpc::convert_to_value(argument_at(args, 2));

    if (count < 0)
      throw torrent::input_error("string.substr: the character count cannot be negative.");

    last = std::min(text_length, position + std::min(count, text_length));
  }

  return utf8_substr(text, offsets, position, last);
}

torrent::Object
apply_string_split(const torrent::Object::list_type& args) {
  verify_argument_count("string.split", args, 2, 2);

  auto text   = rpc::convert_to_string(args.front());
  auto delim  = rpc::convert_to_string(args.back());
  auto result = torrent::Object::create_list();

  // An empty delimiter splits the text into its utf-8 characters.
  if (delim.empty()) {
    auto offsets = utf8_offsets(text);

    for (size_t i = 0; i + 1 < offsets.size(); i++)
      result.as_list().push_back(utf8_substr(text, offsets, i, i + 1));

    return result;
  }

  size_t first = 0;

  while (true) {
    auto pos = text.find(delim, first);

    if (pos == std::string::npos) {
      result.as_list().push_back(text.substr(first));
      return result;
    }

    result.as_list().push_back(text.substr(first, pos - first));
    first = pos + delim.size();
  }
}

torrent::Object
apply_string_join(const torrent::Object::list_type& args) {
  verify_argument_count("string.join", args, 1, 0);

  auto delim = rpc::convert_to_string(args.front());

  std::vector<std::string> parts;

  for (auto itr = std::next(args.begin()); itr != args.end(); itr++)
    flatten_argument(*itr, &parts);

  std::string result;

  for (auto itr = parts.begin(); itr != parts.end(); itr++) {
    if (itr != parts.begin())
      result += delim;

    result += *itr;
  }

  return result;
}

torrent::Object
apply_string_pad(const char* name, const torrent::Object::list_type& args, bool pad_start) {
  verify_argument_count(name, args, 2, 3);

  auto text     = rpc::convert_to_string(args.front());
  auto pad_size = rpc::convert_to_value(argument_at(args, 1));
  auto padding  = args.size() > 2 ? rpc::convert_to_string(argument_at(args, 2)) : std::string(" ");

  auto text_length = utf8_length(text);

  if (pad_size <= text_length || padding.empty())
    return text;

  auto padding_offsets = utf8_offsets(padding);
  auto padding_length  = static_cast<int64_t>(padding_offsets.size() - 1);

  std::string result;

  for (int64_t i = 0; i < pad_size - text_length; i++) {
    auto index = static_cast<size_t>(i % padding_length);
    result += utf8_substr(padding, padding_offsets, index, index + 1);
  }

  return pad_start ? result + text : text + result;
}

torrent::Object
apply_string_strip(const char* name, const torrent::Object::list_type& args, bool strip_start, bool strip_end) {
  verify_argument_count(name, args, 1, 0);

  auto text = rpc::convert_to_string(args.front());

  std::string strippable;

  for (auto itr = std::next(args.begin()); itr != args.end(); itr++)
    strippable += rpc::convert_to_string(*itr);

  if (args.size() == 1)
    strippable = whitespace_characters;

  auto characters = utf8_character_set(strippable);
  auto offsets    = utf8_offsets(text);

  size_t first = 0;
  size_t last  = offsets.size() - 1;

  while (strip_start && first < last && characters.count(utf8_substr(text, offsets, first, first + 1)) != 0)
    first++;

  while (strip_end && last > first && characters.count(utf8_substr(text, offsets, last - 1, last)) != 0)
    last--;

  return utf8_substr(text, offsets, first, last);
}

torrent::Object
apply_string_map(const torrent::Object::list_type& args) {
  verify_argument_count("string.map", args, 2, 0);

  auto text = rpc::convert_to_string(args.front());

  for (auto itr = std::next(args.begin()); itr != args.end(); itr++) {
    auto pair = argument_to_pair("string.map", *itr);

    if (text == pair.first)
      return pair.second;
  }

  return text;
}

torrent::Object
apply_string_replace(const torrent::Object::list_type& args) {
  verify_argument_count("string.replace", args, 2, 0);

  auto text = rpc::convert_to_string(args.front());

  for (auto itr = std::next(args.begin()); itr != args.end(); itr++) {
    auto pair = argument_to_pair("string.replace", *itr);

    if (pair.first.empty())
      throw torrent::input_error("string.replace: the replaced text cannot be empty.");

    for (auto pos = text.find(pair.first); pos != std::string::npos; pos = text.find(pair.first, pos + pair.second.size()))
      text.replace(pos, pair.first.size(), pair.second);
  }

  return text;
}

}

void
initialize_command_string() {
  // clang-format off
  CMD2_ANY_LIST("string.length",        [](auto, const auto& args) { return apply_string_length(args); });
  CMD2_ANY_LIST("string.substr",     [](auto, const auto& args) { return apply_string_substr(args); });
  CMD2_ANY_LIST("string.split",      [](auto, const auto& args) { return apply_string_split(args); });
  CMD2_ANY_LIST("string.join",       [](auto, const auto& args) { return apply_string_join(args); });
  CMD2_ANY_LIST("string.map",        [](auto, const auto& args) { return apply_string_map(args); });
  CMD2_ANY_LIST("string.replace",    [](auto, const auto& args) { return apply_string_replace(args); });

  CMD2_ANY_LIST("string.equals",     [](auto, const auto& args) { return apply_string_predicate("string.equals", args, &text_equals); });
  CMD2_ANY_LIST("string.starts_with", [](auto, const auto& args) { return apply_string_predicate("string.starts_with", args, &text_starts_with); });
  CMD2_ANY_LIST("string.ends_with",   [](auto, const auto& args) { return apply_string_predicate("string.ends_with", args, &text_ends_with); });
  CMD2_ANY_LIST("string.contains",   [](auto, const auto& args) { return apply_string_predicate("string.contains", args, &text_contains); });
  CMD2_ANY_LIST("string.contains_i", [](auto, const auto& args) { return apply_string_predicate("string.contains_i", args, &text_contains_i); });

  CMD2_ANY_LIST("string.lpad",       [](auto, const auto& args) { return apply_string_pad("string.lpad", args, true); });
  CMD2_ANY_LIST("string.rpad",       [](auto, const auto& args) { return apply_string_pad("string.rpad", args, false); });

  CMD2_ANY_LIST("string.strip",      [](auto, const auto& args) { return apply_string_strip("string.strip", args, true, true); });
  CMD2_ANY_LIST("string.lstrip",     [](auto, const auto& args) { return apply_string_strip("string.lstrip", args, true, false); });
  CMD2_ANY_LIST("string.rstrip",     [](auto, const auto& args) { return apply_string_strip("string.rstrip", args, false, true); });
  // clang-format on

  for (const auto name : {"string.length", "string.substr", "string.split", "string.join", "string.map", "string.replace",
                          "string.equals", "string.starts_with", "string.ends_with", "string.contains", "string.contains_i",
                          "string.lpad", "string.rpad", "string.strip", "string.lstrip", "string.rstrip"})
    rpc::rpc.mark_safe(name);
}
