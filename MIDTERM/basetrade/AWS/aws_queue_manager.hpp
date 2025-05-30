// =====================================================================================
//
//       Filename:  aws_config_loader.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/11/2014 06:08:17 PM
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
#include <map>
#include <algorithm>
#include "basetrade/AWS/aws_defines.hpp"
#include "basetrade/AWS/aws_config_parser.hpp"
#include "basetrade/AWS/aws_jobs_manager.hpp"

namespace HFSAT {
namespace AWS {

class AWSQueueManager {
 private:
  std::map<std::string, HFSAT::AWS::AWSQueue*> aws_queue_data_;
  std::map<int, std::vector<HFSAT::AWS::AWSQueue*>*> priority_sorted_map_;
  std::map<double, HFSAT::AWS::AWSQueue*> computational_units_to_queue_map_;
  std::map<double, HFSAT::AWS::AWSQueue*> runnable_computational_units_to_queue_map_;
  std::map<std::string, HFSAT::AWS::AWSJobsManager*> queue_to_jobs_map_;
  std::map<std::string, unsigned int> queues_to_cores_allocation_map_;

 public:
  void AnalyzeQueueAllocationAndPenalizeWithServerData(HFSAT::AWS::AWSServerMetaData* _aws_server_data_) {
    int32_t current_queue_allocation_for_total_cores_top_3 = 0;
    int32_t current_queue_allocation_for_total_cores_medium = 0;
    int32_t current_queue_allocation_for_total_cores_default = 0;
    int32_t current_queue_allocation_for_total_cores_urgent = 0;

    std::map<std::string, HFSAT::AWS::AWSQueue*>::iterator aws_queue_itr;

    for (aws_queue_itr = aws_queue_data_.begin(); aws_queue_itr != aws_queue_data_.end(); aws_queue_itr++) {
      if ((aws_queue_itr->second)->queue_priority_ == URGENT_QUEUE) {
        current_queue_allocation_for_total_cores_urgent += (aws_queue_itr->second)->limit_max_cores_usage_;
        continue;

      } else if ((aws_queue_itr->second)->queue_priority_ <= TOP_3_QUEUE) {
        current_queue_allocation_for_total_cores_top_3 += (aws_queue_itr->second)->limit_max_cores_usage_;

      } else if (((aws_queue_itr->second)->queue_priority_ > TOP_3_QUEUE) &&
                 ((aws_queue_itr->second)->queue_priority_ <= MEDIUM_PRIORITY_QUEUE)) {
        current_queue_allocation_for_total_cores_medium += (aws_queue_itr->second)->limit_max_cores_usage_;

      } else {
        current_queue_allocation_for_total_cores_default += ((aws_queue_itr->second)->limit_max_cores_usage_);
      }
    }

    double penalize_factor_urgent = ((int)((_aws_server_data_->reserved_urgent_cores_))) /
                                    (double)(current_queue_allocation_for_total_cores_urgent);
    double penalize_factor_top_3 =
        ((int)((_aws_server_data_->total_available_cores_) * AWS_TOP_QUEUE_ALLOCATION_FACTOR)) /
        (double)(current_queue_allocation_for_total_cores_top_3);
    double penalize_factor_medium =
        ((int)((_aws_server_data_->total_available_cores_) * AWS_MEDIUM_QUEUE_ALLOCATION_FACTOR)) /
        (double)(current_queue_allocation_for_total_cores_medium);
    double penalize_factor_default =
        ((int)((_aws_server_data_->total_available_cores_) * AWS_MEDIUM_DEFAULT_ALLOCATION_FACTOR)) /
        (double)(current_queue_allocation_for_total_cores_default);

    for (aws_queue_itr = aws_queue_data_.begin(); aws_queue_itr != aws_queue_data_.end(); aws_queue_itr++) {
      if ((aws_queue_itr->second)->queue_priority_ == URGENT_QUEUE) {
        (aws_queue_itr->second)->limit_max_cores_usage_ *= penalize_factor_urgent;

      } else if ((aws_queue_itr->second)->queue_priority_ <= TOP_3_QUEUE) {
        (aws_queue_itr->second)->limit_max_cores_usage_ *= (penalize_factor_top_3);

      } else if (((aws_queue_itr->second)->queue_priority_ > TOP_3_QUEUE) &&
                 ((aws_queue_itr->second)->queue_priority_ <= MEDIUM_PRIORITY_QUEUE)) {
        (aws_queue_itr->second)->limit_max_cores_usage_ *= (penalize_factor_medium);

      } else {
        (aws_queue_itr->second)->limit_max_cores_usage_ *= (penalize_factor_default);
      }
    }
  }

