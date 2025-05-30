// =====================================================================================
//
//       Filename:  aws_jobs_parser.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/11/2014 07:22:30 PM
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

namespace HFSAT {
namespace AWS {

class AWSJobParser {
 public:
  void ParseJobLineAndGenerateJobToken(char* _job_line_, const int& _line_length_,
                                       std::vector<AWSJob*>& _job_config_vec_) {
    HFSAT::PerishableStringTokenizer st(_job_line_, _line_length_);
    const std::vector<const char*>& tokens = st.GetTokens();

    if (tokens.size() < 8) {
      ReportAWSError(JOB_ISSUE, std::string("COULD NOT PARSE JOB LINE : ") + std::string(_job_line_));
      return;
    }

    HFSAT::AWS::AWSJob* new_job_ = new HFSAT::AWS::AWSJob();
    memset((void*)(new_job_), 0, sizeof(HFSAT::AWS::AWSJob));

    new_job_->uniq_queue_name_ = tokens[0];

    if (std::string(tokens[1]) == "URGENT") {
      new_job_->job_type_ = URGENT;

      //        NotifyAWSInfo ( ) ;

    } else if (std::string(tokens[1]) == "SHORTRUN") {
      new_job_->job_type_ = SHORTRUN;

    } else if (std::string(tokens[1]) == "LONGRUN") {
      new_job_->job_type_ = LONGRUN;

    } else {  // user chose to be lazy, will get least priority

      new_job_->job_type_ = DEFAULT;
    }

    // Now Priority

    if (std::string(tokens[2]) == "HIGH") {
      new_job_->job_priority_ = HIGH;

    } else if (std::string(tokens[2]) == "MEDIUM") {
      new_job_->job_priority_ = MEDIUM;

    } else if (std::string(tokens[2]) == "LOW") {
      new_job_->job_priority_ = LOW;

    } else {
      new_job_->job_priority_ = UNDEFINED;
    }

    new_job_->expected_run_duration_hours_ = atoi(tokens[3]);
    new_job_->job_status_ = QUEUED;

    if (std::string(tokens[4]) == "*") {
      new_job_->run_all_hours_ = true;

    } else {
      // TODO Add handling for hours
      new_job_->run_all_hours_ = false;
    }

    if (std::string(tokens[5]) == "*") {
      new_job_->run_all_days_ = true;

    } else {
      // TODO Add handling for days
      new_job_->run_all_days_ = false;
    }

    if (std::string(tokens[6]) == "RUN_ONCE") {
      new_job_->run_once_ = true;

    } else {
      new_job_->run_once_ = false;
    }

    new_job_->unique_job_id_ = strtoul(tokens[7], NULL, 0);

    // New Job
    if (new_job_->unique_job_id_ == 0) {
    }

    std::string exec_with_args = "";

    for (unsigned int counter = 8; counter < tokens.size(); counter++) {
      exec_with_args += std::string(tokens[counter]);
    }

    new_job_->exec_with_arguments_ = exec_with_args;

    _job_config_vec_.push_back(new_job_);
  }
};
}
}
