// =====================================================================================
//
//       Filename:  SystemInformationDerivatives.hpp
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
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "infracore/NSET/NSETemplates/SystemInformation.hpp"

#define NSE_SYSTEMINFO_REQUEST_LASTUPDATEPORTFOLIOTIME_OFFSET \
  (NSE_REQUEST_START_OFFSET + NSE_REQUEST_MESSAGE_HEADER_LENGTH + NSE_PACKET_REQUEST_LENGTH)
#define NSE_SYSTEMINFO_REQUEST_LASTUPDATEPORTFOLIOTIME_LENGTH sizeof(int32_t)

#define NSE_SYSTEMINFO_REQUEST_LENGTH                              \
  (NSE_PACKET_REQUEST_LENGTH + NSE_REQUEST_MESSAGE_HEADER_LENGTH + \
   NSE_SYSTEMINFO_REQUEST_LASTUPDATEPORTFOLIOTIME_LENGTH)

namespace HFSAT {
namespace NSE {

/*
 * Defines sysinfo request semantics for derivatives market
 * Extends generic  NSESystemInfoRequest  template for NSE trimmed structure.
 */

class NSESystemInfoRequestDerivatives : public NSESystemInfoRequest {
 public:
  void InitializeStaticFields() {
    //================================ @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_SYSTEMINFO_REQUEST_LENGTH);

    //================================= @ Message Header

    // Login Transaction Code
    SetMessageHeaderTransactionCode(SYSTEM_INFORMATION_IN);

    // LogTime Shoudl Be Set To 0 While Sending To Exch
    SetMessageHeaderLogTime(0);

    // Alphachar should be set to blank ( ' ' ) while sending to host
    SetMessageHeaderAlphaChar("  ");

    // ErrorCode Should be 0 While Sending To Exch
    SetMessageHeaderErrorCode(0);

    // All TimeStamp Fields Should Be Set To Numeric 0 While Sending To Exch
    memset((void*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_OFFSET), 0,
           NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP_LENGTH + NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP1_LENGTH +
               NSE_REQUEST_MESSAGE_HEADER_TIMESTAMP2_LENGTH);

    // Length of the message header and the following login request, hence packet length has been deducted
    *((int16_t*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_MESSAGELENGTH_OFFSET)) =
        hton16(NSE_SYSTEMINFO_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    //================================= @ SystemInfoRequest
  }

  NSESystemInfoRequestDerivatives() {
    system_info_request_buffer_ = (char*)calloc(NSE_SYSTEMINFO_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (system_info_request_buffer_ + NSE_REQUEST_START_OFFSET);
    // Initialize The Static Fields
    InitializeStaticFields();
  }

 public:
  inline void SetDynamicSystemInfoRequestFieldsAndUpdateChecksum(int32_t const& packet_sequence_number) {
    SetPacketSequenceNumber(packet_sequence_number);
    HFSAT::MD5::MD5((unsigned char*)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_SYSTEMINFO_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int*)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }
  int32_t GetSystemInfoMsgLength() { return NSE_SYSTEMINFO_REQUEST_LENGTH; }
};
}
}
