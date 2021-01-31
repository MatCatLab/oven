#include "system/iocp.h"

#include <Windows.h>

#include <cassert>

namespace oven {
namespace system {
namespace {
const ULONG_PTR kStopCompletionKey = 0xdeadbeef;
}

IOCP::IOCP() :
    handle_(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) {}

IOCP::WaitResult IOCP::Wait(
    const std::chrono::milliseconds timeout, PULONG_PTR completion_key,
    OVERLAPPED** overlapped, DWORD* bytes_tranferred) {
  if (!::GetQueuedCompletionStatus(handle_.get(), bytes_tranferred,
                                   completion_key, overlapped,
                                   static_cast<DWORD>(timeout.count()))) {
    const auto last_error = ::GetLastError();
    return last_error == WAIT_TIMEOUT ? WaitResult::kTimeout : WaitResult::kFailure;
  }
  if (*completion_key == kStopCompletionKey) {
    return WaitResult::kStopped;
  }

  return WaitResult::kSuccess;
}

bool IOCP::Stop() const noexcept {
  return ::PostQueuedCompletionStatus(handle_.get(), 0, kStopCompletionKey, NULL);
}
}  // namespace system
}  // namespace oven
