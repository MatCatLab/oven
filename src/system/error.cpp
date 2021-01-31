#include "system/error.h"

#include <Windows.h>

#include <iostream>
#include <string>

namespace oven {
namespace system {
std::wstring GetErrorMessage(const int message_code) {
  std::wstring error_message;
  if (message_code != 0) {
    LPWSTR message_buffer = nullptr;
    const size_t size = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, static_cast<DWORD>(message_code),
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPWSTR)&message_buffer, 0, NULL);
    error_message.assign(message_buffer, size);
    ::LocalFree(message_buffer);
  }
  return error_message;
}

std::wstring GetErrorMessage() {
  return GetErrorMessage(static_cast<int>(::GetLastError()));
}

void OutputError(const wchar_t* message) {
	std::wclog << message << L": " << GetErrorMessage() << L'\n';
}
}  // namespace system
}  // namespace oven
