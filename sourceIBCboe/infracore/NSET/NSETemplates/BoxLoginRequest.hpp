// =====================================================================================
//
//       Filename:  BoxLoginRequest.hpp
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

#define NSE_BoxLogin_REQUEST_LENGTH 82  // 22 + 40 + 8

namespace HFSAT {
namespace NSE {

/*
 * Generic  interface for nse boxlogin_request request trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class NSEBoxLoginRequest {
 protected:
  char *boxlogin_request_buffer_;
  const char *msg_ptr;

 public:
  void InitializeStaticFields() {}

  NSEBoxLoginRequest() {
    boxlogin_request_buffer_ = (char *)calloc(NSE_BoxLogin_REQUEST_LENGTH, sizeof(char));
    memset((void *)boxlogin_request_buffer_, 0, sizeof(NSE_BoxLogin_REQUEST_LENGTH));
    msg_ptr = (boxlogin_request_buffer_ + NSE_REQUEST_START_OFFSET);
  }
  ~NSEBoxLoginRequest() {}

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

  void SetBoxLoginRequestTransactionCode(int16_t const *boxlogin_request_transaction_code) {}

  void SetBoxLoginRequestSessionKey(char const *session_key) {
    std::cout << "SENDING KEY : " << session_key << std::endl;
    memcpy((void *)(msg_ptr + 74), session_key, 8);
  }

 public:
  inline void SetPreLoadedBoxLoginRequestsFields(int32_t const &user_id, std::string broker_id, int16_t const &box_id) {
    SetPacketLength(NSE_BoxLogin_REQUEST_LENGTH);
    SetMessageHeaderTransactionCode(BoxLogin_REQUEST_IN);
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
        hton16(NSE_BoxLogin_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    *((int16_t *)(msg_ptr + 62)) = hton16(box_id);

    memcpy((void *)(msg_ptr + 64), broker_id.c_str(), 5);
  }

  void SetDynamicBoxLoginRequestsFieldsAndUpdateChecksum(int32_t const &packet_sequence_number,
                                                         char const *session_key) {
    SetPacketSequenceNumber(packet_sequence_number);
    SetBoxLoginRequestSessionKey(session_key);

    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_BoxLogin_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_BoxLogin_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  char const *GetBoxLoginRequestBuffer() const { return boxlogin_request_buffer_; }
  int32_t GetBoxLoginRequestMsgLength() { return NSE_BoxLogin_REQUEST_LENGTH; }
};
}
}