  void LoadAllJobsAvailable() {
    std::map<std::string, HFSAT::AWS::AWSQueue*>::iterator all_queue_itr;

    for (all_queue_itr = aws_queue_data_.begin(); all_queue_itr != aws_queue_data_.end(); all_queue_itr++) {
      if (queue_to_jobs_map_.find(all_queue_itr->first) == queue_to_jobs_map_.end()) {
        HFSAT::AWS::AWSJobsManager* new_aws_jobs_manager = new HFSAT::AWS::AWSJobsManager(all_queue_itr->first);
        queue_to_jobs_map_[all_queue_itr->first] = new_aws_jobs_manager;
      }
    }
  }

  double GenerateQueueComputationUnits(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_metadata_) {
    std::map<std::string, HFSAT::AWS::AWSQueue*>::iterator aws_queue_itr;

    for (aws_queue_itr = aws_queue_data_.begin(); aws_queue_itr != aws_queue_data_.end(); aws_queue_itr++) {
      double computational_units = queue_to_jobs_map_[aws_queue_itr->first]->GetComputationalUnits(_aws_jobs_metadata_);
      computational_units_to_queue_map_[computational_units] = (aws_queue_itr->second);
    }

    return -1;
  }

  void LoadAWSQueueConfig() {
    std::ifstream config_file_stream_;
    config_file_stream_.open(AWS_QUEUE_CONFIG_FILE, std::ios::in);

    if (!config_file_stream_.is_open()) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::FILE_ISSUE,
                                 std::string("COULD NOT LOAD CONFIG FILE : ") + std::string(AWS_QUEUE_CONFIG_FILE));
      exit(1);
    }

#define CONFIG_MAX_LINE_SIZE 1024

    char buffer[CONFIG_MAX_LINE_SIZE];
    std::string line_read = "";

    HFSAT::AWS::AWSQueueConfigParser aws_config_parser;

    while (config_file_stream_.good()) {
      memset((void*)(buffer), 0, sizeof(CONFIG_MAX_LINE_SIZE));
      config_file_stream_.getline(buffer, CONFIG_MAX_LINE_SIZE);

      line_read = buffer;

      if (line_read.find("#") != std::string::npos) continue;

      aws_config_parser.ParseConfigLineAndGenerateQueueToken(buffer, line_read.length(), aws_queue_data_);
    }

    config_file_stream_.close();

#undef CONFIG_MAX_LINE_SIZE
  }

  void GenerateJobsData(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_metadata_) {
    LoadAllJobsAvailable();
    GenerateQueueComputationUnits(_aws_jobs_metadata_);
  }

  void ResetQueueRunningInfo() {
    std::map<std::string, HFSAT::AWS::AWSQueue*>::iterator aws_queue_itr;

    for (aws_queue_itr = aws_queue_data_.begin(); aws_queue_itr != aws_queue_data_.end(); aws_queue_itr++) {
      (aws_queue_itr->second)->total_running_tasks_for_queue_ = 0;
    }
  }

