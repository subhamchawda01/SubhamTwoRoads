// =====================================================================================
//
//       Filename:  UpdateLocalDatabaseRequestCashMarket.hpp
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

#include "infracore/NSET/NSETemplates/UpdateLocalDatabaseRequest.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"

namespace HFSAT {
namespace NSE {

class NSEUpdateLocalDatabaseRequestCashMarket : public NSEUpdateLocalDatabaseRequest {
 public:
  void InitializeStaticFields() {
    //================================= @ Packet

    // Total Length Of The Packet To Be Sent
    SetPacketLength(NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH);

    //================================= @ Message Header

    // Login Transaction Code
    SetMessageHeaderTransactionCode(UPDATE_LOCALDB_IN);

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
        hton16(NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH);

    //================================= @ SystemInfoRequest
  }

 public:
  NSEUpdateLocalDatabaseRequestCashMarket() {
    update_local_database_request_buffer_ = (char *)calloc(NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH, sizeof(char));
    msg_ptr = (update_local_database_request_buffer_ + NSE_REQUEST_START_OFFSET);

    // Initialize The Static Fields
    InitializeStaticFields();
  }

  inline void SetStMarketStatus(st_market_status &st_mkt_status) {
    *((int16_t *)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_NORMAL_OFFSET)) = hton16(st_mkt_status.normal);
    *((int16_t *)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_ODDLOT_OFFSET)) = hton16(st_mkt_status.oddlot);
    *((int16_t *)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_SPOT_OFFSET)) = hton16(st_mkt_status.spot);
    *((int16_t *)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_AUCTION_OFFSET)) = hton16(st_mkt_status.auction);
  }

  inline void SetDynamicUpdateLocalDatabaseRequestFields(int32_t const &packet_sequence_number,
                                                         st_market_status &st_mkt_status,
                                                         st_ex_market_status &st_ex_mkt_status,
                                                         st_pl_market_status &st_pl_mkt_status, int16_t call_auction_1,
                                                         int16_t call_auction_2) {
    SetPacketSequenceNumber(packet_sequence_number);
    SetStMarketStatus(st_mkt_status);

    *((int16_t *)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_CALL_AUCTION_1_OFFSET)) = hton16(call_auction_1);
    *((int16_t *)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_CALL_AUCTION_2_OFFSET)) = hton16(call_auction_2);
    if (AVX512_SUPPORTED)
    HFSAT::MD5::MD5_AVX512VL((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
    else
    HFSAT::MD5::MD5((unsigned char *)(msg_ptr + NSE_REQUEST_MESSAGE_HEADER_TRANSACTION_CODE_OFFSET),
                    NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH - NSE_PACKET_REQUEST_LENGTH,
                    (unsigned int *)(msg_ptr + NSE_PACKET_CHECKSUM_OFFSET));
  }

  int32_t GetUpdateLocalDatabaseRequestLength() { return NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH; }
};
}
}
