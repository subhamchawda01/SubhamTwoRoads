// =====================================================================================
//
//       Filename:  aws_defines.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/11/2014 06:09:57 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <vector>
#include <map>
#include <cstdlib>
#include <string.h>

#include "basetrade/AWS/aws_error_defines.hpp"

#define MAX_CORES 240
#define RESERVED_CORES 16
#define DEFAULT_QUEUE_PRIORITY 128

#define PROCESS_METADATA_SYSTEM_FILE "/mnt/sdf/AWSScheduler/.aws_process_metadata_system.aws"
#define AWS_QUEUE_CONFIG_FILE "/mnt/sdf/AWSScheduler/AWSQueue.dat"
#define AWS_JOBS_METADATA_BIN_FILE "/mnt/sdf/AWSScheduler/.aws_jobs_metadata.aws"
#define AWS_QUEUES_METADATA_BIN_FILE "/mnt/sdf/AWSScheduler/.queues_jobs_metadata.aws"
#define AWS_WORKER_DATA_INFO_FILE "/mnt/sdf/AWSScheduler/.aws_worker_data.info"
#define AWS_JOBS_PATH "/mnt/sdf/AWSScheduler"
#define AWS_BATCH_RUN "/mnt/sdf/AWSScheduler/AWSBatchRun.bat"

#define URGENT_QUEUE 911
#define TOP_3_QUEUE 3
#define MEDIUM_PRIORITY_QUEUE 10

#define AWS_TOP_QUEUE_ALLOCATION_FACTOR 0.7
#define AWS_MEDIUM_QUEUE_ALLOCATION_FACTOR 0.2
#define AWS_MEDIUM_DEFAULT_ALLOCATION_FACTOR 0.1

namespace HFSAT {
namespace AWS {

enum JobType { URGENT = 1, SHORTRUN, LONGRUN, DEFAULT };
enum JobPriority { HIGH = 1, MEDIUM, LOW, UNDEFINED };
enum JobStatus { QUEUED = 1, PROCESSING, DONE, FAILED };
enum ProcessStatus { RUNNING = 1, NOT_RUNNING, MULTIPLE_RUNNING, UNKNOWN };

std::string GetProcessStatusString(ProcessStatus _process_status_) {
  switch (_process_status_) {
    case RUNNING: {
      return "RUNNING";
    } break;
    case NOT_RUNNING: {
      return "NOT_RUNNING";
    } break;
    case MULTIPLE_RUNNING: {
      return "MULTIPLE_RUNNING";
    } break;

    default: { return "INVALID_STATUS"; } break;
  }
}

ProcessStatus GetProcessStatusFromString(std::string _process_status_string_) {
  if (_process_status_string_ == "RUNNING")
    return RUNNING;
  else if (_process_status_string_ == "NOT_RUNNING")
    return NOT_RUNNING;
  else if (_process_status_string_ == "MULTIPLE_RUNNING")
    return MULTIPLE_RUNNING;
  else
    return UNKNOWN;
}

struct AWSWorker {
  std::string worker_unique_id_;
  int32_t total_cores_available_;
  int32_t total_running_jobs_;
  int32_t total_urgent_running_jobs_;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << worker_unique_id_ << " " << total_cores_available_ << " " << total_running_jobs_ << " "
               << total_urgent_running_jobs_;

    return t_temp_oss.str();
  }
};

struct AWSServerMetaData {
  int32_t running_processes_;
  int32_t urgent_running_;
  int32_t reserved_urgent_cores_;
  int32_t total_available_cores_;
  std::vector<AWSWorker*> aws_worker_machines_;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    return t_temp_oss.str();
  }
};

struct ProcessMetaData {
  uint64_t metadata_update_time_;  // Will act as unique identifier
  ProcessStatus process_status_;
  std::string worker_machine_;
  std::string separator_;
  std::string job_exec_with_arg_;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << metadata_update_time_ << " " << GetProcessStatusString(process_status_) << " " << worker_machine_
               << " " << job_exec_with_arg_ << "\n";

    return t_temp_oss.str();
  }
};

struct AWSJob {
  std::string uniq_queue_name_;

  JobType job_type_;
  JobPriority job_priority_;
  JobStatus job_status_;

  std::string exec_with_arguments_;

  int32_t expected_run_duration_hours_;
  int32_t last_run_time_;

  uint64_t unique_job_id_;

  std::string assigned_to_worker_;

  int32_t hours_to_run[24];
  int32_t days_of_week[7];

  bool run_all_hours_;
  bool run_all_days_;
  bool run_once_;

  int32_t run_counter_for_cyclic_queues_;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    return t_temp_oss.str();
  }
};

struct AWSQueue {
  std::string uniq_queue_name_;
  int32_t limit_max_cores_usage_;
  int32_t queue_priority_;
  bool run_once_;
  std::string queue_owner_;
  std::string queue_communication_;
  int32_t total_running_tasks_for_queue_;
  uint64_t last_processing_time_;
  int32_t picked_up_counter_;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    return t_temp_oss.str();
  }
};
}
}
