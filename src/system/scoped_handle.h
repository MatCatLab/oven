#ifndef _OVEN_SYSTEM_SCOPED_HANDLE_H_
#define _OVEN_SYSTEM_SCOPED_HANDLE_H_

#include <Windows.h>

namespace oven {
namespace system {
class ScopedHandle {
 public:
  ScopedHandle() : ScopedHandle(NULL) {}
  explicit ScopedHandle(HANDLE open_handle);
  ~ScopedHandle() noexcept { reset(); }

  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle(ScopedHandle&& other) noexcept;

  ScopedHandle& operator=(const ScopedHandle&) = delete;
  ScopedHandle& operator=(ScopedHandle&& other) noexcept;

  bool IsValid() const noexcept {
    return open_handle_ != NULL && open_handle_ != INVALID_HANDLE_VALUE;
  }

  HANDLE get() const noexcept { return open_handle_; }
  void reset() noexcept { reset(NULL); }
  void reset(const HANDLE new_open_handle);

  operator bool() const noexcept { return IsValid(); }

 private:
  HANDLE open_handle_;
};
}  // namespace system
}  // namespace oven

#endif  // _OVEN_SYSTEM_SCOPED_HANDLE_H_
