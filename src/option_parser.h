#ifndef RTORRENT_OPTION_PARSER_H
#define RTORRENT_OPTION_PARSER_H

#include <functional>
#include <map>
#include <string>

// Throws std::runtime_error upon receiving bad input.

class OptionParser {
public:
  typedef std::function<void (const std::string&)>                     slot_string;
  typedef std::function<void (const std::string&, const std::string&)> slot_string_pair;
  typedef std::function<void (int, int)>                               slot_int_pair;

  void                insert_flag(char c, slot_string s);
  void                insert_option(char c, slot_string s);
  void                insert_option_list(char c, slot_string_pair s);
  void                insert_int_pair(char c, slot_int_pair s);

  // Returns the index of the first non-option argument.
  int                 process(int argc, char** argv);

  static bool         has_flag(char flag, int argc, char** argv);

private:
  std::string         create_optstring();

  void                call(char c, const std::string& arg);
  static void         call_option_list(slot_string_pair slot, const std::string& arg);
  static void         call_int_pair(slot_int_pair slot, const std::string& arg);

  // Use pair instead?
  struct Node {
    slot_string         m_slot;
    bool                m_useOption;
  };

  typedef std::map<char, Node> Container;

  Container           m_container;
};

#endif
