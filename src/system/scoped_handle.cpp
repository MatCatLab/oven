#include "system/scoped_handle.h"

#include "error.h"

namespace oven {
namespace system {
ScopedHandle::ScopedHandle(const HANDLE open_handle)
    : open_handle_(open_handle) {
}

ScopedHandle::ScopedHandle(ScopedHandle&& other) noexcept
    : open_handle_(other.open_handle_) {
  other.open_handle_ = NULL;
}

ScopedHandle& ScopedHandle::operator=(ScopedHandle&& other) noexcept {
  reset();
  open_handle_ = other.open_handle_;
  other.open_handle_ = NULL;
  return *this;
}

void ScopedHandle::reset(const HANDLE new_open_handle) {
  if (IsValid()) {
    if (!::CloseHandle(open_handle_)) {
      OutputError(L"Unable to close handle");
    }
  }
  open_handle_ = new_open_handle;
}
}  // namespace system
}  // namespace oven
