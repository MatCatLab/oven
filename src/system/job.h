#ifndef _OVEN_SYSTEM_JOB_H_
#define _OVEN_SYSTEM_JOB_H_

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "system/iocp.h"
#include "system/scoped_handle.h"

namespace oven {
namespace system {
// Manages an unnamed job object.
class Job {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;
    // In all the following obervations |process_id| may equal 0 if it's value
    // wasn't delivered properly.

    // Indicates that a process associated with the job exited with an
    // exit code that indicates an abnormal exit
    virtual void OnAbnormalExitProcess(const unsigned long process_id) {};

    // Indicates that the active process limit has been exceeded.
    virtual void OnActiveProcessLimit() {};

    // Indicates that the active process count has been decremented to 0.
    virtual void OnActiveProcessZero() {};

    // Indicates that the JOB_OBJECT_POST_AT_END_OF_JOB option is in
    // effect and the end-of-job time limit has been reached.
    // Upon posting this message, the time limit is canceled
    // and the job's processes can continue to run.
    virtual void OnEndOfJobTime() {};

    // Indicates that a process has exceeded a per-process time limit.
    // The system sends this message after the process termination has been requested.
    virtual void OnEndOfProcessTime(const unsigned long process_id) {}

    // Indicates that a process associated with the job has exited.
    virtual void OnExitProcess(const unsigned long process_id) {}

    // Indicates that a process associated with the job caused
    // the job to exceed the job-wide memory limit (if one is in effect).
    virtual void OnJobMemoryLimit() {}

    // Indicates that a process has been added to the job.
    virtual void OnNewProcess(const unsigned long process_id) {}

    // Indicates that a process associated with a job that has
    // registered for resource limit notifications has exceeded
    // one or more limits.
    void OnNotification(const HANDLE job_handle, const unsigned long process_id);

    // TODO(matthewtff): Consider adding OnProcessMemoryLimit.

    void HandleNotification(const HANDLE job_handle,
        OVERLAPPED* overlapped, const DWORD value);
  };

  struct BasicLimits {
    std::uint64_t overall_memory_limit;
    std::uint64_t per_process_memory_limit;
    std::chrono::milliseconds cpu_time_limit;
  };

  Job();
  ~Job();

  // Just forbid job moves for now. Implement if required.
  Job(Job&&) = delete;
  Job& operator=(Job&&) = delete;

  bool SetBasicLimits(const BasicLimits& limits);

  // Assigns process to job and starts listening for notifications on iocp.
  bool AssignProcess(const HANDLE process);

  // Job doesn't own observers, so it's caller's responsibility to make sure
  // observers do outlive job.
  void AddObserver(Observer* observer) {
    std::lock_guard lock(observers_guard_);
    if (std::find(observers_.begin(), observers_.end(), observer) ==
            observers_.end()) {
      observers_.push_back(observer);
    }
  }

 private:
  void ListenForNotifications();
  void NotifyObservers(OVERLAPPED* overlapped, const DWORD value);

  // Use our own |stop_| flag in case iocp get's a large
  // queue of never-ending notifications for any reason.
  std::thread listening_thread_;
  std::atomic_bool stop_ = false;  // True, if job is destructing.

  std::mutex observers_guard_;
  std::vector<Observer*> observers_;
  
  ScopedHandle handle_;
  IOCP job_iocp_;
};
}  // namespace system
}  // namespace oven

#endif _OVEN_SYSTEM_JOB_H_
