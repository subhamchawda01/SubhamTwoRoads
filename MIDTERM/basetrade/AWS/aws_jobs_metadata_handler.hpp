// =====================================================================================
//
//       Filename:  aws_jobs_metadata.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/01/2014 05:34:41 PM
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

class AWSJobsMetadataHandler {
 private:
  std::map<uint64_t, HFSAT::AWS::AWSJob*> aws_jobs_metadata_;

 public:
  void LoadAWSJobsMetaData() {
    HFSAT::BulkFileReader* bulk_file_reader = new HFSAT::BulkFileReader();
    bulk_file_reader->open(AWS_JOBS_METADATA_BIN_FILE);

    if (!bulk_file_reader->is_open()) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::FILE_ISSUE,
                                 std::string("COULD NOT LOAD JOBS METADATA FILE : ") + AWS_JOBS_METADATA_BIN_FILE);
      aws_jobs_metadata_.clear();
      return;
    }

    while (true) {
      HFSAT::AWS::AWSJob* aws_job = new HFSAT::AWS::AWSJob();
      memset((void*)(aws_job), 0, sizeof(HFSAT::AWS::AWSJob));

      size_t read_length = bulk_file_reader->read(aws_job, sizeof(HFSAT::AWS::AWSJob));
      if (read_length < sizeof(HFSAT::AWS::AWSJob)) break;

      aws_jobs_metadata_[aws_job->unique_job_id_] = aws_job;
    }

    if (bulk_file_reader) {
      bulk_file_reader->close();
      delete bulk_file_reader;
      bulk_file_reader = NULL;
    }
  }

  void AnalyzeProcessDataAndUpdateJobsMetaData(std::map<uint64_t, HFSAT::AWS::ProcessMetaData*>& _process_metadata_) {
    // Nothing available in the ProcessMetadata
    if (_process_metadata_.size() == 0) return;

    std::map<uint64_t, HFSAT::AWS::ProcessMetaData*>::iterator pmc_itr;

    for (pmc_itr = _process_metadata_.begin(); pmc_itr != _process_metadata_.end(); pmc_itr++) {
      if (aws_jobs_metadata_.find(pmc_itr->first) != aws_jobs_metadata_.end()) {
        switch ((pmc_itr->second)->process_status_) {
          case HFSAT::AWS::RUNNING:
          case HFSAT::AWS::MULTIPLE_RUNNING: {
            aws_jobs_metadata_[pmc_itr->first]->job_status_ = HFSAT::AWS::PROCESSING;

          } break;

          case HFSAT::AWS::NOT_RUNNING: {
            aws_jobs_metadata_[pmc_itr->first]->job_status_ = HFSAT::AWS::DONE;

          } break;

          default: {
            aws_jobs_metadata_[pmc_itr->first]->job_status_ = HFSAT::AWS::FAILED;
            HFSAT::AWS::ReportAWSError(HFSAT::AWS::SCHED_ISSUE,
                                       std::string("JOB PROCESSING FAILED FOR PROCESS : ") +
                                           std::string(aws_jobs_metadata_[pmc_itr->first]->exec_with_arguments_));

          } break;
        }

      } else {
        HFSAT::AWS::ReportAWSError(HFSAT::AWS::SCHED_ISSUE,
                                   std::string("COULD NOT FIND ANY JOBS METADATA FOR PROCESS : ") +
                                       std::string((pmc_itr->second)->job_exec_with_arg_));
      }
    }
  }

  void AddJobMetaData(HFSAT::AWS::AWSJob* _aws_job_) {
    if (aws_jobs_metadata_.find(_aws_job_->unique_job_id_) == aws_jobs_metadata_.end()) {
      aws_jobs_metadata_[_aws_job_->unique_job_id_] = _aws_job_;

    } else {
      ReportAWSError(SCHED_ISSUE, std::string("Duplicate JOB ID"));
    }
  }

  std::map<uint64_t, HFSAT::AWS::AWSJob*>& GetJobsMetaData() { return aws_jobs_metadata_; }

  void UpdateSystem() {
    HFSAT::BulkFileWriter* bulk_file_writer = NULL;
    bulk_file_writer->Open(AWS_JOBS_METADATA_BIN_FILE, std::ios::out);

    if (!bulk_file_writer->is_open()) {
      ReportAWSError(FILE_ISSUE, std::string("FAILED TO OPEN JOBS METADATA FILE FOR WRITING : ") +
                                     std::string(AWS_JOBS_METADATA_BIN_FILE));
      exit(1);
    }

    std::map<uint64_t, HFSAT::AWS::AWSJob*>::iterator aws_jobs_itr;

    for (aws_jobs_itr = aws_jobs_metadata_.begin(); aws_jobs_itr != aws_jobs_metadata_.end(); aws_jobs_itr++) {
      bulk_file_writer->Write((void*)(aws_jobs_itr->second), sizeof(HFSAT::AWS::AWSJob));
      bulk_file_writer->CheckToFlushBuffer();
    }

    if (bulk_file_writer) {
      bulk_file_writer->Close();
      delete bulk_file_writer;
      bulk_file_writer = NULL;
    }
  }
};
}
}
