#ifndef _OVEN_SYSTEM_PIPE_H_
#define _OVEN_SYSTEM_PIPE_H_

#include <Windows.h>

#include "system/scoped_handle.h"

namespace oven {
namespace system {
class Pipe {
 public:
  Pipe();
  ~Pipe();

  Pipe(const Pipe&) = delete;
  Pipe(Pipe&&) = default;

  Pipe& operator=(const Pipe&) = delete;
  Pipe& operator=(Pipe&&) = default;

  bool IsValid() { return in_ && out_; }

  ScopedHandle& in() { return in_; }
  ScopedHandle& out() { return out_; }

  OVERLAPPED& overlapped() { return overlapped_; }

 private:
  OVERLAPPED overlapped_;
  ScopedHandle in_;
  ScopedHandle out_;
};
}  // namespace system
}  // namespace oven

#endif  // _OVEN_SYSTEM_PIPE_H_
