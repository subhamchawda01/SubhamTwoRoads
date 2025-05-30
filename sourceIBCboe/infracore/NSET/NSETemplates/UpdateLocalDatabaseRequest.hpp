// =====================================================================================
//
//       Filename:  UpdateLocalDatabaseRequest.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 07:58:06 PM
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

#include "infracore/NSET/NSETemplates/RequestPacket.hpp"
#include "infracore/NSET/NSETemplates/RequestHeader.hpp"

namespace HFSAT {
namespace NSE {

class NSEUpdateLocalDatabaseRequest {
 protected:
  char *update_local_database_request_buffer_;
  char const *msg_ptr;

  virtual void InitializeStaticFields() {}

 public:
  NSEUpdateLocalDatabaseRequest() {
    update_local_database_request_buffer_ = nullptr;
    msg_ptr = nullptr;
  }
  virtual ~NSEUpdateLocalDatabaseRequest() {}

  inline void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  inline void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  inline void SetPacketMD5CheckSum(char const *packet_md5sum_checksum) {
    memcpy((void *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET), packet_md5sum_checksum, NSE_PACKET_CHECKSUM_LENGTH);
  }

  // Main field setters

  inline void SetMessageHeaderTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }

  inline void SetMessageHeaderLogTime(int32_t const &message_header_logtime) {
    *((int32_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_LOGTIME_OFFSET)) = hton32(message_header_logtime);
  }

  inline void SetMessageHeaderAlphaChar(char const *message_header_alpha_char) {
    memcpy((void *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_OFFSET), message_header_alpha_char,
           NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_LENGTH);
  }

  inline void SetMessageHeaderTraderId(int32_t const &message_header_trader_id) {
    *((int32_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRADERID_OFFSET)) = hton32(message_header_trader_id);
  }

  inline void SetMessageHeaderErrorCode(int16_t const &message_header_error_code) {
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ERRORCODE_OFFSET)) = hton16(message_header_error_code);
  }

  virtual void SetStMarketStatus(st_market_status &st_mkt_status) {}

  virtual void SetStExMarketStatus(st_ex_market_status &st_ex_mkt_status) {}

  virtual void SetStPlMarketStatus(st_pl_market_status &st_pl_mkt_status) {}

 public:
  inline void SetPreLoadedUpdateLocalDatabaseRequestFields(int32_t const &user_id) {
    SetMessageHeaderTraderId(user_id);
  }

  virtual void SetDynamicUpdateLocalDatabaseRequestFields(int32_t const &packet_sequence_number,
                                                          st_market_status &st_mkt_status,
                                                          st_ex_market_status &st_ex_mkt_status,
                                                          st_pl_market_status &st_pl_mkt_status, int16_t call_auction_1,
                                                          int16_t call_auction_2) {}

  char const *GetUpdateLocalDatabaseRequestBuffer() const { return update_local_database_request_buffer_; }
  virtual int32_t GetUpdateLocalDatabaseRequestLength() { return 0; }
};
}
}
