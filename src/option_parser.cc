#include "config.h"

#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <sigc++/hide.h>

#include "option_parser.h"

void
OptionParser::insert_flag(char c, SlotFlag s) {
  m_container[c].m_slot = sigc::hide(s);
  m_container[c].m_useOption = false;
}

void
OptionParser::insert_option(char c, SlotOption s) {
  m_container[c].m_slot = s;
  m_container[c].m_useOption = true;
}

int
OptionParser::process(int argc, char** argv) {
  int c;
  std::string optString = create_optstring();

  opterr = 0;

  while ((c = getopt(argc, argv, optString.c_str())) != -1) {
    if (c == '?') {
      std::stringstream s;
      s << "Invalid use of option flag -" << (char)optopt;
      
      throw std::runtime_error(s.str());

    } else {
      call_flag(c, optarg ? optarg : "");
    }
  }

  return optind;
}

void
OptionParser::call_int_pair(const std::string& str, SlotIntPair slot) {
  int a, b;

  if (sscanf(str.c_str(), "%u-%u", &a, &b) != 2)
    throw std::runtime_error("Invalid argument \"" + str + "\" should be <a-b>");

  slot(a, b);
}

std::string
OptionParser::create_optstring() {
  std::string s;

  for (Container::iterator itr = m_container.begin(); itr != m_container.end(); ++itr) {
    s += itr->first;

    if (itr->second.m_useOption)
      s += ':';
  }

  return s;
}

void
OptionParser::call_flag(char c, const std::string& arg) {
  Container::iterator itr = m_container.find(c);

  if (itr == m_container.end())
    throw std::logic_error("OptionParser::call_flag(...) could not find the flag");

  itr->second.m_slot(arg);
}
