// =====================================================================================
//
//       Filename:  aws_jobs_loader.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/11/2014 07:57:59 PM
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
#include <fstream>
#include <cstdlib>

#include "basetrade/AWS/aws_jobs_parser.hpp"

namespace HFSAT {
namespace AWS {

class AWSJobsManager {
 private:
  std::vector<HFSAT::AWS::AWSJob*> aws_jobs_data_;
  std::map<double, HFSAT::AWS::AWSJob*> aws_runnable_sorted_jobs_list_;

 public:
  AWSJobsManager(std::string _queue_file_) {
    std::ifstream jobs_config_file_stream_;

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << AWS_JOBS_PATH << "/" << _queue_file_;

    jobs_config_file_stream_.open(t_temp_oss_.str(), std::ios::in);

    if (!jobs_config_file_stream_.is_open()) {
      ReportAWSError(FILE_ISSUE, std::string("COULD NOT OPEN JOBS CONFIG FILE : ") + std::string(t_temp_oss_.str()));
      exit(1);
    }

#define CONFIG_MAX_LINE_SIZE 1024

    char buffer[CONFIG_MAX_LINE_SIZE];
    std::string line_read = "";

    HFSAT::AWS::AWSJobParser aws_job_parser;

    while (jobs_config_file_stream_.good()) {
      memset((void*)(buffer), 0, sizeof(CONFIG_MAX_LINE_SIZE));
      jobs_config_file_stream_.getline(buffer, CONFIG_MAX_LINE_SIZE);

      line_read = buffer;

      if (line_read.find("#") != std::string::npos) continue;

      aws_job_parser.ParseJobLineAndGenerateJobToken(buffer, line_read.length(), aws_jobs_data_);
    }

    jobs_config_file_stream_.close();

#undef CONFIG_MAX_LINE_SIZE
  }

  HFSAT::AWS::AWSJob* GetNextRunnableUrgentJob(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_metadata_) {
    for (unsigned int job_counter = 0; job_counter < aws_jobs_data_.size(); job_counter++) {
      if (_aws_jobs_metadata_.find(aws_jobs_data_[job_counter]->unique_job_id_) == _aws_jobs_metadata_.end()) {
        return (aws_jobs_data_[job_counter]);
      }
    }

    for (unsigned int job_counter = 0; job_counter < aws_jobs_data_.size(); job_counter++) {
      if (_aws_jobs_metadata_.find(aws_jobs_data_[job_counter]->unique_job_id_) != _aws_jobs_metadata_.end()) {
        if (aws_jobs_data_[job_counter]->job_status_ != PROCESSING) {
          return (aws_jobs_data_[job_counter]);
        }
      }
    }
  }

  void AnalyzeMetaDataAndPrepareRunnableJobsQueue(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_metadata_) {
    for (unsigned int job_counter = 0; job_counter < aws_jobs_data_.size(); job_counter++) {
      HFSAT::AWS::AWSJob* this_aws_job = aws_jobs_data_[job_counter];

      if (this_aws_job->job_type_ == HFSAT::AWS::URGENT) continue;

      if (_aws_jobs_metadata_.find(this_aws_job->unique_job_id_) != _aws_jobs_metadata_.end()) {
        if (_aws_jobs_metadata_[this_aws_job->unique_job_id_]->job_status_ == HFSAT::AWS::PROCESSING ||
            _aws_jobs_metadata_[this_aws_job->unique_job_id_]->job_status_ == HFSAT::AWS::DONE)
          continue;
      }

      struct timeval curr_time;
      gettimeofday(&(curr_time), NULL);

      uint64_t current_time = (curr_time.tv_sec * 1000000) + (curr_time.tv_usec);
      unsigned int run_only_once = ((this_aws_job->run_once_) == true) ? 2 : 1;

      double computational_units_for_this_job =
          ((this_aws_job->job_priority_) * (this_aws_job->job_type_) * (this_aws_job->run_counter_for_cyclic_queues_)) /
          (double)((current_time - (this_aws_job->unique_job_id_) * run_only_once));

      while (aws_runnable_sorted_jobs_list_.find(computational_units_for_this_job) !=
             aws_runnable_sorted_jobs_list_.end()) {
        computational_units_for_this_job += 0.000001;
      }

      aws_runnable_sorted_jobs_list_[computational_units_for_this_job] = this_aws_job;
    }
  }

  double GetJobsRunCount() {
    double avg_jobs_runcount = 1.0;
    unsigned int total_jobs_runcount = 0;

    if (aws_jobs_data_.size() == 0) {
      return 1.0;
    }

    for (unsigned int jobs_counter = 0; jobs_counter < aws_jobs_data_.size(); jobs_counter++) {
      total_jobs_runcount += (aws_jobs_data_[jobs_counter]->run_counter_for_cyclic_queues_);
    }

    avg_jobs_runcount = (total_jobs_runcount / (double)(aws_jobs_data_.size()));

    return avg_jobs_runcount;
  }

