#ifndef RTORRENT_OPTION_PARSER_H
#define RTORRENT_OPTION_PARSER_H

#include <map>
#include <string>
#include <sigc++/slot.h>

// Throws std::runtime_error upon receiving bad input.

class OptionParser {
public:
  typedef sigc::slot0<void>                     SlotFlag;
  typedef sigc::slot1<void, const std::string&> SlotOption;
  typedef sigc::slot2<void, int, int>           SlotIntPair;

  struct Node {
    SlotOption m_slot;
    bool       m_useOption;
  };

  typedef std::map<char, Node>                  Container;

  void        insert_flag(char c, SlotFlag s);
  void        insert_option(char c, SlotOption s);

  // Returns the index of the first non-option argument.
  int         process(int argc, char** argv);

  static void call_int_pair(const std::string& str, SlotIntPair slot);

private:
  std::string create_optstring();

  void        call_flag(char c, const std::string& arg);

  Container   m_container;
};

#endif
