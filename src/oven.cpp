#include <AclAPI.h>
#include <Windows.h>

#include <cassert>
#include <iostream>
#include <list>
#include <string>

#include "base/command_line.h"
#include "execution_result.h"
#include "system/child_process.h"
#include "system/desktop.h"
#include "system/error.h"
#include "system/job.h"

namespace arguments {
const wchar_t kDesktopName[] = L"desktop-name";
const wchar_t kDesktopHeapSize[] = L"desktop-heap-size";
const wchar_t kChildPath[] = L"child-path";
const wchar_t kChildTimeout[] = L"child-timeout";
const wchar_t kRequiresActivation[] = L"requires-activation";
const wchar_t kResultPath[] = L"result-path";

// Additional limits
const wchar_t kLimitCPUTime[] = L"limit-cpu-time";
const wchar_t kLimitOverallMemory[] = L"limit-overall-memory";
const wchar_t kLimitPerProcessMemory[] = L"limit-per-process-memory";
}  // arguments namespace

namespace {
const wchar_t kDefaultDesktopName[] = L"OvenDesktop";
}  // anonymous namespace

class JobObserver : public oven::system::Job::Observer {
 public:
  void OnNewProcess(const unsigned long process_id) override {
    std::wcout << L"New process was created inside of job: " << process_id << "\n";
  }

  void OnExitProcess(const unsigned long process_id) override {
    std::wcout << L"Process with id " << process_id << L" has exited\n";
  }

  void OnActiveProcessZero() override {
    std::wcout << L"Number of child processes equal zero!\n";
  }
};

void AddLimitingArguments(oven::base::CommandLine& command_line) {
  command_line.AddOptionalArgument(
      arguments::kLimitCPUTime,
      L"User-mode execution time limit, in milliseconds",
      oven::base::CommandLine::ArgumentType::kInt);

  command_line.AddOptionalArgument(
      arguments::kLimitOverallMemory,
      L"Specifies the limit for the virtual memory that can be "
      L"committed for all child processes, in bytes",
      oven::base::CommandLine::ArgumentType::kInt);

  command_line.AddOptionalArgument(
      arguments::kLimitPerProcessMemory,
      L"Specifies the limit for the virtual memory that can be "
      L"committed by any child process",
      oven::base::CommandLine::ArgumentType::kInt);
}

void ParseArguments(oven::base::CommandLine& command_line) {
  command_line.AddArgument(arguments::kChildPath, L"Path to child executable",
                           oven::base::CommandLine::ArgumentType::kString);

  command_line.AddArgument(arguments::kChildTimeout,
                           L"Timeout in milliseconds for child process",
                           oven::base::CommandLine::ArgumentType::kInt);

  command_line.AddOptionalArgument(arguments::kDesktopName,
                                   L"Name of virtual desktop to use",
                                   oven::base::CommandLine::ArgumentType::kString);

  command_line.AddOptionalArgument(
      arguments::kRequiresActivation,
      L"Pass if child process requires activated interactive desktop",
      oven::base::CommandLine::ArgumentType::kBool);

  command_line.AddOptionalArgument(
      arguments::kResultPath,
      L"Path to file to serialize result json to",
      oven::base::CommandLine::ArgumentType::kString);

  command_line.AddOptionalArgument(
      arguments::kDesktopHeapSize,
      L"Heap size of created desktop",
      oven::base::CommandLine::ArgumentType::kInt);

  AddLimitingArguments(command_line);

  const std::wstring command_line_parse_error = command_line.Parse();
  if (command_line.ShouldShowUsage()) {
    command_line.ShowUsage(std::wcout);
    exit(0);
  }
  if (!command_line_parse_error.empty()) {
    std::wclog << L"Unable to parse command line arguments: "
               << command_line_parse_error << L'\n';
    command_line.ShowUsage(std::wclog);
    exit(1);
  }
}

int wmain(int argc, wchar_t* argv[]) {
  oven::base::CommandLine command_line(argc, argv);
  ParseArguments(command_line);

  oven::ExecutionResult execution_result(
      command_line.GetValue(arguments::kResultPath, std::wstring()));

  const std::wstring desktop_name = command_line.GetValue(
      arguments::kDesktopName, std::wstring(kDefaultDesktopName));
  const int desktop_heap_size = static_cast<int>(
      command_line.GetValue(arguments::kDesktopHeapSize, std::int64_t(2048)));
  oven::system::Desktop virtual_desktop(desktop_name, desktop_heap_size);
  if (!virtual_desktop.IsValid()) {
    execution_result.SetInternalError(L"Unable to create virtual desktop");
    return execution_result.Exit(1);
#if defined(ENABLE_ASSERTIONS)
  } else {
    assert(desktop_name == virtual_desktop.GetName() && "Unable to retrieve desktop name");
#endif
  }

  JobObserver test_observer;
  oven::system::Job limited_job;

  limited_job.AddObserver(&test_observer);

  oven::system::Job::BasicLimits basic_limits;
  basic_limits.cpu_time_limit = std::chrono::milliseconds(command_line.GetValue(
      arguments::kLimitCPUTime,
      static_cast<std::int64_t>(std::chrono::milliseconds::max().count())));

  basic_limits.overall_memory_limit = command_line.GetValue(
      arguments::kLimitOverallMemory, std::numeric_limits<std::int64_t>::max());
  basic_limits.per_process_memory_limit = command_line.GetValue(
      arguments::kLimitPerProcessMemory, std::numeric_limits<std::int64_t>::max());

  if (!limited_job.SetBasicLimits(basic_limits)) {
    execution_result.SetInternalError(L"Unable to set limits on job");
    return execution_result.Exit(1);
  }

  std::optional<oven::system::ScopedDesktopActivation> scoped_activation;
  if (*command_line.GetValue<bool>(arguments::kRequiresActivation)) {
    scoped_activation.emplace(virtual_desktop);
  }

  oven::system::ChildProcess child(
      *command_line.GetValue<std::wstring>(arguments::kChildPath),
      false /* detached */);
  child.SetArguments(command_line.GetUnparsed());
  const auto pid = child.Run(limited_job, desktop_name);
  if (!pid) {
    execution_result.SetInternalError(L"Unable to run child process");
    return execution_result.Exit(1);
  }

  const auto child_timeout = *command_line.GetValue<std::int64_t>(arguments::kChildTimeout);
  auto exit_code = child.Wait(std::chrono::milliseconds(child_timeout));
  if (!exit_code) {
    if (child.IsAlive()) {
      execution_result.ChildTimedOut();
    }
    exit_code = child.Terminate();
  }

  if (exit_code) {
    execution_result.ChildExitCode(*exit_code);
  }

  const oven::system::ChildProcess::Outputs& outputs = child.GetOutputs();
  execution_result.SetChildStderr(outputs.stderror);
  execution_result.SetChildStdout(outputs.stdoutput);
  return execution_result.Exit(0);
}
