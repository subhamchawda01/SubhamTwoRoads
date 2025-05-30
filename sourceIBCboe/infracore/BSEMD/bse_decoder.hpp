/**
    \file EobiD/eobi_decoder.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551

 */

#pragma once

#include <iostream>

#define EOBI_MSG_HEADER_SIZE 8
#define IS_LOGGING_ENABLE 0
#define ORDER_ADD 13100
#define ORDER_MODIFY 13101
#define ORDER_MODIFY_SAME_PRIORITY 13106
#define ORDER_DELETE 13102
#define ORDER_MASS_DELETE 13103
#define PARTIAL_EXECUTION 13105
#define FULL_EXECUTION 13104
#define EXECUTION_SUMMARY 13202
#define PRODUCT_SUMMARY 13600
#define SNAPSHOT_ORDER 13602
#define INSTRUMENT_SUMMARY 13601

namespace HFSAT {

// EOBI is binary encoded stream. Therefore, we directly obtain various parameters from the stream itself using offsets
// To verify the correctness of offsets etc, please refer to the EOBI document provided by EUREX
class EobiDecoder {
 private:
  uint16_t body_len_;
  uint16_t template_id_;

 public:
  EobiDecoder();

  void Decode(char* bytes, uint32_t len);

  void DecodeTemplate(char* bytes);
};
}
