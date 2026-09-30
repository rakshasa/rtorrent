#ifndef RTORRENT_RPC_PARSE_H
#define RTORRENT_RPC_PARSE_H

#include <string>
#include <cctype>
#include <torrent/object.h>

namespace rpc {

// parse_* functions do the bare minimum necessary to parse what was
// asked for. If a whitespace is found, it will be treated as empty
// input rather than skipped.
//
// parse_whole_* functions allow for whitespaces and throw an
// exception if there is any garbage at the end of the input.

inline bool parse_is_quote(const char c)     { return c == '"'; }
inline bool parse_is_escape(const char c)    { return c == '\\'; }
inline bool parse_is_seperator(const char c) { return c == ','; }
inline bool parse_is_space(const char c)     { return c == ' ' || c == '\t'; }

// The block delim is used in {} blocks to contain code. Since it
// doesn't check for isspace, it will include useless characters but
// that is the price for sane syntax.

inline bool parse_is_delim_default(const char c) { return parse_is_seperator(c) || std::isspace(c); }
inline bool parse_is_delim_list(const char c)    { return parse_is_seperator(c) || c == '}' || std::isspace(c); }
inline bool parse_is_delim_command(const char c) { return parse_is_seperator(c) || c == ';' || std::isspace(c); }
// inline bool parse_is_delim_block(const char c)   { return c == ';' || c == '}'; }
inline bool parse_is_delim_block(const char c)   { return parse_is_seperator(c) || c == '}'; }
inline bool parse_is_delim_func(const char c)    { return parse_is_seperator(c) || c == ')'; }

const char* parse_skip_wspace(const char* first);
const char* parse_skip_wspace(const char* first, const char* last);

const char* parse_string(const char* first, const char* last, std::string* dest, bool (*delim)(const char) = &parse_is_delim_default);
void        parse_whole_string(const char* first, const char* last, std::string* dest);

const char* parse_value(const char* src, int64_t* value, int base = 0, int unit = 1);
const char* parse_value_nothrow(const char* src, int64_t* value, int base = 0, int unit = 1);
const char* parse_value_nothrow(const char* first, const char* last, int64_t* value, int base = 0, int unit = 0);

void        parse_whole_value(const char* src, int64_t* value, int base = 0, int unit = 1);
bool        parse_whole_value_nothrow(const char* src, int64_t* value, int base = 0, int unit = 1);

const char* parse_object    (const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char) = &parse_is_delim_default, uint32_t depth = 0);
const char* parse_list      (const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char) = &parse_is_delim_default, uint32_t depth = 0);
const char* parse_whole_list(const char* first, const char* last, torrent::Object* dest, bool (*delim)(const char) = &parse_is_delim_default, uint32_t depth = 0);

std::string convert_to_string(const torrent::Object& src);

std::string convert_list_to_string(const torrent::Object& src);
std::string convert_list_to_string(torrent::Object::list_const_iterator first, torrent::Object::list_const_iterator last);
std::string convert_list_to_command(torrent::Object::list_const_iterator first, torrent::Object::list_const_iterator last);

int64_t     convert_to_value(const torrent::Object& src, int base = 0, int unit = 1);
bool        convert_to_value_nothrow(const torrent::Object& src, int64_t* value, int base = 0, int unit = 1);

inline const torrent::Object&
convert_to_single_argument(const torrent::Object& args) {
  if (args.type() == torrent::Object::TYPE_LIST && args.as_list().size() == 1)
    return args.as_list().front();
  else
    return args;
}

static const int print_expand_tilde = 0x1;

char*       print_object(char* first, char* last, const torrent::Object* src, int flags);
void        print_object_std(std::string* dest, const torrent::Object* src, int flags);

}

#endif