  double GetJobsAvgPriority() {
    double avg_jobs_priority = 1.0;
    unsigned int total_jobs_priority = 0;

    if (aws_jobs_data_.size() == 0) {
      return 1.0;
    }

    for (unsigned int jobs_counter = 0; jobs_counter < aws_jobs_data_.size(); jobs_counter++) {
      total_jobs_priority += (aws_jobs_data_[jobs_counter]->job_priority_);
    }

    avg_jobs_priority = (total_jobs_priority / (double)(aws_jobs_data_.size()));

    return avg_jobs_priority;
  }

  double GetJobsAvgIdleTime(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_metadata_) {
    double avg_idle_time = 1.0;
    unsigned long long total_idle_time = 0;

    struct timeval curr_time;
    gettimeofday(&(curr_time), NULL);

    uint64_t current_time = (curr_time.tv_sec * 1000000) + (curr_time.tv_usec);

    if (aws_jobs_data_.size() == 0) {
      return 1.0;
    }

    for (unsigned int jobs_counter = 0; jobs_counter < aws_jobs_data_.size(); jobs_counter++) {
      if (_aws_jobs_metadata_.find(aws_jobs_data_[jobs_counter]->unique_job_id_) != _aws_jobs_metadata_.end()) continue;

      total_idle_time += (current_time - (aws_jobs_data_[jobs_counter]->unique_job_id_));
    }

    avg_idle_time = (total_idle_time / (double)(aws_jobs_data_.size()));

    if (avg_idle_time == 0) return 1.0;

    return avg_idle_time;
  }

  double GetJobsAvgType() {
    double avg_jobs_type = 1.0;
    unsigned int total_jobs_type = 0;

    if (aws_jobs_data_.size() == 0) {
      return 1.0;
    }

    for (unsigned int jobs_counter = 0; jobs_counter < aws_jobs_data_.size(); jobs_counter++) {
      if (aws_jobs_data_[jobs_counter]->job_type_ == HFSAT::AWS::URGENT) continue;

      total_jobs_type += (aws_jobs_data_[jobs_counter]->job_type_);
    }

    avg_jobs_type = (total_jobs_type / (double)(aws_jobs_data_.size()));

    return avg_jobs_type;
  }

  double GetAvgOneTimeRunJobs() {
    double avg_onetime_jobs = 1.0;
    unsigned int total_onetime_jobs = 0;

    if (aws_jobs_data_.size() == 0) {
      return 1.0;
    }

    for (unsigned int jobs_counter = 0; jobs_counter < aws_jobs_data_.size(); jobs_counter++) {
      total_onetime_jobs += (aws_jobs_data_[jobs_counter]->run_once_ == true ? 1 : 0);
    }

    avg_onetime_jobs = total_onetime_jobs;

    if (avg_onetime_jobs == 0) return 1.0;

    return avg_onetime_jobs;
  }

  double GetComputationalUnits(std::map<uint64_t, HFSAT::AWS::AWSJob*>& aws_jobs_metadata_) {
    double avg_jobs_runcount = GetJobsRunCount();
    double avg_jobs_priority = GetJobsAvgPriority();
    double avg_jobs_idle_time = GetJobsAvgIdleTime(aws_jobs_metadata_);
    double avg_jobs_type = GetJobsAvgType();
    double avg_jobs_onetime_run = GetAvgOneTimeRunJobs();

    double computational_units =
        (avg_jobs_priority * avg_jobs_type * avg_jobs_runcount) / (avg_jobs_idle_time * avg_jobs_onetime_run);

    return computational_units;
  }

  std::vector<HFSAT::AWS::AWSJob*> AllocateJobsWithFreeCores(unsigned int _cores_alloted_to_this_queue_,
                                                             unsigned int& _cores_actually_allocated_) {
    if (aws_runnable_sorted_jobs_list_.size() < _cores_alloted_to_this_queue_) {
      _cores_actually_allocated_ = aws_runnable_sorted_jobs_list_.size();

    } else {
      _cores_actually_allocated_ = _cores_alloted_to_this_queue_;
    }

    std::vector<HFSAT::AWS::AWSJob*> allocated_jobs_vec;
    std::map<double, HFSAT::AWS::AWSJob*>::iterator arj_itr;

    for (arj_itr = aws_runnable_sorted_jobs_list_.begin(); arj_itr != aws_runnable_sorted_jobs_list_.end(); arj_itr++) {
      allocated_jobs_vec.push_back((arj_itr->second));
    }

    return allocated_jobs_vec;
  }
};
}
}
