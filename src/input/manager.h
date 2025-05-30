#ifndef RTORRENT_INPUT_MANAGER_H
#define RTORRENT_INPUT_MANAGER_H

#include <vector>

namespace input {

class Bindings;
class TextInput;

class Manager : private std::vector<Bindings*> {
public:
  typedef std::vector<Bindings*> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::push_back;

  void erase(Bindings* b);

  void pressed(int key);

  void set_text_input(TextInput* input = nullptr) { m_textInput = input; }

private:
  TextInput* m_textInput{};
};

}

#endif
