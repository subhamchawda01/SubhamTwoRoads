// =====================================================================================
//
//       Filename:  aws_worker_config_parser.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/11/2014 06:45:48 PM
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
#include <fstream>
#include "basetrade/AWS/aws_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
namespace AWS {

class AWSWorkerConfigParser {
 public:
  void ParseWorkerConfigLineAndGenerateWorkerTokens(
      char* _worker_config_line_, const int32_t& _line_length_,
      std::map<std::string, HFSAT::AWS::AWSWorker*>& _worker_config_vec_) {
    HFSAT::PerishableStringTokenizer st(_worker_config_line_, _line_length_);
    const std::vector<const char*>& tokens = st.GetTokens();

    if (tokens.size() < 3) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::CONFIG_ISSUE, std::string("COULD NOT PARSE WORKER CONFIG LINE : ") +
                                                               std::string(_worker_config_line_));
      return;
    }

    HFSAT::AWS::AWSWorker* new_worker = new HFSAT::AWS::AWSWorker();
    memset((void*)(new_worker), 0, sizeof(HFSAT::AWS::AWSWorker));

    new_worker->worker_unique_id_ = tokens[0];
    new_worker->total_cores_available_ = atoi(tokens[1]);
    new_worker->total_running_jobs_ = atoi(tokens[2]);

    if (new_worker->total_running_jobs_ > new_worker->total_cores_available_) {
      ReportAWSError(SCHED_ISSUE,
                     std::string("MORE JOBS RUNNING THAN TOTAL CORES") + std::string(new_worker->ToString()));
      return;
    }

    _worker_config_vec_[new_worker->worker_unique_id_] = new_worker;
  }
};
}
}
