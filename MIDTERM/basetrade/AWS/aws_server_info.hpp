// =====================================================================================
//
//       Filename:  aws_server_info.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/02/2014 09:27:07 AM
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

#include "basetrade/AWS/aws_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace HFSAT {
namespace AWS {

class AWSServerInfo {
 private:
  HFSAT::AWS::AWSServerMetaData* aws_server_metadata_;

 public:
  void AnalyzeWorkerInfoAndGenerateServerData(std::map<std::string, HFSAT::AWS::AWSWorker*> _aws_worker_metadata_) {
    aws_server_metadata_ = new HFSAT::AWS::AWSServerMetaData();

    // TODO Load From Config
    aws_server_metadata_->reserved_urgent_cores_ = RESERVED_CORES;

    std::map<std::string, HFSAT::AWS::AWSWorker*>::iterator aws_wkr_itr;

    int32_t total_running_jobs_counter = 0;
    int32_t total_available_cores = 0;
    int32_t total_urgent_running_jobs_counter = 0;

    for (aws_wkr_itr = _aws_worker_metadata_.begin(); aws_wkr_itr != _aws_worker_metadata_.end(); aws_wkr_itr++) {
      total_running_jobs_counter += ((aws_wkr_itr->second)->total_running_jobs_);
      total_available_cores += ((aws_wkr_itr->second)->total_cores_available_);
      total_urgent_running_jobs_counter += ((aws_wkr_itr->second)->total_urgent_running_jobs_);

      // Same Pointer
      (aws_server_metadata_->aws_worker_machines_).push_back((aws_wkr_itr->second));
    }

    aws_server_metadata_->running_processes_ = total_running_jobs_counter;
    aws_server_metadata_->urgent_running_ = total_urgent_running_jobs_counter;
    aws_server_metadata_->total_available_cores_ =
        (total_available_cores - aws_server_metadata_->reserved_urgent_cores_);
  }

  HFSAT::AWS::AWSServerMetaData* GetServerMetaData() { return aws_server_metadata_; }

  HFSAT::AWS::AWSWorker* GetNextUrgentFreeWorker() {
    for (unsigned int worker_counter = 0; worker_counter < (aws_server_metadata_->aws_worker_machines_).size();
         worker_counter++) {
      if ((aws_server_metadata_->aws_worker_machines_[worker_counter])->total_running_jobs_ <
          (aws_server_metadata_->aws_worker_machines_[worker_counter])->total_cores_available_) {
        (aws_server_metadata_->aws_worker_machines_[worker_counter])->total_running_jobs_++;
        aws_server_metadata_->urgent_running_++;
        return (aws_server_metadata_->aws_worker_machines_[worker_counter]);
      }
    }

    ReportAWSError(SCHED_ISSUE, std::string("NO FREE WORKERS ! SOMETHING MUST HAVE WENT WRONG IN SCHEDULING"));
    return NULL;
  }

  HFSAT::AWS::AWSWorker* GetNextFreeWorker() {
    for (unsigned int worker_counter = 0; worker_counter < (aws_server_metadata_->aws_worker_machines_).size();
         worker_counter++) {
      if ((aws_server_metadata_->aws_worker_machines_[worker_counter])->total_running_jobs_ <
          (aws_server_metadata_->aws_worker_machines_[worker_counter])->total_cores_available_) {
        (aws_server_metadata_->aws_worker_machines_[worker_counter])->total_running_jobs_++;
        aws_server_metadata_->running_processes_++;
        return (aws_server_metadata_->aws_worker_machines_[worker_counter]);
      }
    }

    ReportAWSError(SCHED_ISSUE, std::string("NO FREE WORKERS ! SOMETHING MUST HAVE WENT WRONG IN SCHEDULING"));
    return NULL;
  }
};
}
}
