// =====================================================================================
//
//       Filename:  LogonRequest.hpp
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

namespace HFSAT {
namespace NSE {

/*
 * Generic  interface for nse logon request trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class NSELogonRequest {
 protected:
  char *logon_request_buffer_;
  const char *msg_ptr;

 public:
  void InitializeStaticFields() {}

  NSELogonRequest() {
    logon_request_buffer_ = nullptr;
    msg_ptr = nullptr;
  }
  virtual ~NSELogonRequest() {}

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

  void SetLoginRequestTransactionCode(int16_t const *login_request_transaction_code) {}

  virtual void SetLoginRequestUserId(int32_t const &login_request_user_id) {}

  virtual void SetLoginRequestPassword(char const *login_request_password) {}

  virtual void SetLoginRequestNewPassword(char const *login_request_new_password) {}

  virtual void SetLoginRequestTraderName(char const *login_request_trader_name) {}

  virtual void SetLoginRequestLastPasswordChangeDate(int32_t const &last_password_change_date) {}

  virtual void SetLoginRequestBrokerId(char const *login_request_broker_id) {}

  virtual void SetLoginRequestBranchId(int16_t const &login_request_branch_id) {}

  virtual void SetLoginRequestVersionNumber(int32_t const &login_request_version_number) {}

  virtual void SetLoginRequestBatch2StartTime(int32_t const &login_request_batch2starttime) {}

  virtual void SetLoginRequestHostSwitchContext(char const &login_request_host_switch_context) {}

  virtual void SetLoginRequestColour(char const *login_request_colour) {}

  virtual void SetLoginRequestUserType(int16_t const &login_request_user_type) {}

  virtual void SetLoginRequestSequenceNumber(double const &login_request_sequence_number) {}

  virtual void SetLoginRequestWsClassName(char const *login_request_wsclass_name) {}

  virtual void SetLoginRequestBrokerStatus(char const &login_request_broker_status) {}

  virtual void SetLoginRequestShowIndex(char const &login_request_show_index) {}

  virtual void SetLoginRequestSTEligibilityPerMkt(int16_t const &login_request_st_broker_eligibility_per_mkt) {}

  virtual void SetLoginRequestMemberType(int16_t const &login_request_member_type) {}

  virtual void SetLoginRequestCleaingStatus(char const &login_request_cleaing_status) {}

  virtual void SetLoginRequestBrokerName(char const *login_request_broker_name) {}

 public:
  inline void SetPreLoadedLogonRequestsFields(int32_t const &user_id, char const *password, char const *broker_id,
                                              int32_t const &version_number, int16_t const &user_type,
                                              char const *ws_class_name, char const *broker_name,
                                              char const *new_password, int16_t const &branch_id) {
    SetLoginRequestUserId(user_id);
    SetMessageHeaderTraderId(user_id);
    SetLoginRequestPassword(password);
    SetLoginRequestBrokerId(broker_id);
    SetLoginRequestBranchId(branch_id);
    SetLoginRequestVersionNumber(version_number);
    SetLoginRequestUserType(user_type);
    SetLoginRequestWsClassName(ws_class_name);
    SetLoginRequestBrokerName(broker_name);
    SetLoginRequestNewPassword(new_password);
  }

  virtual void SetDynamicLogonRequestsFieldsAndUpdateChecksum(int32_t const &packet_sequence_number) {}

  char const *GetLogonRequestBuffer() const { return logon_request_buffer_; }
  virtual int32_t GetLogonRequestMsgLength() { return 0; }
};
}
}
