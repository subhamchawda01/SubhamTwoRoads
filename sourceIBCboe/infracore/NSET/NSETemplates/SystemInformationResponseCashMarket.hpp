// =====================================================================================
//
//       Filename:  SystemInformationResponse.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/30/2015 08:32:04 AM
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

#include "infracore/NSET/NSETemplates/DataDefines.hpp"
#include "infracore/NSET/NSETemplates/SystemInformationResponse.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"

namespace HFSAT {
namespace NSE {

class SystemInfoResponseCashMarket : public SystemInfoResponse {
 public:
  inline ProcessedSystemInformationResponse* ProcessSystemInfoResponse(char const* msg_ptr) {
    processed_system_info_response.st_mkt_status.normal =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_NORMAL_OFFSET)));
    processed_system_info_response.st_mkt_status.oddlot =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_ODDLOT_OFFSET)));
    processed_system_info_response.st_mkt_status.spot =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_SPOT_OFFSET)));
    processed_system_info_response.st_mkt_status.auction =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_AUCTION_OFFSET)));

    processed_system_info_response.call_auction_1 =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_CALL_AUCTION_1_OFFSET)));

    processed_system_info_response.call_auction_2 =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_CALL_AUCTION_2_OFFSET)));

    processed_system_info_response.market_index =
        ntoh32(*((int32_t*)(msg_ptr + NSE_CM_SYSTEMINFO_RESPONSE_MARKETINDEX_OFFSET)));

    return &processed_system_info_response;
  }
};
}
}
