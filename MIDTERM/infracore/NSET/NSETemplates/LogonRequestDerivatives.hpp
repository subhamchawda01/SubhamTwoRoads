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

#define NSE_LOGON_REQUEST_USERID_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_REQUEST_MESSAGE_HEADER_LENGTH + NSE_PACKET_REQUEST_LENGTH)
#define NSE_LOGON_REQUEST_USERID_LENGTH sizeof(int32_t)

#define NSE_LOGON_REQUEST_STP_RESERVED1_OFFSET (NSE_LOGON_REQUEST_USERID_OFFSET + NSE_LOGON_REQUEST_USERID_LENGTH)
#define NSE_LOGON_REQUEST_STP_RESERVED1_LENGTH 8

#define NSE_LOGON_REQUEST_PASSWORD_OFFSET (NSE_LOGON_REQUEST_STP_RESERVED1_OFFSET+NSE_LOGON_REQUEST_STP_RESERVED1_LENGTH)
#define NSE_LOGON_REQUEST_PASSWORD_LENGTH LENGTH_OF_LOGON_REQUEST_PASSWORD_CHAR_FIELD

#define NSE_LOGON_REQUEST_STP_RESERVED2_OFFSET (NSE_LOGON_REQUEST_PASSWORD_OFFSET+NSE_LOGON_REQUEST_PASSWORD_LENGTH)
#define NSE_LOGON_REQUEST_STP_RESERVED2_LENGTH 8

#define NSE_LOGON_REQUEST_NEWPASSWORD_OFFSET (NSE_LOGON_REQUEST_STP_RESERVED2_OFFSET+NSE_LOGON_REQUEST_STP_RESERVED2_LENGTH)
#define NSE_LOGON_REQUEST_NEWPASSWORD_LENGTH LENGTH_OF_LOGON_REQUEST_PASSWORD_CHAR_FIELD

#define NSE_LOGON_REQUEST_TRADERNAME_OFFSET \
  (NSE_LOGON_REQUEST_NEWPASSWORD_OFFSET + NSE_LOGON_REQUEST_NEWPASSWORD_LENGTH)
#define NSE_LOGON_REQUEST_TRADERNAME_LENGTH LENGTH_OF_LOGON_REQUEST_TRADERNAME_CHAR_FIELD

#define NSE_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_OFFSET \
  (NSE_LOGON_REQUEST_TRADERNAME_OFFSET + NSE_LOGON_REQUEST_TRADERNAME_LENGTH)
#define NSE_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_LENGTH sizeof(int32_t)

#define NSE_LOGON_REQUEST_BROKERID_OFFSET \
  (NSE_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_OFFSET + NSE_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_LENGTH)
#define NSE_LOGON_REQUEST_BROKERID_LENGTH LENGTH_OF_LOGON_REQUEST_BROKERID_CHAR_FIELD

#define NSE_LOGON_REQUEST_RESERVED_1_OFFSET (NSE_LOGON_REQUEST_BROKERID_OFFSET + NSE_LOGON_REQUEST_BROKERID_LENGTH)
#define NSE_LOGON_REQUEST_RESERVED_1_LENGTH sizeof(char)

#define NSE_LOGON_REQUEST_BRANCHID_OFFSET (NSE_LOGON_REQUEST_RESERVED_1_OFFSET + NSE_LOGON_REQUEST_RESERVED_1_LENGTH)
#define NSE_LOGON_REQUEST_BRANCHID_LENGTH sizeof(int16_t)

#define NSE_LOGON_REQUEST_VERSIONNUMBER_OFFSET (NSE_LOGON_REQUEST_BRANCHID_OFFSET + NSE_LOGON_REQUEST_BRANCHID_LENGTH)
#define NSE_LOGON_REQUEST_VERSIONNUMBER_LENGTH sizeof(int32_t)

#define NSE_LOGON_REQUEST_BATCH2STARTTIME_OFFSET \
  (NSE_LOGON_REQUEST_VERSIONNUMBER_OFFSET + NSE_LOGON_REQUEST_VERSIONNUMBER_LENGTH)
#define NSE_LOGON_REQUEST_BATCH2STARTTIME_LENGTH sizeof(int32_t)

#define NSE_LOGON_REQUEST_HOSTSWITCHCONTEXT_OFFSET \
  (NSE_LOGON_REQUEST_BATCH2STARTTIME_OFFSET + NSE_LOGON_REQUEST_BATCH2STARTTIME_LENGTH)
