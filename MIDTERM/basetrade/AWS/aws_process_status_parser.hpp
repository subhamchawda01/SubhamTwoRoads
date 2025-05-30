// =====================================================================================
//
//       Filename:  aws_process_status_parser.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/01/2014 04:12:28 PM
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
#include <cstdlib>
#include <string>
#include "basetrade/AWS/aws_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
namespace AWS {

class AWSProcessStatusParser {
 public:
  void ParseProcessMetaDataAndGenerateTokens(const std::vector<const char*>& _tokens_,
                                             std::map<uint64_t, HFSAT::AWS::ProcessMetaData*> _aws_process_metadata_) {
    HFSAT::AWS::ProcessMetaData* process_metadata = new HFSAT::AWS::ProcessMetaData();

    process_metadata->metadata_update_time_ = strtoul(_tokens_[0], NULL, 0);
    process_metadata->process_status_ = HFSAT::AWS::GetProcessStatusFromString(_tokens_[1]);
    process_metadata->separator_ = _tokens_[2];

    std::string process_exec_with_args = "";

    for (unsigned int token_counter = 3; token_counter < _tokens_.size(); token_counter++) {
      process_exec_with_args += std::string(_tokens_[token_counter]);
    }

    process_metadata->job_exec_with_arg_ = process_exec_with_args;

    _aws_process_metadata_[process_metadata->metadata_update_time_] = process_metadata;
  }
};
}
}
