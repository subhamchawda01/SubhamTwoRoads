// =====================================================================================
//
//       Filename:  LogoffRequest.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 10:41:26 PM
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

#define NSE_LOGOFF_REQUEST_LENGTH (NSE_PACKET_REQUEST_LENGTH + NSE_REQUEST_MESSAGE_HEADER_LENGTH)

namespace HFSAT {
namespace NSE {

class LogoffRequest {
 private:
  char logoff_request_buffer[NSE_LOGOFF_REQUEST_LENGTH];
  char const *msg_ptr;

 private:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_LOGOFF_REQUEST_LENGTH);

    // Reserved Sequence is always filled with 0
    //    SetPacketReservedSequenceNumber(0);

    // Message Count is 1 as we are only sending login message in this packet
    //    SetPacketMessageCount(1);

    //================================= @ Message Header

    // Login Transaction Code
    SetMessageHeaderTransactionCode(SIGN_OFF_REQUEST_IN);

    // LogTime Shoudl Be Set To 0 While Sending To Exch
    SetMessageHeaderLogTime(0);

    // Alphachar should be set to blank ( ' ' ) while sending to host
    SetMessageHeaderAlphaChar("  ");

    // ErrorCode Should be 0 While Sending To Exch
    SetMessageHeaderErrorCode(0);

    // All TimeStamp Fields Should Be Set To Numeric 0 While Sending To Exch
    memset((void *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_OFFSET), 0,
           NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_LENGTH + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP1_LENGTH +
               NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP2_LENGTH);

    // Length of the message header and the following login request, hence packet length has been deducted
    *((int16_t *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_MESSAGELENGTH_OFFSET)) =
        hton16(NSE_LOGOFF_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);
  }

 public:
  LogoffRequest() : msg_ptr(logoff_request_buffer + NSE_REQUEST_START_OFFSET) {
    // Initialize Logoff Request Buffer
    memset((void *)logoff_request_buffer, 0, NSE_LOGOFF_REQUEST_LENGTH);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

 private:
  inline void SetPacketLength(int16_t const &packet_length) {
    *((int16_t *)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  inline void SetPacketSequenceNumber(int32_t const &packet_sequence_number) {
    *((int32_t *)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  //  inline void SetPacketReservedSequenceNumber(int16_t const &packet_reserved_sequence_number) {
  //    *((int16_t *)(msg_ptr + NSE_PACKET_RESERVED_SEQUENCE_NUMBER_OFFSET)) = hton16(packet_reserved_sequence_number);
  //  }

  inline void SetPacketMD5CheckSum(char const *packet_md5sum_checksum) {
    memcpy((void *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET), packet_md5sum_checksum, NSE_PACKET_CHECKSUM_LENGTH);
  }

  //  inline void SetPacketMessageCount(int16_t const &packet_message_count) {
  //    *((int16_t *)(msg_ptr + NSE_PACKET_MESSAGE_COUNT_OFFSET)) = hton16(packet_message_count);
  //  }

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

 public:
  inline void SetPreLoadedLogoffRequestFields(int32_t const &user_id) { SetMessageHeaderTraderId(user_id); }

  inline void SetDynamicLogoffRequestFields(int32_t const &packet_sequence_number) {
    SetPacketSequenceNumber(packet_sequence_number);
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_LOGOFF_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  char const *GetLogoffRequestBuffer() const { return logoff_request_buffer; }
};
}
}