#define NSE_LOGON_REQUEST_HOSTSWITCHCONTEXT_LENGTH sizeof(char)

#define NSE_LOGON_REQUEST_COLOUR_OFFSET \
  (NSE_LOGON_REQUEST_HOSTSWITCHCONTEXT_OFFSET + NSE_LOGON_REQUEST_HOSTSWITCHCONTEXT_LENGTH)
#define NSE_LOGON_REQUEST_COLOUR_LENGTH LENGTH_OF_LOGON_REQUEST_COLOUR_CHAR_FIELD

#define NSE_LOGON_REQUEST_RESERVED_2_OFFSET (NSE_LOGON_REQUEST_COLOUR_OFFSET + NSE_LOGON_REQUEST_COLOUR_LENGTH)
#define NSE_LOGON_REQUEST_RESERVED_2_LENGTH sizeof(char)

#define NSE_LOGON_REQUEST_USERTYPE_OFFSET (NSE_LOGON_REQUEST_RESERVED_2_OFFSET + NSE_LOGON_REQUEST_RESERVED_2_LENGTH)
#define NSE_LOGON_REQUEST_USERTYPE_LENGTH sizeof(int16_t)

#define NSE_LOGON_REQUEST_SEQUENCENUMBER_OFFSET (NSE_LOGON_REQUEST_USERTYPE_OFFSET + NSE_LOGON_REQUEST_USERTYPE_LENGTH)
#define NSE_LOGON_REQUEST_SEQUENCENUMBER_LENGTH sizeof(double)

#define NSE_LOGON_REQUEST_WSCLASSNAME_OFFSET \
  (NSE_LOGON_REQUEST_SEQUENCENUMBER_OFFSET + NSE_LOGON_REQUEST_SEQUENCENUMBER_LENGTH)
#define NSE_LOGON_REQUEST_WSCLASSNAME_LENGTH LENGTH_OF_LOGON_REQUEST_WSCLASSNAME_CHAR_FIELD

#define NSE_LOGON_REQUEST_BROKERSTATUS_OFFSET \
  (NSE_LOGON_REQUEST_WSCLASSNAME_OFFSET + NSE_LOGON_REQUEST_WSCLASSNAME_LENGTH)
#define NSE_LOGON_REQUEST_BROKERSTATUS_LENGTH sizeof(char)

#define NSE_LOGON_REQUEST_SHOWINDEX_OFFSET \
  (NSE_LOGON_REQUEST_BROKERSTATUS_OFFSET + NSE_LOGON_REQUEST_BROKERSTATUS_LENGTH)
#define NSE_LOGON_REQUEST_SHOWINDEX_LENGTH sizeof(char)

#define NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_OFFSET \
  (NSE_LOGON_REQUEST_SHOWINDEX_OFFSET + NSE_LOGON_REQUEST_SHOWINDEX_LENGTH)
#define NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_LENGTH sizeof(int8_t)

#define NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD2_OFFSET \
  (NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_OFFSET +    \
   NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_LENGTH)
#define NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD2_LENGTH sizeof(int8_t)

#define NSE_LOGON_REQUEST_MEMBERTYPE_OFFSET                           \
  (NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD2_OFFSET + \
   NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD2_LENGTH)
#define NSE_LOGON_REQUEST_MEMBERTYPE_LENGTH sizeof(int16_t)

#define NSE_LOGON_REQUEST_CLEARING_STATUS_OFFSET \
  (NSE_LOGON_REQUEST_MEMBERTYPE_OFFSET + NSE_LOGON_REQUEST_MEMBERTYPE_LENGTH)
#define NSE_LOGON_REQUEST_CLEARING_STATUS_LENGTH sizeof(char)

#define NSE_LOGON_REQUEST_BROKERNAME_OFFSET \
  (NSE_LOGON_REQUEST_CLEARING_STATUS_OFFSET + NSE_LOGON_REQUEST_CLEARING_STATUS_LENGTH)
#define NSE_LOGON_REQUEST_BROKERNAME_LENGTH LENGTH_OF_LOGON_REQUEST_BROKERNAME_CHAR_FIELD

#define NSE_LOGON_REQUEST_STP_RESERVED_GROUP_OFFSET \
  (NSE_LOGON_REQUEST_BROKERNAME_OFFSET+NSE_LOGON_REQUEST_BROKERNAME_LENGTH)
