// =====================================================================================
//
//       Filename:  SystemInformation.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 01:48:32 PM
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

/*
 * Generic interface for nse SysInfo trimmed template. This is required right after login call deemed mandatory by NSE.
 * Please extend this for both derivatives and cash market.
 */

class NSESystemInfoRequest {
 protected:
  char* system_info_request_buffer_;
  const char* msg_ptr;

 public:
  void InitializeStaticFields() {}

  NSESystemInfoRequest() {
    // Initialize The Static Fields
    InitializeStaticFields();
  }
  virtual ~NSESystemInfoRequest() {}

  inline void SetPacketLength(int16_t const& packet_length) {
    *((int16_t*)(msg_ptr + NSE_PACKET_LENGTH_OFFSET)) = hton16(packet_length);
  }

  inline void SetPacketSequenceNumber(int32_t const& packet_sequence_number) {
    *((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)) = hton32(packet_sequence_number);
  }

  inline void SetPacketMD5CheckSum(char const* packet_md5sum_checksum) {
    memcpy((void*)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET), packet_md5sum_checksum, NSE_PACKET_CHECKSUM_LENGTH);
  }

  inline void SetMessageHeaderTransactionCode(int16_t const& message_header_transaction_code) {
    *((int16_t*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET)) =
        hton16(message_header_transaction_code);
  }

  inline void SetMessageHeaderLogTime(int32_t const& message_header_logtime) {
    *((int32_t*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_LOGTIME_OFFSET)) = hton32(message_header_logtime);
  }

  inline void SetMessageHeaderAlphaChar(char const* message_header_alpha_char) {
    memcpy((void*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_OFFSET), message_header_alpha_char,
           NSE_REQUEST_MESSAGE_HEADER_ALPHACHAR_LENGTH);
  }

  inline void SetMessageHeaderTraderId(int32_t const& message_header_trader_id) {
    *((int32_t*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRADERID_OFFSET)) = hton32(message_header_trader_id);
  }

  inline void SetMessageHeaderErrorCode(int16_t const& message_header_error_code) {
    *((int16_t*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_ERRORCODE_OFFSET)) = hton16(message_header_error_code);
  }

 public:
  void SetPreLoadedSystemInfoRequestFields(int32_t const& user_id) { SetMessageHeaderTraderId(user_id); }

  virtual void SetDynamicSystemInfoRequestFieldsAndUpdateChecksum(int32_t const& packet_sequence_number) {}

  char const* GetSystemInfoRequestBuffer() const { return system_info_request_buffer_; }
  virtual int32_t GetSystemInfoMsgLength() { return 0; }
};
}
}
