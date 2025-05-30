// =====================================================================================
//
//       Filename:  SpreadOrderResponses.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/29/2015 11:11:05 PM
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

#include "infracore/NSET/NSETemplates/RequestHeader.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "infracore/NSET/NSETemplates/DataDefines.hpp"
#include "infracore/NSET/NSETemplates/SpreadOrderResponses.hpp"

namespace HFSAT {
namespace NSE {

// ============= SpreadOrdeEntryConfirmation Response =====================================

class OrderResponseThreeLeg {
 private:
  ProcessedSpreadOrderResponse processed_order_response_;

 public:
  OrderResponseThreeLeg() { memset((void*)&processed_order_response_, 0, sizeof(ProcessedSpreadOrderResponse)); }

  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int16_t const &buy_sell,
                                                int32_t const &price, HFSAT::ORS::Order *order_,
                                                InstrumentDesc *inst_desc, SecurityInfoCashMarket *sec_info,
                                                bool is_mo = false, bool add_preopen = false) {
                                                  std::cout <<"Empty...." << std::endl;
                                                }
  inline ProcessedSpreadOrderResponse *ProcessOrderResponse(const char *msg_ptr) {
    //NEED TO CHECK THE RETURN PROPER
    processed_order_response_.entry_date_time =
        ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_ENTRYDATETIME_OFFSET)));
    processed_order_response_.token =
        ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_TOKENNO_OFFSET)));
    processed_order_response_.token_2 =
        ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_MS_SPD_LEG_2_TOKENNO_OFFSET)));
    processed_order_response_.token_3 =
        ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_MS_SPD_LEG_3_TOKENNO_OFFSET)));


    // std::cout << processed_order_response_.token << " " << processed_order_response_.token_2 << " "
    // << processed_order_response_.token_3 << std::endl;

    processed_order_response_.order_number =
        ntoh64(*((int64_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_ORDERNUMBER_OFFSET)));

    processed_order_response_.price = ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_PRICE_OFFSET)));
    processed_order_response_.price_2 = ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_MS_SPD_LEG_2_PRICE_OFFSET)));
    processed_order_response_.price_3 = ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_MS_SPD_LEG_3_PRICE_OFFSET)));

    processed_order_response_.last_modified_date_time =
        ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_LASTMODIFIED_OFFSET)));

    processed_order_response_.size = ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_VOLUME_OFFSET)));
    processed_order_response_.size_2 = ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_MS_SPD_LEG_2_VOLUME_OFFSET)));
    processed_order_response_.size_3 = ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_MS_SPD_LEG_3_VOLUME_OFFSET)));
    
    
    processed_order_response_.saos =
        ntoh32(*((int32_t*)(msg_ptr + NSE_SPREAD_ORDERENTRY_RESPONSE_PRICEDIFF_OFFSET))); // only can work in 3leg order. spread order propably will not work.
    // std::cout <<"OrderResponseThreeLeg " << processed_order_response_.ToString();
    return &processed_order_response_;
  }
};
}
}