#define NSE_LOGON_REQUEST_STP_RESERVED_GROUP_LENGTH 48

#define NSE_LOGON_REQUEST_LENGTH                                                                                      \
  (NSE_PACKET_REQUEST_LENGTH + NSE_REQUEST_MESSAGE_HEADER_LENGTH + NSE_LOGON_REQUEST_USERID_LENGTH +                  \
   NSE_LOGON_REQUEST_PASSWORD_LENGTH + NSE_LOGON_REQUEST_NEWPASSWORD_LENGTH + NSE_LOGON_REQUEST_TRADERNAME_LENGTH +   \
   NSE_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_LENGTH + NSE_LOGON_REQUEST_BROKERID_LENGTH +                              \
   NSE_LOGON_REQUEST_RESERVED_1_LENGTH + NSE_LOGON_REQUEST_BRANCHID_LENGTH + NSE_LOGON_REQUEST_VERSIONNUMBER_LENGTH + \
   NSE_LOGON_REQUEST_BATCH2STARTTIME_LENGTH + NSE_LOGON_REQUEST_HOSTSWITCHCONTEXT_LENGTH +                            \
   NSE_LOGON_REQUEST_COLOUR_LENGTH + NSE_LOGON_REQUEST_RESERVED_2_LENGTH + NSE_LOGON_REQUEST_USERTYPE_LENGTH +        \
   NSE_LOGON_REQUEST_SEQUENCENUMBER_LENGTH + NSE_LOGON_REQUEST_WSCLASSNAME_LENGTH +                                   \
   NSE_LOGON_REQUEST_BROKERSTATUS_LENGTH + NSE_LOGON_REQUEST_SHOWINDEX_LENGTH +                                       \
   NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_LENGTH +                                                 \
   NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD2_LENGTH + NSE_LOGON_REQUEST_MEMBERTYPE_LENGTH +           \
   NSE_LOGON_REQUEST_CLEARING_STATUS_LENGTH + NSE_LOGON_REQUEST_BROKERNAME_LENGTH +                                   \
   NSE_LOGON_REQUEST_STP_RESERVED1_LENGTH + NSE_LOGON_REQUEST_STP_RESERVED2_LENGTH +                                  \
   NSE_LOGON_REQUEST_STP_RESERVED_GROUP_LENGTH)

namespace HFSAT {
namespace NSE {

/*
 * Defines logon request semantics for derivatives market
 * Extends generic  NSELogonRequest  template for NSE trimmed structure.
 */

class NSELogonRequestDerivatives : public NSELogonRequest {
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_LOGON_REQUEST_LENGTH);

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
        hton16(NSE_LOGON_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    //================================= @ Logon Request

    // The New Password Should Only be Sent if we want to change the password or it has been expired, otherwise should
    // be set to blank
    memset((void *)(msg_ptr + NSE_LOGON_REQUEST_NEWPASSWORD_OFFSET), ' ', NSE_LOGON_REQUEST_NEWPASSWORD_LENGTH);

    // TraderName Should be Set to blank while sending to the exch
    memset((void *)(msg_ptr + NSE_LOGON_REQUEST_TRADERNAME_OFFSET), ' ', NSE_LOGON_REQUEST_TRADERNAME_LENGTH);

    // 0 while sending
    *((int32_t *)(msg_ptr + NSE_LOGON_REQUEST_LASTPASSWORDCHANGEDATE_OFFSET)) = 0;

    // BranchId Should be 1
    // *((int16_t *)(msg_ptr + NSE_LOGON_REQUEST_BRANCHID_OFFSET)) = hton16(1);

    // Version Number - 93600
    *((int32_t *)(msg_ptr + NSE_LOGON_REQUEST_VERSIONNUMBER_OFFSET)) = hton32(93600);

    // 0 while sending
    *((int32_t *)(msg_ptr + NSE_LOGON_REQUEST_VERSIONNUMBER_LENGTH)) = 0;

    // blank while sending
    *((char *)(msg_ptr + NSE_LOGON_REQUEST_HOSTSWITCHCONTEXT_OFFSET)) = ' ';

    // blank while sending
    memset((void *)(msg_ptr + NSE_LOGON_REQUEST_COLOUR_OFFSET), ' ', NSE_LOGON_REQUEST_COLOUR_LENGTH);

    // User Type
    // 0 - dealer
    // 4 - corporate manager
    // 5 - brnach manager
    *((int16_t *)(msg_ptr + NSE_LOGON_REQUEST_USERTYPE_OFFSET)) = 5;

    // TODO - don't know about this field
    memcpy((void *)(msg_ptr + NSE_LOGON_REQUEST_WSCLASSNAME_OFFSET), "1234567       ",
           NSE_LOGON_REQUEST_WSCLASSNAME_LENGTH);

    // blank while sending
    *((char *)(msg_ptr + NSE_LOGON_REQUEST_BROKERSTATUS_OFFSET)) = ' ';

    // blank while sending
    *((char *)(msg_ptr + NSE_LOGON_REQUEST_SHOWINDEX_OFFSET)) = 'T';

    // set both fields of the struct ( each 1 byte ) to 0
    *((int16_t *)(msg_ptr + NSE_LOGON_REQUEST_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_OFFSET)) = 0;

    // 0 while sending
    *((int16_t *)(msg_ptr + NSE_LOGON_REQUEST_MEMBERTYPE_OFFSET)) = 0;

    // blank while sending
    *((char *)(msg_ptr + NSE_LOGON_REQUEST_CLEARING_STATUS_OFFSET)) = ' ';
  }

