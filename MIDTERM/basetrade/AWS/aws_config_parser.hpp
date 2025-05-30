// =====================================================================================
//
//       Filename:  aws_config_parser.hpp
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
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
namespace AWS {

class AWSQueueConfigParser {
 public:
  void ParseConfigLineAndGenerateQueueToken(char* _config_line_, const int32_t& _length_,
                                            std::map<std::string, AWSQueue*>& _queue_config_data_) {
    HFSAT::PerishableStringTokenizer st(_config_line_, _length_);
    const std::vector<const char*>& tokens = st.GetTokens();

    if (tokens.size() < 2) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::CONFIG_ISSUE,
                                 std::string("COULD NOT PARSE CONFIG LINE : ") + std::string(_config_line_));
      return;
    }

    HFSAT::AWS::AWSQueue* new_queue = new HFSAT::AWS::AWSQueue();
    memset((void*)(new_queue), 0, sizeof(HFSAT::AWS::AWSQueue));
    new_queue->queue_priority_ = DEFAULT_QUEUE_PRIORITY;

    if (tokens.size() > 2) {
      if (tokens.size() == 3) {
        new_queue->queue_priority_ = atoi(tokens[2]);
      } else if (tokens.size() == 4) {
        new_queue->run_once_ = std::string(tokens[3]) == "LINEAR" ? true : false;
      } else if (tokens.size() == 5) {
        new_queue->queue_owner_ = tokens[4];
      } else if (tokens.size() == 6) {
        new_queue->queue_communication_ = tokens[5];
      }
    }

    new_queue->uniq_queue_name_ = tokens[0];
    new_queue->limit_max_cores_usage_ = atoi(tokens[1]);

    if (new_queue->limit_max_cores_usage_ < 0 || new_queue->limit_max_cores_usage_ > MAX_CORES) {
      HFSAT::AWS::ReportAWSError(HFSAT::AWS::CONFIG_ISSUE,
                                 std::string("BAD VALUE SPECIFIED FOR CONFIG : ") + std::string(tokens[1]),
                                 new_queue->queue_communication_);
      delete new_queue;
      return;
    }

    _queue_config_data_[new_queue->uniq_queue_name_] = new_queue;
  }
};
}
}
