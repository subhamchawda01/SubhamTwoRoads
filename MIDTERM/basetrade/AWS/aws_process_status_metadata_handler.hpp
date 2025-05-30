// =====================================================================================
//
//       Filename:  aws_process_status_metadata_handler.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/21/2014 10:37:08 AM
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

#include "basetrade/AWS/aws_defines.hpp"
#include "basetrade/AWS/aws_process_status_parser.hpp"

namespace HFSAT {
namespace AWS {

class AWSProcessMetaDataHandler {
 private:
  std::map<uint64_t, HFSAT::AWS::ProcessMetaData*> aws_process_metadata_;
  HFSAT::AWS::AWSProcessStatusParser aws_process_status_parser_;

 public:
  AWSProcessMetaDataHandler()
      : aws_process_metadata_(),
        aws_process_status_parser_()

  {}

  void LoadProcessMetaDataUpdatedBySystem() {
    std::ifstream aws_process_metadata_filestream;

    aws_process_metadata_filestream.open(PROCESS_METADATA_SYSTEM_FILE, std::ios::in);

    if (!aws_process_metadata_filestream.is_open()) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::FILE_INFO,
                                 std::string("Could Not Load Process MetaData File : ") + PROCESS_METADATA_SYSTEM_FILE);
      aws_process_metadata_.clear();
      return;
    }

#define MAX_LINE_SIZE 4096

    char buffer[MAX_LINE_SIZE];
    std::string line_buffer = "";

    memset((void*)(buffer), 0, MAX_LINE_SIZE);

    HFSAT::AWS::AWSProcessStatusParser aws_process_status_parser;

    while (aws_process_metadata_filestream.good()) {
      aws_process_metadata_filestream.getline(buffer, MAX_LINE_SIZE);
      line_buffer = buffer;

      HFSAT::PerishableStringTokenizer st(buffer, line_buffer.length());
      const std::vector<const char*>& tokens = st.GetTokens();

      if (tokens.size() < 5) continue;

      aws_process_status_parser.ParseProcessMetaDataAndGenerateTokens(tokens, aws_process_metadata_);
    }

    aws_process_metadata_filestream.close();

#undef MAX_LINE_SIZE
  }

  void AddProcessMetaDataFromJob(HFSAT::AWS::AWSJob* _aws_job_) {
    HFSAT::AWS::ProcessMetaData* new_process = new HFSAT::AWS::ProcessMetaData();

    new_process->metadata_update_time_ = _aws_job_->unique_job_id_;
    new_process->process_status_ = HFSAT::AWS::UNKNOWN;
    new_process->worker_machine_ = "CONTROLLER";
    new_process->job_exec_with_arg_ = _aws_job_->exec_with_arguments_;

    aws_process_metadata_[new_process->metadata_update_time_] = new_process;
  }

  void UpdateSystem() {
    std::ofstream aws_process_metadata_filestream;

    aws_process_metadata_filestream.open(PROCESS_METADATA_SYSTEM_FILE, std::ios::out);

    if (!aws_process_metadata_filestream.is_open()) {
      ReportAWSError(FILE_ISSUE, std::string("COULD NOT LOAD PROCESS METADATA FILE : ") + PROCESS_METADATA_SYSTEM_FILE);
      return;
    }

    for (unsigned int process_metadata_counter_ = 0; process_metadata_counter_ < aws_process_metadata_.size();
         process_metadata_counter_++) {
      if (aws_process_metadata_[process_metadata_counter_]->process_status_ != NOT_RUNNING) {
        aws_process_metadata_filestream << aws_process_metadata_[process_metadata_counter_]->ToString();
      }
    }

    aws_process_metadata_filestream.close();
  }

  std::map<uint64_t, HFSAT::AWS::ProcessMetaData*>& GetProcessMetaData() { return aws_process_metadata_; }
};
}
}
