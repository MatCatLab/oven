#include "base/base64.h"

namespace oven {
namespace base {

std::wstring Base64Encode(const std::string& data) {
  static const wchar_t table[] = {
      L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I', L'J', L'K', L'L', L'M',
      L'N', L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V', L'W', L'X', L'Y', L'Z',
      L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h', L'i', L'j', L'k', L'l', L'm',
      L'n', L'o', L'p', L'q', L'r', L's', L't', L'u', L'v', L'w', L'x', L'y', L'z',
      L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'+', L'/'};

  const int output_size = 4 * ((data.size() + 2) / 3);
  std::wstring result(output_size, 0);
  int position;
  wchar_t* encode = result.data();

  for (position = 0; position < static_cast<int>(data.size()) - 2; position += 3) {
    *encode++ = table[(data[position] >> 2) & 0x3F];
    *encode++ = table[((data[position] & 0x3) << 4) |
                      ((int)(data[position + 1] & 0xF0) >> 4)];
    *encode++ = table[((data[position + 1] & 0xF) << 2) |
                      ((int)(data[position + 2] & 0xC0) >> 6)];
    *encode++ = table[data[position + 2] & 0x3F];
  }

  if (position < static_cast<int>(data.size())) {
    *encode++ = table[(data[position] >> 2) & 0x3F];
    if (position == (data.size() - 1)) {
      *encode++ = table[((data[position] & 0x3) << 4)];
      *encode++ = L'=';
    } else {
      *encode++ = table[((data[position] & 0x3) << 4) |
                        ((int)(data[position + 1] & 0xF0) >> 4)];
      *encode++ = table[((data[position + 1] & 0xF) << 2)];
    }
    *encode++ = L'=';
  }

  return result;
}

}  // namespace base
}  // namespace oven
