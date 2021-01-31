#include "system/pipe.h"

#include <algorithm>
#include <random>

#include "system/error.h"

namespace {
std::wstring GeneratePipeName() {
  thread_local static std::mt19937 generator{std::random_device{}()};
  std::wstring name = L"0123456789";
  std::shuffle(name.begin(), name.end(), generator);
  return name;
}
}  // anonymous namespace

namespace oven {
namespace system {
Pipe::Pipe() : overlapped_({0}) {
  const std::wstring name = LR"RAW(\\.\pipe\oven-)RAW" + GeneratePipeName();

  in_.reset(::CreateNamedPipeW(
      name.c_str(),
      PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_REJECT_REMOTE_CLIENTS, 1, 
      0, 0, 0, NULL));

  if (!in_) {
    OutputError(L"Unable to create named pipe");
    return;
  }

  SECURITY_ATTRIBUTES security_attributes;
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = true;
  security_attributes.lpSecurityDescriptor = NULL;
  out_.reset(::CreateFileW(name.c_str(), GENERIC_WRITE, 0, &security_attributes,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
  if (!out_) {
    OutputError(L"Unable to open named pipe");
    return;
  }

  if (!::ConnectNamedPipe(in_.get(), &overlapped_)) {
    const auto error = ::GetLastError();
    if (error != ERROR_PIPE_CONNECTED) {
      OutputError(L"Unable to connect named pipe");
      return;
    }
  }
}

Pipe::~Pipe() {
  if (IsValid()) {
    OutputError(L"Closing valid pipe!");
  }
}
}  // namespace system
}  // namespace oven