  NSELogonRequestDerivatives() {
    logon_request_buffer_ = (char *)calloc(NSE_LOGON_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (logon_request_buffer_ + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  // signon field setter

  inline void SetLoginRequestUserId(int32_t const &login_request_user_id) {
    *((int32_t *)(msg_ptr + NSE_LOGON_REQUEST_USERID_OFFSET)) = hton32(login_request_user_id);
  }

  inline void SetLoginRequestPassword(char const *login_request_password) {
    memcpy((void *)(msg_ptr + NSE_LOGON_REQUEST_PASSWORD_OFFSET), (void *)login_request_password,
           NSE_LOGON_REQUEST_PASSWORD_LENGTH);
  }

  inline void SetLoginRequestNewPassword(char const *login_request_new_password) {
    memcpy((void *)(msg_ptr + NSE_LOGON_REQUEST_NEWPASSWORD_OFFSET), (void *)login_request_new_password,
           NSE_LOGON_REQUEST_NEWPASSWORD_LENGTH);
  }
  
  inline void SetLoginRequestBranchId(int16_t const &login_request_branch_id) {
    *((int16_t *)(msg_ptr + NSE_LOGON_REQUEST_BRANCHID_OFFSET)) = hton16(login_request_branch_id);
  }

  inline void SetLoginRequestBrokerId(char const *login_request_broker_id) {
    memcpy((void *)(msg_ptr + NSE_LOGON_REQUEST_BROKERID_OFFSET), (void *)login_request_broker_id,
           NSE_LOGON_REQUEST_BROKERID_LENGTH);
  }

  inline void SetLoginRequestVersionNumber(int32_t const &login_request_version_number) {
    *((int32_t *)(msg_ptr + NSE_LOGON_REQUEST_VERSIONNUMBER_OFFSET)) = hton32(login_request_version_number);
  }

  inline void SetLoginRequestUserType(int16_t const &login_request_user_type) {
    *((int16_t *)(msg_ptr + NSE_LOGON_REQUEST_USERTYPE_OFFSET)) = hton16(login_request_user_type);
  }

  inline void SetLoginRequestWsClassName(char const *login_request_wsclass_name) {
    memcpy((void *)(msg_ptr + NSE_LOGON_REQUEST_WSCLASSNAME_OFFSET), (void *)login_request_wsclass_name,
           NSE_LOGON_REQUEST_WSCLASSNAME_LENGTH);
  }

  inline void SetLoginRequestBrokerName(char const *login_request_broker_name) {
    memcpy((void *)(msg_ptr + NSE_LOGON_REQUEST_BROKERNAME_OFFSET), (void *)login_request_broker_name,
           NSE_LOGON_REQUEST_BROKERNAME_LENGTH);
  }

 public:
  inline void SetDynamicLogonRequestsFieldsAndUpdateChecksum(int32_t const &packet_sequence_number) {
    SetPacketSequenceNumber(packet_sequence_number);

    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_LOGON_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  int32_t GetLogonRequestMsgLength() { return NSE_LOGON_REQUEST_LENGTH; }
};
}
}
