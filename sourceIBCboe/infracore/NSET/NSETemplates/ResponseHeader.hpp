// =====================================================================================
//
//       Filename:  ResponseHeader.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 07:31:02 AM
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

#include "infracore/NSET/NSETemplates/DataDefines.hpp"

//===================================  NSE PACKET RESPONSE START ==================================================//

#define NSE_RESPONSE_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET (NSE_RESPONSE_START_OFFSET)
#define NSE_RESPONSE_MESSAGE_HEADER_TRANSACTION_CODE_LENGTH sizeof(int16_t)

#define NSE_RESPONSE_MESSAGE_HEADER_LOGTIME_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_TRANSACTION_CODE_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_LOGTIME_LENGTH sizeof(int32_t)

#define NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_LOGTIME_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_LOGTIME_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_LENGTH LENGTH_OF_MESSAGE_HEADER_ALPHACHAR_CHAR_FIELD

#define NSE_RESPONSE_MESSAGE_HEADER_TRADERID_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_TRADERID_LENGTH sizeof(int32_t)

#define NSE_RESPONSE_MESSAGE_HEADER_ERRORCODE_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_TRADERID_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_TRADERID_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_ERRORCODE_LENGTH sizeof(int16_t)

#define NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_ERRORCODE_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_ERRORCODE_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP_LENGTH LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD

#define NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP1_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP1_LENGTH LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD

#define NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP2_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP1_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP1_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP2_LENGTH LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD

#define NSE_RESPONSE_MESSAGE_HEADER_MESSAGELENGTH_OFFSET \
  (NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP2_OFFSET + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP2_LENGTH)
#define NSE_RESPONSE_MESSAGE_HEADER_MESSAGELENGTH_LENGTH sizeof(int16_t)

#define NSE_RESPONSE_MESSAGE_HEADER_LENGTH                                                            \
  (NSE_RESPONSE_MESSAGE_HEADER_TRANSACTION_CODE_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_LOGTIME_LENGTH + \
   NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_TRADERID_LENGTH +       \
   NSE_RESPONSE_MESSAGE_HEADER_ERRORCODE_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP_LENGTH +      \
   NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP1_LENGTH + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP2_LENGTH +    \
   NSE_RESPONSE_MESSAGE_HEADER_MESSAGELENGTH_LENGTH)

namespace HFSAT {
namespace NSE {

class ResponseHeader {
 private:
  ProcessedResponseHeader processed_response_header_;

 public:
  ResponseHeader() { memset((void *)&processed_response_header_, 0, sizeof(ProcessedResponseHeader)); }

  inline ProcessedResponseHeader *ProcessHeader(char const *msg_ptr) {
    processed_response_header_.transaction_code =
        ntoh16(*((int16_t *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET)));

    processed_response_header_.logtime = ntoh32(*((int32_t *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_LOGTIME_OFFSET)));

    memcpy((void *)processed_response_header_.alphachar,
           (void *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_OFFSET),
           NSE_RESPONSE_MESSAGE_HEADER_ALPHACHAR_LENGTH);

    processed_response_header_.trader_id =
        ntoh32(*((int32_t *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_TRADERID_OFFSET)));

    processed_response_header_.error_code =
        ntoh16(*((int16_t *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_ERRORCODE_OFFSET)));

    memcpy((void *)processed_response_header_.timestamp,
           (void *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP_OFFSET),
           LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD);

    memcpy((void *)processed_response_header_.timestamp1,
           (void *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP1_OFFSET),
           LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD);

    memcpy((void *)processed_response_header_.timestamp2,
           (void *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_TIMESTAMP2_OFFSET),
           LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD);

    processed_response_header_.message_length =
        ntoh16(*((int16_t *)(msg_ptr + NSE_RESPONSE_MESSAGE_HEADER_MESSAGELENGTH_OFFSET)));

    return &processed_response_header_;
  }
};
}
}
