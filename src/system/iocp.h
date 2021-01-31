#ifndef _OVEN_SYSTEM_IOCP_H_
#define _OVEN_SYSTEM_IOCP_H_

#include <chrono>

#include "system/scoped_handle.h"

namespace oven {
namespace system {

class IOCP {
 public:
  enum class WaitResult {
    kTimeout = 0,
    kStopped,
    kFailure,
    kSuccess,
  };
  IOCP();

  WaitResult Wait(const std::chrono::milliseconds timeout,
                  PULONG_PTR completion_key,
                  OVERLAPPED** overlapped, DWORD* bytes_tranferred);

  const HANDLE handle() const noexcept { return handle_.get(); }

  bool Stop() const noexcept;

 private:
  ScopedHandle handle_;
};

}  // namespace system
}  // namespace oven

#endif  // _OVEN_SYSTEM_IOCP_H_
