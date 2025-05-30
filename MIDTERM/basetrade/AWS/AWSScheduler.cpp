// =====================================================================================
//
//       Filename:  AWS_Scheduler.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/21/2014 11:19:29 AM
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

#include <iostream>
#include <cstdlib>

#include "basetrade/AWS/aws_defines.hpp"
#include "basetrade/AWS/aws_server_info.hpp"
#include "basetrade/AWS/aws_process_status_metadata_handler.hpp"
#include "basetrade/AWS/aws_jobs_metadata_handler.hpp"
#include "basetrade/AWS/aws_queues_metadata_handler.hpp"
#include "basetrade/AWS/aws_worker_metadata_handler.hpp"
#include "basetrade/AWS/aws_queue_manager.hpp"
#include "basetrade/AWS/aws_task_manager.hpp"

int main(int argc, char *argv[]) {
  // Load What's running data
  HFSAT::AWS::AWSServerInfo aws_server_info;
  HFSAT::AWS::AWSProcessMetaDataHandler aws_process_metadata_loader;
  HFSAT::AWS::AWSJobsMetadataHandler aws_jobs_metadata_handler;
  HFSAT::AWS::AWSQueuesMetadataHandler aws_queues_metadata_handler;
  HFSAT::AWS::AWSWorkerMetaDataHandler aws_worker_metadata_handler;
  HFSAT::AWS::AWSQueueManager aws_queue_manager;
  HFSAT::AWS::AWSTaskManager aws_task_manager(aws_server_info, aws_process_metadata_loader, aws_jobs_metadata_handler,
                                              aws_worker_metadata_handler, aws_queue_manager,
                                              aws_queues_metadata_handler);

  aws_process_metadata_loader.LoadProcessMetaDataUpdatedBySystem();
  aws_jobs_metadata_handler.LoadAWSJobsMetaData();
  aws_worker_metadata_handler.LoadWorkerData();
  aws_queues_metadata_handler.LoadAWSQueuesMetaData();
  aws_jobs_metadata_handler.AnalyzeProcessDataAndUpdateJobsMetaData(aws_process_metadata_loader.GetProcessMetaData());
  aws_worker_metadata_handler.AnalyzeJobsAndUpdateWorkerData(aws_jobs_metadata_handler.GetJobsMetaData());
  aws_server_info.AnalyzeWorkerInfoAndGenerateServerData(aws_worker_metadata_handler.GetWorkerMetaData());
  aws_queue_manager.LoadAWSQueueConfig();
  aws_queue_manager.AnalyzeQueueAllocationAndPenalizeWithServerData(aws_server_info.GetServerMetaData());
  aws_queue_manager.AnalyzeJobsAndUpdateQueueMetaData(aws_jobs_metadata_handler.GetJobsMetaData());

  while (aws_task_manager.CanScheduleMoreJobs()) {
    aws_task_manager.AssignTasks();
  }

  while (aws_task_manager.CanScheduleMoreUrgentJobs()) {
    aws_task_manager.AssignUrgentTasks();
  }

  aws_task_manager.UpdateSystemAndCleanUp();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
