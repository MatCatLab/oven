#include "system/job.h"

#include <Windows.h>

#include <cassert>
#include <chrono>
#include <iostream>

#include "system/error.h"

namespace oven {
namespace system {
namespace {
const ULONG_PTR kJobNotificationCompletionKey = 0xbad;
}

void Job::Observer::HandleNotification(
    const HANDLE job_handle,
    OVERLAPPED* overlapped, const DWORD value) {
  const unsigned long process_id = (unsigned long)overlapped;
  switch (value) {
    case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
      OnAbnormalExitProcess(process_id);
      break;
    case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
      OnActiveProcessLimit();
      break;
    case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
      OnActiveProcessZero();
      break;
    case JOB_OBJECT_MSG_END_OF_JOB_TIME:
      OnEndOfJobTime();
      break;
    case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
      OnEndOfProcessTime(process_id);
      break;
    case JOB_OBJECT_MSG_EXIT_PROCESS:
      OnExitProcess(process_id);
      break;
    case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
      OnExitProcess(process_id);
      break;
    case JOB_OBJECT_MSG_NEW_PROCESS:
      OnNewProcess(process_id);
      break;
    case JOB_OBJECT_MSG_NOTIFICATION_LIMIT:
      OnNotification(job_handle, process_id);
      break;
    default:
      break;
  }
}

void Job::Observer::OnNotification(
    const HANDLE job_handle, const unsigned long process_id) {
  JOBOBJECT_LIMIT_VIOLATION_INFORMATION limit_violation;

  if (!::QueryInformationJobObject(job_handle,
      JobObjectLimitViolationInformation, &limit_violation,
      sizeof(limit_violation), NULL)) {
    OutputError(L"Unable to query job object information");
  }

  // TODO(matthewtff): Cleanup following outputs.
  if (limit_violation.ViolationLimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY) {
    std::wcout << L"Job reached memory limit with up to "
               << limit_violation.JobMemory << L" bytes consumed";
  }

  if (limit_violation.ViolationLimitFlags & JOB_OBJECT_LIMIT_JOB_TIME) {
    std::wcout << L"Job reached user-mode execution time limit with up to "
               << static_cast<std::uint64_t>(limit_violation.PerJobUserTime.QuadPart)
               << L" * 100ns consumed";
  }
}

Job::Job() : handle_(::CreateJobObjectW(NULL, NULL)) {}

Job::~Job() {
  stop_ = true;
  job_iocp_.Stop();
  if (listening_thread_.joinable()) {
    listening_thread_.join();
  }
}

bool Job::SetBasicLimits(const BasicLimits& limits) {
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_information;
  limit_information.JobMemoryLimit = static_cast<size_t>(limits.overall_memory_limit);
  limit_information.ProcessMemoryLimit = static_cast<size_t>(limits.per_process_memory_limit);
  limit_information.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = limits.cpu_time_limit.count() * 1000;
  limit_information.BasicLimitInformation.LimitFlags =
      JOB_OBJECT_LIMIT_JOB_TIME | JOB_OBJECT_LIMIT_JOB_MEMORY |
      JOB_OBJECT_LIMIT_PROCESS_MEMORY;
  if (!::SetInformationJobObject(handle_.get(), ::JobObjectExtendedLimitInformation,
          &limit_information, sizeof(limit_information))) {
    OutputError(L"Unable to set basic limits for job");
    return false;
  }

  return true;
}

bool Job::AssignProcess(const HANDLE process) {
  if (!::AssignProcessToJobObject(handle_.get(), process)) {
    return false;
  }

  if (!listening_thread_.joinable()) {
    listening_thread_ = std::thread(&Job::ListenForNotifications, this);
  }

  JOBOBJECT_ASSOCIATE_COMPLETION_PORT iocp_association;
  iocp_association.CompletionKey = PVOID(kJobNotificationCompletionKey);
  iocp_association.CompletionPort = job_iocp_.handle();

  if (!::SetInformationJobObject(handle_.get(), JobObjectAssociateCompletionPortInformation,
      &iocp_association, sizeof(iocp_association))) {
    return false;
  }

  return true;
}

void Job::ListenForNotifications() {
  while (!stop_) {
    OVERLAPPED* overlapped;
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    const IOCP::WaitResult wait_result = job_iocp_.Wait(
        std::chrono::seconds(1), &completion_key,
        &overlapped, &bytes_transferred);

    switch (wait_result) {
      case IOCP::WaitResult::kTimeout:
        continue;
      case IOCP::WaitResult::kStopped:
        return;
      case IOCP::WaitResult::kFailure:
        OutputError(L"Unable to dequeue message from iocp");
        return;
      case IOCP::WaitResult::kSuccess:
#if defined(ENABLE_ASSERTIONS)
        assert(kJobNotificationCompletionKey == completion_key &&
               "Received unexpected completion key");
#endif
        NotifyObservers(overlapped, bytes_transferred);
        break;
    }
  }
}

void Job::NotifyObservers(OVERLAPPED* overlapped, const DWORD value) {
  std::lock_guard lock(observers_guard_);
  for (Observer* observer : observers_) {
    observer->HandleNotification(handle_.get(), overlapped, value);
  }
}
}  // namespace system
}  // namespace oven
