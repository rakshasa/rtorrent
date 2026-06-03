#include "base64.h"

#include <string>
#include <openssl/evp.h>

namespace utils {

std::string
remove_newlines(const std::string& str) {
  std::string result;
  for (auto& itr : str) {
    if (itr != '\n' && itr != '\r')
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

// TODO: Move to torrent::utils.

std::optional<std::vector<uint8_t>>
base64_to_vector_unsafe(const std::string& src) {
  if (src.empty())
    return std::vector<uint8_t>{};

  if (src.length() % 4)
    return std::nullopt;

  std::vector<uint8_t> bytes((src.length() * 3) / 4);

  int decoded_len = EVP_DecodeBlock(bytes.data(), reinterpret_cast<const uint8_t*>(src.data()), src.length());

  if (decoded_len <= 0)
    return std::nullopt;

  if (src.back() == '=')
    decoded_len--;

  if (src.length() > 1 && src[src.length() - 2] == '=')
    decoded_len--;

  // If the input contains extra padding characters, this could cause negative decoded_len.
  if (decoded_len < 0)
    return std::nullopt;

  bytes.resize(decoded_len);

  return bytes;
}

std::string
openssl_base64_encode(const std::string& src) {
  if (src.empty()) return {};

  std::string result((4 * ((src.size() + 2) / 3)), '\0');

  int actual_length = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(result.data()),
                                      reinterpret_cast<const unsigned char*>(src.data()),
                                      src.size());

  result.resize(actual_length);
  return result;
}

} // namespace utils
