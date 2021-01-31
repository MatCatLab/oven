#include "system/desktop.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace oven {
namespace system {

Desktop::Desktop(const std::wstring_view desktop_name,
                 const size_t heap_size) {
  desktop_handle_ = ::CreateDesktopExW(
    desktop_name.data(), NULL, NULL, 0, MAXIMUM_ALLOWED,
    NULL, heap_size, NULL);
}

Desktop::~Desktop() {
  Release();
}

Desktop::Desktop(Desktop&& other) noexcept : desktop_handle_(other.desktop_handle_) {
  other.desktop_handle_ = NULL;
}

Desktop& Desktop::operator=(Desktop&& other) noexcept {
  Release();
  desktop_handle_ = other.desktop_handle_;
  other.desktop_handle_ = NULL;
  return *this;
}

// static
std::optional<Desktop> Desktop::OpenInteractive() {
  const HDESK interactive_desktop_handle =
    ::OpenInputDesktop(0, false, MAXIMUM_ALLOWED);
  if (interactive_desktop_handle == NULL) {
    return std::optional<Desktop>();
  }
  return Desktop(interactive_desktop_handle);
}

std::optional<std::wstring> Desktop::GetName() const noexcept {
  DWORD length_needed = 0;
  if (!::GetUserObjectInformationW(desktop_handle_, UOI_NAME, NULL, 0, &length_needed) &&
      length_needed == 0) {
    return std::optional<std::wstring>();
  }
  std::wcerr << L"Buffer size required: " << length_needed << std::endl;
  std::vector<wchar_t> buffer(length_needed);
  if (!::GetUserObjectInformationW(desktop_handle_,
      UOI_NAME, buffer.data(), buffer.capacity(), &length_needed)) {
    return std::optional<std::wstring>();
  }
  std::wstring name(buffer.data());
  return std::wstring(buffer.data());
}

std::optional<size_t> Desktop::GetHeapSize() const noexcept {
  ULONG heap_size;
  DWORD length_needed;
  if (!::GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),
                                   UOI_HEAPSIZE, &heap_size, sizeof(heap_size),
                                   &length_needed)) {
    return std::optional<size_t>();
  }
  return static_cast<size_t>(heap_size);
}

bool Desktop::SetForCurrentThread() const {
  return ::SetThreadDesktop(desktop_handle_);
}

Desktop::Desktop(const HDESK desktop_handle) : desktop_handle_(desktop_handle) {
}

bool Desktop::Activate() const noexcept {
  return ::SwitchDesktop(desktop_handle_);
}

void Desktop::Release() noexcept {
  if (desktop_handle_ != NULL) {
    [[maybe_unused]] const bool desktop_was_closed = ::CloseDesktop(desktop_handle_);
#if defined(ENABLE_ASSERTIONS)
    assert(desktop_was_closed);
#endif
  }
}

}  // namespace system
}  // namespace oven
