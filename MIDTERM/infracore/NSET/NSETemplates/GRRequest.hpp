// =====================================================================================
//
//       Filename:  GRRequest.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 05:47:46 AM
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

#define NSE_GR_REQUEST_LENGTH 70 //22 + 40 + 8

namespace HFSAT {
namespace NSE {

/*
 * Generic  interface for nse gr_request request trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class NSEGRRequest {
 protected:
  char *gr_request_buffer_;
  const char *msg_ptr;

 public:
  void InitializeStaticFields() {}

  NSEGRRequest() {
    gr_request_buffer_ = (char *)calloc(NSE_GR_REQUEST_LENGTH, sizeof(char));
    memset((void*)gr_request_buffer_,0,sizeof(NSE_GR_REQUEST_LENGTH));
    msg_ptr = (gr_request_buffer_ + NSE_REQUEST_START_OFFSET);
  }
  ~NSEGRRequest() {}

  void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  void SetPacketMD5CheckSum(char const *packet_md5sum_checksum) {
    memcpy((void *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET), packet_md5sum_checksum, NSE_PACKET_CHECKSUM_LENGTH);
  }

  void SetMessageHeaderTransactionCode(int16_t const &message_header_transaction_code) {
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }

  void SetMessageHeaderLogTime(int32_t const &message_header_logtime) {
    *((int32_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_LOGTIME_OFFSET)) = hton32(message_header_logtime);
  }

  void SetMessageHeaderAlphaChar(char const *message_header_alpha_char) {
    memcpy((void *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_OFFSET), message_header_alpha_char,
           NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_LENGTH);
  }

  void SetMessageHeaderTraderId(int32_t const &message_header_trader_id) {
    *((int32_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRADERID_OFFSET)) = hton32(message_header_trader_id);
  }

  void SetMessageHeaderErrorCode(int16_t const &message_header_error_code) {
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ERRORCODE_OFFSET)) = hton16(message_header_error_code);
  }

  void SetMessageHeaderTimeStamp(char const *message_header_timestamp) {}

  void SetMessageHeaderTimeStamp1(char const *message_header_timestamp1) {}

  void SetMessageHeaderTimeStamp2(char const *message_header_timestamp2) {}

  void SetMessageHeaderLength(int16_t const *message_header_length) {}

  void SetGRRequestTransactionCode(int16_t const *gr_request_transaction_code) {}

 public:
  inline void SetPreLoadedGRRequestsFields(int32_t const &user_id, std::string broker_id, int16_t const &box_id){
    SetPacketLength(NSE_GR_REQUEST_LENGTH);
    SetMessageHeaderTransactionCode(GR_REQUEST_IN);
    SetMessageHeaderLogTime(0);
    SetMessageHeaderAlphaChar("  ");
    SetMessageHeaderErrorCode(0);
    SetMessageHeaderTraderId(user_id);

    // All TimeStamp Fields Should Be Set To Numeric 0 While Sending To Exch
    memset((void *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_OFFSET), 0,
           NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_LENGTH + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP1_LENGTH +
               NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP2_LENGTH);

    // Length of the message header and the following login request, hence packet length has been deducted
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_MESSAGELENGTH_OFFSET)) =
        hton16(NSE_GR_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    *((int16_t *)(msg_ptr+62)) = hton16(box_id);
    memcpy((void *)(msg_ptr+64), broker_id.c_str(), 5);

  }

  void SetDynamicGRRequestsFieldsAndUpdateChecksum(int32_t const &packet_sequence_number) {
    SetPacketSequenceNumber(packet_sequence_number);

    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_GR_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));

  }

  char const *GetGRRequestBuffer() const { return gr_request_buffer_; }
  int32_t GetGRRequestMsgLength() { return NSE_GR_REQUEST_LENGTH ; }
};
}
}
