#ifndef _OVEN_SYSTEM_CHILD_PROCESS_H_
#define _OVEN_SYSTEM_CHILD_PROCESS_H_

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "system/scoped_handle.h"

namespace oven {
namespace system {

class Job;

class ChildProcess {
 public:
  struct Outputs {
    std::string stdoutput;
    std::string stderror;
  };
  explicit ChildProcess(const std::wstring_view executable_path);
  ChildProcess(const std::wstring_view executable_path, const bool detached);
  ~ChildProcess();

  ChildProcess(const ChildProcess&) = delete;
  ChildProcess(ChildProcess&& other) noexcept = default;

  // Do not allow to reassign as it looks like a bad idea
  // to terminate child process hosted by sink.
  ChildProcess& operator=(const ChildProcess&) = delete;
  ChildProcess& operator=(ChildProcess&&) = delete;

  // Returns process id on success, nothing on failure.
  std::optional<unsigned long> Run(Job& job);

  // If desktop from another window station (one that is not currently assigned
  // to current process) is passed, make sure it's name is prefixed in the
  // following way:  <window_station_name>\\<desktop_name>>
  std::optional<unsigned long> Run(Job& job, const std::wstring_view desktop_name);

  template <typename ArgsContainer>
  void SetArguments(const ArgsContainer& arguments) noexcept {
    arguments_.reserve(arguments.size() + 1);  // Extra 1 for executable image path.
    for (const auto& arg : arguments) {
      arguments_.emplace_back(arg);
    }
  }

  std::wstring RenderCommandLine() noexcept;

  // Returns true if child process has started and it's exit code wasn't yet
  // collected via |Wait|.
  bool IsAlive() const noexcept { return child_process_handle_.get(); }

  // Wait until child process exits. Returns exit code on success.
  std::optional<int> Wait() { return Wait(std::chrono::milliseconds::max()); }

  // Wait for specified number of milliseconds.
  std::optional<int> Wait(const std::chrono::milliseconds timeout);

  // Marks all threads of child process as finishing and waits for child process
  // to complete it's execution, returns exit code if available.
  std::optional<int> Terminate();

  const Outputs& GetOutputs() {
    RetreiveOutputStreams();
    return *output_streams_;
  }

 private:
  std::optional<unsigned long> RunImpl(Job& job, STARTUPINFOW&& startup_info);
  void RetreiveOutputStreams();

  std::wstring executable_path_;
  bool detached_;

  ScopedHandle child_process_handle_;
  std::future<Outputs> output_streams_future_;
  std::optional<Outputs> output_streams_;
  std::vector<std::wstring> arguments_;

  std::atomic_bool terminated_ = false;
};

}  // namespace system
}  // namespace oven

#endif  // _OVEN_SYSTEM_CHILD_PROCESS_H_
