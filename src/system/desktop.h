#ifndef _OVEN_SYSTEM_DESKTOP_H_
#define _OVEN_SYSTEM_DESKTOP_H_

#include <Windows.h>

#include <optional>
#include <string>
#include <string_view>

#include "system/error.h"

namespace oven {
namespace system {

class Desktop {
 public:
  // Creates new desktop within the current window station.
  Desktop(const std::wstring_view desktop_name, const size_t heap_size);
  ~Desktop();

  Desktop(const Desktop&) = delete;
  Desktop(Desktop&& other) noexcept;

  Desktop& operator=(const Desktop&) = delete;
  Desktop& operator=(Desktop&& other) noexcept;

  static std::optional<Desktop> OpenInteractive();

  bool IsValid() const noexcept { return desktop_handle_ != NULL; }

  std::optional<std::wstring> GetName() const noexcept;

  std::optional<size_t> GetHeapSize() const noexcept;

  bool SetForCurrentThread() const;

 private:
  friend class ScopedDesktopActivation;

  explicit Desktop(const HDESK desktop_handle);
  bool Activate() const noexcept;
  void Release() noexcept;

  HDESK desktop_handle_;
};

class ScopedDesktopActivation {
 public:
  ScopedDesktopActivation(const Desktop& new_active_desktop)
      : initial_active_desktop_(Desktop::OpenInteractive()),
        success_(initial_active_desktop_.has_value() && new_active_desktop.Activate()) {
  }

  ~ScopedDesktopActivation() {
    if (initial_active_desktop_ && !initial_active_desktop_->Activate()) {
      OutputError(L"Unable to switch to initial active desktop");
    }
  }

  ScopedDesktopActivation(ScopedDesktopActivation&& other) = default;
  ScopedDesktopActivation(const ScopedDesktopActivation& other) = delete;

  ScopedDesktopActivation& operator=(ScopedDesktopActivation&& other) = default;
  ScopedDesktopActivation& operator=(const ScopedDesktopActivation& other) = delete;
                                           
  bool Success() const { return success_; }

 private:
  std::optional<Desktop> initial_active_desktop_;
  const bool success_;
};

}  // namespace system
}  // namespace oven

#endif  // _OVEN_SYSTEM_DESKTOP_H_