  void AnalyzeJobsAndUpdateQueueMetaData(std::map<uint64_t, HFSAT::AWS::AWSJob*>& _aws_jobs_data_) {
    ResetQueueRunningInfo();

    std::map<uint64_t, HFSAT::AWS::AWSJob*>::iterator aws_jobs_itr;

    for (aws_jobs_itr = _aws_jobs_data_.begin(); aws_jobs_itr != _aws_jobs_data_.end(); aws_jobs_itr++) {
      if (aws_queue_data_.find((aws_jobs_itr->second)->uniq_queue_name_) == aws_queue_data_.end()) {
        ReportAWSError(SCHED_ISSUE, std::string("JOB QUEUE NO LONGER AVAILABLE IN QUEUE CONFIG") +
                                        std::string((aws_jobs_itr->second)->uniq_queue_name_));
        continue;

      } else {
        if ((aws_jobs_itr->second)->job_status_ == HFSAT::AWS::PROCESSING) {
          aws_queue_data_[(aws_jobs_itr->second)->uniq_queue_name_]->total_running_tasks_for_queue_++;

          if ((aws_queue_data_[(aws_jobs_itr->second)->uniq_queue_name_]->total_running_tasks_for_queue_) >
              (aws_queue_data_[(aws_jobs_itr->second)->uniq_queue_name_]->limit_max_cores_usage_)) {
            ReportAWSError(SCHED_ISSUE,
                           std::string("MORE TASKS RUNNNIG ON A QUEUE THAN MAXLIMIT") +
                               std::string(aws_queue_data_[(aws_jobs_itr->second)->uniq_queue_name_]->ToString()),
                           aws_queue_data_[(aws_jobs_itr->second)->uniq_queue_name_]->queue_communication_);
          }
        }
      }
    }

    std::map<std::string, HFSAT::AWS::AWSQueue*>::iterator aws_queue_itr;

    for (aws_queue_itr = aws_queue_data_.begin(); aws_queue_itr != aws_queue_data_.end(); aws_queue_itr++) {
      if (priority_sorted_map_.find((aws_queue_itr->second)->queue_priority_) == priority_sorted_map_.end()) {
        std::vector<HFSAT::AWS::AWSQueue*>* new_priority_vec = new std::vector<HFSAT::AWS::AWSQueue*>();
        priority_sorted_map_[(aws_queue_itr->second)->queue_priority_] = new_priority_vec;
      }

      std::vector<HFSAT::AWS::AWSQueue*>* priority_vec = priority_sorted_map_[(aws_queue_itr->second)->queue_priority_];
      priority_vec->push_back(aws_queue_itr->second);
    }
  }

