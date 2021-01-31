#ifndef _OVEN_EXECUTION_RESULT_H_
#define _OVEN_EXECUTION_RESULT_H_

#include <filesystem>
#include <optional>

namespace oven {

class ExecutionResult {
 public:
  ExecutionResult(const std::filesystem::path& result_file);

  [[nodiscard]] int Exit(const int exit_code);

  void SetInternalError(const std::wstring_view message);
  
  void ChildTimedOut() {
    child_timed_out_ = true;
  }

  void ChildExitCode(const int exit_code) {
    child_exit_code_ = exit_code;
  }

  void SetChildStdout(const std::string& contents) {
    child_stdout_ = contents;
  }
  void SetChildStderr(const std::string& contents) {
    child_stderr_ = contents;
  }

 private:
  std::wstring ExitCodeAsJson() const;

  const std::filesystem::path result_file_;
  std::wstring internal_error_;
  bool child_timed_out_ = false;
  std::optional<int> child_exit_code_;
  std::string child_stdout_;
  std::string child_stderr_;
};

}  // namespace oven

#endif  // _OVEN_EXECUTION_RESULT_H_