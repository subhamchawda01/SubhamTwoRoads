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

#include "infracore/NSET/NSETemplates/LogonRequest.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"

namespace HFSAT {
namespace NSE {

/*
 * Defines logon request semantics for cash market
 * Extends generic  NSELogonRequest  template for NSE trimmed structure.
 */

class NSELogonRequestCashMarket : public NSELogonRequest {
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CM_LOGON_REQUEST_LENGTH);

    // Reserved Sequence is always filled with 0
    //    SetPacketReservedSequenceNumber(0);

    // Message Count is 1 as we are only sending login message in this packet
    //    SetPacketMessageCount(1);

    //================================= @ Message Header

    // Login Transaction Code
    SetMessageHeaderTransactionCode(SIGN_ON_REQUEST_IN);

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
        hton16(NSE_CM_LOGON_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    //================================= @ Logon Request

    // The New Password Should Only be Sent if we want to change the password or it has been expired, otherwise should
    // be set to blank
    memset((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_NEWPASSWORD_OFFSET), ' ', NSE_CM_LOGON_REQUEST_NEWPASSWORD_LENGTH);

    // TraderName Should be Set to blank while sending to the exch
    memset((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_TRADERNAME_OFFSET), ' ', NSE_CM_LOGON_REQUEST_TRADERNAME_LENGTH);

    // 0 while sending
    *((int32_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_OFFSET)) = 0;

    // BranchId Should be 1
    // *((int16_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_BRANCHID_OFFSET)) = hton16(1);

    // Version Number - 93600
    *((int32_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_VERSIONNUMBER_OFFSET)) = hton32(93600);

    // 0 while sending
    *((int32_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_VERSIONNUMBER_LENGTH)) = 0;

    // User Type
    // 0 - dealer
    // 4 - corporate manager
    // 5 - brnach manager
    //‘7’ denotes Market Maker. This field should be set to ‘0’ while sending to the host
    *((int16_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_USERTYPE_OFFSET)) = 0;

    // TODO - don't know about this field
    memcpy((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_WSCLASSNAME_OFFSET), "1234567       ",
           NSE_CM_LOGON_REQUEST_WSCLASSNAME_LENGTH);

    // blank while sending
    *((char *)(msg_ptr + NSE_CM_LOGON_REQUEST_BROKERSTATUS_OFFSET)) = ' ';

    // blank while sending
    *((char *)(msg_ptr + NSE_CM_LOGON_REQUEST_SHOWINDEX_OFFSET)) = 'T';

    // set both fields of the struct ( each 1 byte ) to 0
    *((int16_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_OFFSET)) = 0;
  }

  NSELogonRequestCashMarket() {
    logon_request_buffer_ = (char *)calloc(NSE_CM_LOGON_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (logon_request_buffer_ + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // signon field setter

  inline void SetLoginRequestUserId(int32_t const &login_request_user_id) {
    *((int32_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_USERID_OFFSET)) = hton32(login_request_user_id);
  }

  inline void SetLoginRequestPassword(char const *login_request_password) {
    memcpy((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_PASSWORD_OFFSET), (void *)login_request_password,
           NSE_CM_LOGON_REQUEST_PASSWORD_LENGTH);
  }

  inline void SetLoginRequestNewPassword(char const *login_request_new_password) {
    memcpy((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_NEWPASSWORD_OFFSET), (void *)login_request_new_password,
           NSE_CM_LOGON_REQUEST_NEWPASSWORD_LENGTH);
  }
  
  inline void SetLoginRequestBranchId(int16_t const &login_request_branch_id) {
    *((int16_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_BRANCHID_OFFSET)) = hton16(login_request_branch_id);
  }


  inline void SetLoginRequestBrokerId(char const *login_request_broker_id) {
    memcpy((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_BROKERID_OFFSET), (void *)login_request_broker_id,
           NSE_CM_LOGON_REQUEST_BROKERID_LENGTH);
  }

  inline void SetLoginRequestVersionNumber(int32_t const &login_request_version_number) {
    *((int32_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_VERSIONNUMBER_OFFSET)) = hton32(login_request_version_number);
  }

  inline void SetLoginRequestUserType(int16_t const &login_request_user_type) {
    *((int16_t *)(msg_ptr + NSE_CM_LOGON_REQUEST_USERTYPE_OFFSET)) = hton16(0);
  }

  inline void SetLoginRequestWsClassName(char const *login_request_wsclass_name) {
    memcpy((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_WSCLASSNAME_OFFSET), (void *)login_request_wsclass_name,
           NSE_CM_LOGON_REQUEST_WSCLASSNAME_LENGTH);
  }

  inline void SetLoginRequestBrokerName(char const *login_request_broker_name) {
    memcpy((void *)(msg_ptr + NSE_CM_LOGON_REQUEST_BROKERNAME_OFFSET), (void *)login_request_broker_name,
           NSE_CM_LOGON_REQUEST_BROKERNAME_LENGTH);
  }

 public:
  inline void SetDynamicLogonRequestsFieldsAndUpdateChecksum(int32_t const &packet_sequence_number) {
    SetPacketSequenceNumber(packet_sequence_number);

    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_LOGON_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }
  int32_t GetLogonRequestMsgLength() { return NSE_CM_LOGON_REQUEST_LENGTH; }
};
}
}
