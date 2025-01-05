#include "base64.h"

#include <string>

namespace utils {

std::string
remove_newlines(const std::string& str) {
  std::string result;
  for (auto& itr : str) {
    if (itr != '\n' || itr != '\n')
      result.push_back(itr);
  }
  return result;
}

// Modified from the public domain code in
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C++_2
constexpr static char base64_pad_character = '=';

std::string
decode_base64(const std::string& input) {
  if (input.length() % 4) // Sanity check
    throw torrent::input_error("Invalid base64.");
  size_t padding = 0;
  if (input.length()) {
    if (input[input.length() - 1] == base64_pad_character)
      padding++;
    if (input[input.length() - 2] == base64_pad_character)
      padding++;
  }
  // Setup a vector to hold the result
  std::string decodedBytes;
  decodedBytes.reserve(((input.length() / 4) * 3) - padding);
  long                        temp   = 0; // Holds decoded quanta
  std::string::const_iterator cursor = input.begin();
  while (cursor < input.end()) {
    for (size_t quantumPosition = 0; quantumPosition < 4; quantumPosition++) {
      temp <<= 6;
      if (*cursor >= 0x41 && *cursor <= 0x5A) // This area will need tweaking if
        temp |= *cursor - 0x41;               // you are using an alternate alphabet
      else if (*cursor >= 0x61 && *cursor <= 0x7A)
        temp |= *cursor - 0x47;
      else if (*cursor >= 0x30 && *cursor <= 0x39)
        temp |= *cursor + 0x04;
      else if (*cursor == 0x2B)
        temp |= 0x3E; // change to 0x2D for URL alphabet
      else if (*cursor == 0x2F)
        temp |= 0x3F;                           // change to 0x5F for URL alphabet
      else if (*cursor == base64_pad_character) // pad
      {
        switch (input.end() - cursor) {
        case 1: // One pad character
          decodedBytes.push_back((temp >> 16) & 0x000000FF);
          decodedBytes.push_back((temp >> 8) & 0x000000FF);
          return decodedBytes;
        case 2: // Two pad characters
          decodedBytes.push_back((temp >> 10) & 0x000000FF);
          return decodedBytes;
        default:
          throw torrent::input_error("Invalid padding in base64.");
        }
      } else
        throw torrent::input_error("Invalid character in base64.");
      cursor++;
    }
    decodedBytes.push_back((temp >> 16) & 0x000000FF);
    decodedBytes.push_back((temp >> 8) & 0x000000FF);
    decodedBytes.push_back((temp) & 0x000000FF);
  }
  return decodedBytes;
}
} // namespace utils
