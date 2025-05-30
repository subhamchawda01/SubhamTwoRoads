// =====================================================================================
//
//       Filename:  Heartbeat.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 10:56:43 PM
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

#define NSE_HEARTBEAT_LENGTH (NSE_PACKET_REQUEST_LENGTH + NSE_REQUEST_MESSAGE_HEADER_LENGTH)

namespace HFSAT {
namespace NSE {

class NSEHeartbeat {
 private:
  char heartbeat_buffer_[NSE_HEARTBEAT_LENGTH];
  char const *msg_ptr;

 private:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_HEARTBEAT_LENGTH);

    // Reserved Sequence is always filled with 0
    //    SetPacketReservedSequenceNumber(0);

    // Message Count is 1 as we are only sending login message in this packet
    //    SetPacketMessageCount(1);

    //================================= @ Message Header

    // Login Transaction Code
    SetMessageHeaderTransactionCode(NSE_HEARTBEAT);

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
        hton16(NSE_HEARTBEAT_LENGTH - NSE_PACKET_REQUEST_LENGTH);
  }

 public:
  NSEHeartbeat() : msg_ptr(heartbeat_buffer_ + NSE_REQUEST_START_OFFSET) {
    // Initialize Heartbeat Buffer
    memset((void *)heartbeat_buffer_, 0, NSE_HEARTBEAT_LENGTH);

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
  inline void SetPreLoadedHeartbeatFields(int32_t const &user_id) { SetMessageHeaderTraderId(user_id); }

  inline void SetDynamicHeartbeatFields(int32_t const &packet_sequence_number) {
    SetPacketSequenceNumber(packet_sequence_number);
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_HEARTBEAT_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                   NSE_HEARTBEAT_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  inline void SetMD5(){
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_HEARTBEAT_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_HEARTBEAT_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  char const *GetHeartbeatBuffer() const { return heartbeat_buffer_; }
};
}
}
