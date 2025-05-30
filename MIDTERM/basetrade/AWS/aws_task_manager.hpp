// =====================================================================================
//
//       Filename:  aws_task_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/01/2014 06:43:40 PM
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
#include "basetrade/AWS/aws_server_info.hpp"
#include "basetrade/AWS/aws_process_status_metadata_handler.hpp"
#include "basetrade/AWS/aws_jobs_metadata_handler.hpp"
#include "basetrade/AWS/aws_queues_metadata_handler.hpp"
#include "basetrade/AWS/aws_worker_metadata_handler.hpp"
#include "basetrade/AWS/aws_queue_manager.hpp"
#include "basetrade/AWS/aws_jobs_manager.hpp"
#include "basetrade/AWS/aws_batch_run.hpp"

namespace HFSAT {
namespace AWS {

class AWSTaskManager {
 private:
  HFSAT::AWS::AWSServerInfo& aws_server_info_;
  HFSAT::AWS::AWSProcessMetaDataHandler& aws_process_metadata_loader_;
  HFSAT::AWS::AWSJobsMetadataHandler& aws_jobs_metadata_handler_;
  HFSAT::AWS::AWSWorkerMetaDataHandler& aws_worker_metadata_handler_;
  HFSAT::AWS::AWSQueueManager& aws_queue_manager_;
  std::map<std::string, HFSAT::AWS::AWSJobsManager*> queue_to_jobs_map_;
  std::vector<std::string> batch_job_list_;
  HFSAT::AWS::AWSServerMetaData* aws_server_data_;
  HFSAT::AWS::AWSBatchRun batch_run_;
  HFSAT::AWS::AWSQueuesMetadataHandler aws_queues_metadata_handler_;

 public:
  AWSTaskManager(HFSAT::AWS::AWSServerInfo& _aws_server_info_,
                 HFSAT::AWS::AWSProcessMetaDataHandler& _aws_process_metadata_loader_,
                 HFSAT::AWS::AWSJobsMetadataHandler& _aws_jobs_metadata_handler_,
                 HFSAT::AWS::AWSWorkerMetaDataHandler& _aws_worker_metadata_handler_,
                 HFSAT::AWS::AWSQueueManager& _aws_queue_manager_,
                 HFSAT::AWS::AWSQueuesMetadataHandler& _aws_queues_metadata_handler_)
      : aws_server_info_(_aws_server_info_),
        aws_process_metadata_loader_(_aws_process_metadata_loader_),
        aws_jobs_metadata_handler_(_aws_jobs_metadata_handler_),
        aws_worker_metadata_handler_(_aws_worker_metadata_handler_),
        aws_queue_manager_(_aws_queue_manager_),
        queue_to_jobs_map_(),
        aws_server_data_(aws_server_info_.GetServerMetaData()),
        batch_run_(),
        aws_queues_metadata_handler_(_aws_queues_metadata_handler_) {}

  bool CanScheduleMoreJobs() {
    if ((aws_server_data_->total_available_cores_) > (aws_server_data_->running_processes_)) return true;

    if ((aws_server_data_->total_available_cores_) < (aws_server_data_->running_processes_)) {
      HFSAT::AWS::ReportAWSError(SCHED_ISSUE,
                                 std::string("SCHEDULING ERROR : ") + std::string(aws_server_data_->ToString()));
    }

    return false;
  }

  bool CanScheduleMoreUrgentJobs() {
    if ((aws_server_data_->reserved_urgent_cores_) > (aws_server_data_->urgent_running_)) return true;

    if ((aws_server_data_->reserved_urgent_cores_) < (aws_server_data_->urgent_running_)) {
      HFSAT::AWS::ReportAWSError(SCHED_ISSUE,
                                 std::string("URGENT SCHEDULING ERROR") + std::string(aws_server_data_->ToString()));
    }

    return false;
  }