  double AllocateCoresToRunnableQueues(unsigned int _free_cores_,
                                       std::map<std::string, HFSAT::AWS::AWSQueue*>& _queue_metadata_) {
    std::map<double, HFSAT::AWS::AWSQueue*>::iterator cuqm_itr;

    for (cuqm_itr = computational_units_to_queue_map_.begin(); cuqm_itr != computational_units_to_queue_map_.end();
         cuqm_itr++) {
      if (_queue_metadata_.find((cuqm_itr->second)->uniq_queue_name_) != _queue_metadata_.end()) {
        if ((_queue_metadata_[(cuqm_itr->second)->uniq_queue_name_]->total_running_tasks_for_queue_) >=
            (cuqm_itr->second)->limit_max_cores_usage_)
          continue;
      }

      runnable_computational_units_to_queue_map_[cuqm_itr->first] = (cuqm_itr->second);
    }

    std::map<double, HFSAT::AWS::AWSQueue*>::iterator rcuqm_itr;
    unsigned int total_runnable_queues_counter = runnable_computational_units_to_queue_map_.size();
    unsigned int runnable_counter = 0;
    unsigned int free_left = _free_cores_;

    for (runnable_counter = 0, rcuqm_itr = runnable_computational_units_to_queue_map_.begin();
         rcuqm_itr != runnable_computational_units_to_queue_map_.end(); rcuqm_itr++, runnable_counter++) {
      int cores_allocated = 0;
      double queue_position_to_cores =
          ((runnable_computational_units_to_queue_map_.size() - runnable_counter) * _free_cores_) /
          ((double)total_runnable_queues_counter);

      if (queue_position_to_cores < 1.0)
        cores_allocated = 1;
      else
        cores_allocated = (unsigned int)(queue_position_to_cores);

      int total_running_jobs_for_this_queue = 0;
      int max_limit_for_this_queue = (rcuqm_itr->second)->limit_max_cores_usage_;

      if (_queue_metadata_.find((rcuqm_itr->second)->uniq_queue_name_) != _queue_metadata_.end()) {
        total_running_jobs_for_this_queue =
            (_queue_metadata_[(rcuqm_itr->second)->uniq_queue_name_])->total_running_tasks_for_queue_;
      }

      cores_allocated = ((max_limit_for_this_queue - total_running_jobs_for_this_queue) >= cores_allocated)
                            ? cores_allocated
                            : ((max_limit_for_this_queue - total_running_jobs_for_this_queue));

      free_left -= cores_allocated;

      queues_to_cores_allocation_map_[(rcuqm_itr->second)->uniq_queue_name_] = (unsigned int)(cores_allocated);
    }

    if ((_free_cores_ - free_left) > 0) {
      for (rcuqm_itr = runnable_computational_units_to_queue_map_.begin();
           rcuqm_itr != runnable_computational_units_to_queue_map_.end(); rcuqm_itr++) {
        int total_running_jobs_for_this_queue = 0;
        int max_limit_for_this_queue = (rcuqm_itr->second)->limit_max_cores_usage_;

        if (_queue_metadata_.find((rcuqm_itr->second)->uniq_queue_name_) != _queue_metadata_.end()) {
          total_running_jobs_for_this_queue =
              (_queue_metadata_[(rcuqm_itr->second)->uniq_queue_name_])->total_running_tasks_for_queue_;
        }

        int given_cores_on_previous_check = queues_to_cores_allocation_map_[(rcuqm_itr->second)->uniq_queue_name_];

        int room_for_more_core_allocation =
            (max_limit_for_this_queue - total_running_jobs_for_this_queue - given_cores_on_previous_check);

        room_for_more_core_allocation =
            (room_for_more_core_allocation > 0)
                ? (std::min(room_for_more_core_allocation, (int)((_free_cores_ - free_left))))
                : ((_free_cores_ - free_left));

        if (room_for_more_core_allocation < 0) continue;

        free_left -= (room_for_more_core_allocation);

        queues_to_cores_allocation_map_[(rcuqm_itr->second)->uniq_queue_name_] += (room_for_more_core_allocation);
      }
    }

    if ((_free_cores_ - free_left) > 0) {
      return -1;
      // TODO, Nothing More to Do Just Return, Send An Email
    }

    return -1;
  }

  std::map<double, HFSAT::AWS::AWSQueue*>& GetRunnableQueues(
      std::map<std::string, HFSAT::AWS::AWSQueue*>& _queue_metadata_) {
    return runnable_computational_units_to_queue_map_;
  }

  std::map<std::string, unsigned int>& GetRunnableQueuesToCoreAllocationData() {
    return queues_to_cores_allocation_map_;
  }

  std::map<std::string, HFSAT::AWS::AWSQueue*>& GetAllQueuesData() { return aws_queue_data_; }

  std::vector<HFSAT::AWS::AWSJob*> AssignJobsFromQueueForCores(std::string _queue_name_,
                                                               unsigned int _cores_alloted_to_this_queue_,
                                                               unsigned int& _cores_actually_allocated_) {
    if ((aws_queue_data_.find(_queue_name_) == aws_queue_data_.end()) ||
        (queue_to_jobs_map_.find(_queue_name_) == queue_to_jobs_map_.end())) {
      // TODO report error
    }

    HFSAT::AWS::AWSJobsManager* this_aws_jobs_manager = queue_to_jobs_map_[_queue_name_];

    std::vector<HFSAT::AWS::AWSJob*> jobs_allocated_vec =
        this_aws_jobs_manager->AllocateJobsWithFreeCores(_cores_alloted_to_this_queue_, _cores_actually_allocated_);

    return jobs_allocated_vec;
  }
};
}
}
