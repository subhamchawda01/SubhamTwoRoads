// =====================================================================================
//
//       Filename:  aws_worket_metadata.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/01/2014 07:01:04 PM
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
#include "basetrade/AWS/aws_worker_config_parser.hpp"

namespace HFSAT {
namespace AWS {

class AWSWorkerMetaDataHandler {
 private:
  std::map<std::string, HFSAT::AWS::AWSWorker*> aws_worker_metadata_;

 public:
  AWSWorkerMetaDataHandler() : aws_worker_metadata_() {}

  void LoadWorkerData() {
    std::ifstream worker_config_file_stream_;
    worker_config_file_stream_.open(AWS_WORKER_DATA_INFO_FILE, std::ios::in);

    if (!worker_config_file_stream_.is_open()) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::FILE_ISSUE, std::string("COULD NOT LOAD WORKER DATA INFO : ") +
                                                             std::string(AWS_WORKER_DATA_INFO_FILE));
      exit(1);
    }

#define CONFIG_MAX_LINE_SIZE 1024

    char buffer[CONFIG_MAX_LINE_SIZE];
    std::string line_read = "";

    HFSAT::AWS::AWSWorkerConfigParser aws_worker_config_parser;

    while (worker_config_file_stream_.good()) {
      memset((void*)(buffer), 0, sizeof(CONFIG_MAX_LINE_SIZE));
      worker_config_file_stream_.getline(buffer, CONFIG_MAX_LINE_SIZE);

      line_read = buffer;

      if (line_read.find("#") != std::string::npos) continue;

      aws_worker_config_parser.ParseWorkerConfigLineAndGenerateWorkerTokens(buffer, line_read.length(),
                                                                            aws_worker_metadata_);
    }

    worker_config_file_stream_.close();

#undef CONFIG_MAX_LINE_SIZE
  }

  void ResetWorkerRunningInfo() {
    std::map<std::string, HFSAT::AWS::AWSWorker*>::iterator aws_wkr_itr;

    for (aws_wkr_itr = aws_worker_metadata_.begin(); aws_wkr_itr != aws_worker_metadata_.end(); aws_wkr_itr++) {
      (aws_wkr_itr->second)->total_running_jobs_ = 0;
    }
  }

  void AnalyzeJobsAndUpdateWorkerData(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_data_) {
    ResetWorkerRunningInfo();

    std::map<uint64_t, HFSAT::AWS::AWSJob*>::iterator ajd_itr;

    for (ajd_itr = _aws_jobs_data_.begin(); ajd_itr != _aws_jobs_data_.end(); ajd_itr++) {
      if (aws_worker_metadata_.find((ajd_itr->second)->assigned_to_worker_) == aws_worker_metadata_.end()) {
        HFSAT::AWS::ReportAWSError(HFSAT::AWS::SCHED_ISSUE, std::string("JOB ASSIGNED TO UKNOWN WORKER : ") +
                                                                std::string((ajd_itr->second)->ToString()));
        continue;

      } else {
        if ((ajd_itr->second)->job_status_ == HFSAT::AWS::PROCESSING) {
          aws_worker_metadata_[(ajd_itr->second)->assigned_to_worker_]->total_running_jobs_++;
        }

        if ((aws_worker_metadata_[(ajd_itr->second)->assigned_to_worker_]->total_running_jobs_) >
            (aws_worker_metadata_[(ajd_itr->second)->assigned_to_worker_]->total_cores_available_)) {
          ReportAWSError(SCHED_ISSUE, std::string("MORE JOBS RUNNING ON A WORKER THAN CAPACITY") +
                                          std::string((ajd_itr->second)->ToString()));
        }
      }
    }
  }

  void UpdateWorkerDataWithAssignedTask(std::string _worker_machine_) {
    (aws_worker_metadata_[_worker_machine_]->total_running_jobs_)++;
  }

  void UpdateAndSaveWorkerMetaData() {
    std::ofstream worker_config_file_stream_;

    worker_config_file_stream_.open(AWS_WORKER_DATA_INFO_FILE, std::ios::out);

    if (!worker_config_file_stream_.is_open()) {
      ReportAWSError(FILE_ISSUE, std::string("FAILED TO OPEN WORKER CONFIG FOR WRITING : ") +
                                     std::string(AWS_WORKER_DATA_INFO_FILE));
      return;
    }

    std::map<std::string, HFSAT::AWS::AWSWorker*>::iterator aws_wkr_itr;

    for (aws_wkr_itr = aws_worker_metadata_.begin(); aws_wkr_itr != aws_worker_metadata_.end(); aws_wkr_itr++) {
      worker_config_file_stream_ << (aws_wkr_itr->second)->ToString();
    }

    worker_config_file_stream_.close();
  }

  std::map<std::string, HFSAT::AWS::AWSWorker*>& GetWorkerMetaData() { return aws_worker_metadata_; }
};
}
}
