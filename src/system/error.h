#ifndef _OVEN_SYSTEM_ERROR_H_
#define _OVEN_SYSTEM_ERROR_H_

#include <string>

namespace oven {
namespace system {

std::wstring GetErrorMessage(const int message_code);
std::wstring GetErrorMessage();

void OutputError(const wchar_t* message);

}  // namespace system
}  // namespace oven

#endif  // _OVEN_SYSTEM_ERROR_H_
