// =====================================================================================
//
//       Filename:  aws_batch_run.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/02/2014 10:54:25 AM
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

namespace HFSAT {
namespace AWS {

class AWSBatchRun {
 public:
  void CommitBatchJobs(std::vector<std::string>& aws_batch_run_) {
    std::ofstream aws_batch_run_filestream;

    aws_batch_run_filestream.open(AWS_BATCH_RUN, std::ios::out);

    if (!aws_batch_run_filestream.is_open()) {
      ReportAWSError(FILE_ISSUE, std::string("COULD NOT AWS BATCH FILE FOR WRITING : ") + AWS_BATCH_RUN);
      return;
    }

    for (unsigned int aws_batch_run_counter_ = 0; aws_batch_run_counter_ < aws_batch_run_.size();
         aws_batch_run_counter_++) {
      aws_batch_run_filestream << aws_batch_run_[aws_batch_run_counter_] << "\n";
    }

    aws_batch_run_filestream.close();
  }
};
}
}
