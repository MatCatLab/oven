#include "execution_result.h"

#include <fstream>
#include <string>

#include "base/base64.h"
#include "system/error.h"

namespace oven {

ExecutionResult::ExecutionResult(
    const std::filesystem::path& result_file)
    : result_file_(result_file) {
}

int ExecutionResult::Exit(const int exit_code) {
  std::wofstream file(result_file_);
  file << L"{\n"
        << LR"RAW(  "internal_error": ")RAW" << internal_error_ << L"\",\n"
        << LR"RAW(  "child_timed_out": )RAW"
        << (child_timed_out_ ? L"true,\n" : L"false,\n")
        << LR"RAW(  "child_exit_code": )RAW" << ExitCodeAsJson() << L",\n"
        << LR"RAW(  "child_stdout": ")RAW" << base::Base64Encode(child_stdout_) << L"\",\n"
        << LR"RAW(  "child_stderr": ")RAW" << base::Base64Encode(child_stderr_) << L"\",\n"
        << LR"RAW(  "exit_code": )RAW" << std::to_wstring(exit_code)
        << L"\n}";
  return exit_code;
}

void ExecutionResult::SetInternalError(
    const std::wstring_view message) {
  internal_error_ = message;
  internal_error_.append(L": ");
  internal_error_ += system::GetErrorMessage();
}

std::wstring ExecutionResult::ExitCodeAsJson() const {
  if (child_exit_code_)
    return std::to_wstring(*child_exit_code_);
  return L"null";
}

}  // namespace oven