  //      void LoadAllJobsAvailable () {
  //
  //        std::map < std::string, HFSAT::AWS::AWSQueue * > & all_queues_data = aws_queue_manager_.GetAllQueuesData ()
  //        ;
  //        std::map < std::string, HFSAT::AWS::AWSQueue * > :: iterator all_queue_itr ;
  //
  //        for ( all_queue_itr = all_queues_data.begin () ; all_queue_itr != all_queues_data.end () ; all_queue_itr ++
  //        ) {
  //
  //          if ( queue_to_jobs_map_.find ( all_queue_itr -> first ) == queue_to_jobs_map_.end () ) {
  //
  //            HFSAT::AWS::AWSJobsManager * new_aws_jobs_manager = new HFSAT::AWS::AWSJobsManager ( all_queue_itr ->
  //            first ) ;
  //            queue_to_jobs_map_ [ all_queue_itr -> first ] = new_aws_jobs_manager ;
  //
  //          }
  //
  //        }
  //
  //      }
  //
  void AllocateJobToWorker(HFSAT::AWS::AWSJob* _aws_job_, HFSAT::AWS::AWSWorker* _aws_worker_) {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ssh -f -n -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no "
               << _aws_worker_->worker_unique_id_ << " \"sh -c 'nohup sh " << _aws_job_->exec_with_arguments_
               << " >/dev/null 2>/dev/null </dev/null & ' \" ";

    batch_job_list_.push_back(t_temp_oss.str());
    aws_jobs_metadata_handler_.AddJobMetaData(_aws_job_);
  }

  void AssignUrgentTasks() {
    ////        HFSAT::AWS::AWSQueue * next_aws_urgent_queue =
    /// aws_queue_manager_.GetNextBestAvailableUrgentQueueForProcessing () ;
    //
    //        if ( next_aws_urgent_queue == NULL ) {
    //
    //          ReportAWSError ( SCHED_INFO, std::string ( "NOTHING IN THE URGENT QUEUE" ) );
    //          return ;
    //
    //        }
    //
    //        std::string queue_name = ( next_aws_urgent_queue -> uniq_queue_name_ ) ;
    //
    //        if ( queue_to_jobs_map_.find ( queue_name ) == queue_to_jobs_map_.end () ) {
    //
    //          HFSAT::AWS::AWSJobsManager * new_aws_jobs_manager = new HFSAT::AWS::AWSJobsManager ( queue_name ) ;
    //          queue_to_jobs_map_ [ queue_name ] = new_aws_jobs_manager ;
    //
    //        }
    //
    //        HFSAT::AWS::AWSJob * next_aws_job = ( queue_to_jobs_map_ [ queue_name ] ) -> GetNextRunnableUrgentJob (
    //        aws_jobs_metadata_handler_.GetJobsMetaData () ) ;
    //        HFSAT::AWS::AWSWorker * next_free_worker = aws_server_info_.GetNextUrgentFreeWorker () ;
    //
    //        AllocateJobToWorker ( next_aws_job, next_free_worker ) ;
  }

  void AssignTasks() {
    ////        HFSAT::AWS::AWSQueue * next_aws_queue = aws_queue_manager_.GetNextBestAvailableQueueForProcessing (
    /// aws_queues_metadata_handler_.GetQueuesMetaData (), queue_to_jobs_map_ ) ;
    //
    //        std::string queue_name = ( next_aws_queue -> uniq_queue_name_ ) ;
    //
    //        if ( queue_to_jobs_map_.find ( queue_name ) == queue_to_jobs_map_.end () ) {
    //
    //          HFSAT::AWS::AWSJobsManager * new_aws_jobs_manager = new HFSAT::AWS::AWSJobsManager ( queue_name ) ;
    //          queue_to_jobs_map_ [ queue_name ] = new_aws_jobs_manager ;
    //
    //        }
    //
    ////        HFSAT::AWS::AWSJob * next_aws_job = ( queue_to_jobs_map_ [ queue_name ] ) -> GetNextRunnableJob () ;
    //        HFSAT::AWS::AWSWorker * next_free_worker = aws_server_info_.GetNextFreeWorker () ;
    //
    ////        AllocateJobToWorker ( next_aws_job, next_free_worker ) ;
  }

  void CleanUp() {}

  void UpdateSystemAndCleanUp() {
    aws_jobs_metadata_handler_.UpdateSystem();
    aws_process_metadata_loader_.UpdateSystem();
    batch_run_.CommitBatchJobs(batch_job_list_);

    CleanUp();
  }
};
}
}
