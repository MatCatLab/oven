#include "system/child_process.h"

#include <cassert>

#include "system/error.h"
#include "system/job.h"
#include "system/pipe.h"

namespace oven {
namespace system {
namespace {
const int kKillExitCode = 1;

ChildProcess::Outputs ReadOutputs(Pipe stdoutput, Pipe stderror) {
  stdoutput.out().reset();
  stderror.out().reset();

  ChildProcess::Outputs outputs;

  const DWORD number_of_bytes_to_read = 4096;
  struct StreamData {
    StreamData(Pipe* pipe, std::string* output) : pipe(pipe), output(output) {}
    Pipe* pipe;
    char buffer[number_of_bytes_to_read];
    std::string* output;
  };
  StreamData output(&stdoutput, &outputs.stdoutput);
  StreamData error(&stderror, &outputs.stderror);

  IOCP iocp;
  if (!::CreateIoCompletionPort(output.pipe->in().get(), iocp.handle(),
          reinterpret_cast<ULONG_PTR>(&output), 0) ||
      !::CreateIoCompletionPort(error.pipe->in().get(), iocp.handle(),
          reinterpret_cast<ULONG_PTR>(&error), 0)) {
    OutputError(L"Unable to assosiate pipe with compiltion port");
    return outputs;
  }

  if (const auto result =
          ::ReadFile(output.pipe->in().get(), output.buffer,
                   number_of_bytes_to_read, NULL, &output.pipe->overlapped());
      result == 0 && ::GetLastError() != ERROR_IO_PENDING) {
    OutputError(L"Unable to read from pipe");
    return outputs;
  }

  if (const auto result =
          ::ReadFile(error.pipe->in().get(), error.buffer,
                   number_of_bytes_to_read, NULL, &error.pipe->overlapped());
      result == 0 && ::GetLastError() != ERROR_IO_PENDING) {
    OutputError(L"Unable to read from pipe");
    return outputs;
  }

  size_t completed_reading = 0;
  IOCP::WaitResult wait_result;
  do {
    ULONG_PTR completion_key;
    OVERLAPPED* overlapped;
    DWORD bytes_transferred;
    wait_result = iocp.Wait(std::chrono::milliseconds::max(), &completion_key,
                            &overlapped, &bytes_transferred);

    if (wait_result != IOCP::WaitResult::kSuccess)
      continue;
    if (bytes_transferred) {
      StreamData* stream_data = reinterpret_cast<StreamData*>(completion_key);
      stream_data->output->append(stream_data->buffer, bytes_transferred);

      // Failure of read means pipe is closed by child.
      ::ReadFile(stream_data->pipe->in().get(), stream_data->buffer,
                 number_of_bytes_to_read, NULL, &stream_data->pipe->overlapped());
    }
  } while(wait_result == IOCP::WaitResult::kSuccess);
  
  return outputs;
}
}  // anonymous namespace

ChildProcess::ChildProcess(const std::wstring_view executable_path)
    : ChildProcess(executable_path, false) {}

ChildProcess::ChildProcess(const std::wstring_view executable_path, const bool detached)
    : executable_path_(executable_path), detached_(detached), arguments_(1, executable_path_) {
}

ChildProcess::~ChildProcess() {
  if (IsAlive() && !detached_) {
    const auto exit_code = Wait();
    if (!exit_code.has_value()) {
      OutputError(L"Unable to wait for child process...");
    }
  }
  // Make sure process awaiting for child outputs has joined.
  RetreiveOutputStreams();
}

std::optional<unsigned long> ChildProcess::Run(Job& job) {
  STARTUPINFOW startup_info {
    sizeof(STARTUPINFOW),
  };
  return RunImpl(job, std::move(startup_info));
}

std::optional<unsigned long> ChildProcess::Run(
    Job& job, const std::wstring_view desktop_name) {
  STARTUPINFOW startup_info {
    sizeof(STARTUPINFOW),
  };
  startup_info.lpDesktop = const_cast<LPWSTR>(desktop_name.data());
  return RunImpl(job, std::move(startup_info));
}

std::wstring ChildProcess::RenderCommandLine() noexcept {
  std::wstring command_line;
  for (const std::wstring& arg : arguments_) {
    command_line += arg;
    command_line += L' ';
  }

  command_line.pop_back();
  return command_line;
}

std::optional<int> ChildProcess::Wait(const std::chrono::milliseconds timeout) {
#if defined(ENABLE_ASSERTIONS)
  assert(IsAlive() && "Cannot wait for null process");
#endif
  const auto wait_result = ::WaitForSingleObject(
      child_process_handle_.get(), static_cast<DWORD>(timeout.count()));
  if (wait_result == WAIT_OBJECT_0) {
    DWORD exit_code;
    if (!::GetExitCodeProcess(child_process_handle_.get(), &exit_code)) {
       OutputError(L"Unable to retrieve exit code of child process");
      return std::optional<int>();
    }
    child_process_handle_.reset();
    return static_cast<int>(exit_code);
  }
  return std::optional<int>();
}

std::optional<int> ChildProcess::Terminate() {
#if defined(ENABLE_ASSERTIONS)
  assert(IsAlive() && "Cannot terminate dead process");
#endif
  if (!::TerminateProcess(child_process_handle_.get(), kKillExitCode)) {
    // TODO(matthewtff): Process May have terminated already, try to receive it's exit code here.
    OutputError(L"Unable to teminate child process");
    return {};
  }
  return Wait();
}

std::optional<unsigned long> ChildProcess::RunImpl(
    Job& job, STARTUPINFOW&& startup_info) {

  startup_info.dwFlags = STARTF_USESTDHANDLES;
  Pipe stdout_stream;
  Pipe stderr_stream;
  startup_info.hStdOutput = stdout_stream.out().get();
  startup_info.hStdError = stderr_stream.out().get();
  
  std::wstring command_line = RenderCommandLine();
  PROCESS_INFORMATION process_info;
  if (!::CreateProcessW(const_cast<LPWSTR>(executable_path_.c_str()),
                        const_cast<LPWSTR>(command_line.c_str()),
                        NULL, NULL, TRUE, NULL, NULL, NULL,
                        &startup_info, &process_info)) {
    OutputError(L"Unable to start child process");
    std::promise<Outputs> empty_outputs;
    output_streams_future_ = empty_outputs.get_future();
    empty_outputs.set_value({});
		return std::optional<unsigned long>();
	} else {
    ::CloseHandle(process_info.hThread);
    child_process_handle_.reset(process_info.hProcess);
    if (!job.AssignProcess(child_process_handle_.get())) {
      OutputError(L"Unable to assign child process to job object");
    }
    /* TODO(matthewtff): Decide whether reuiqred
		if (!::WaitForInputIdle(child_process_handle_.get(), INFINITE)) {
      OutputError(L"Unable to wait for child process");
			std::optional<unsigned long>();
		}*/

    output_streams_future_ = std::async(ReadOutputs, std::move(stdout_stream),
                                 std::move(stderr_stream)); 
	}
  return process_info.dwProcessId;
}

void ChildProcess::RetreiveOutputStreams() {
  if (output_streams_)
    return;

  output_streams_ = output_streams_future_.get();
}

}  // namespace system
}  // namespace oven
