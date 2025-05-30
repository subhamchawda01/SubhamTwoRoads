// =====================================================================================
//
//       Filename:  TradeConfirmationResponse.hpp
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

#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"

namespace HFSAT {
namespace NSE {

/*
 * Defines Trade confirmation semantics for cash market
 * Extends generic  TradeConfirmationResponse  template for NSE trimmed structure.
 */

class TradeConfirmationResponseCashMarket : public TradeConfirmationResponse {
 public:
  TradeConfirmationResponseCashMarket() {
    memset((void*)&processed_trade_conf_response_, 0, sizeof(ProcessedTradeConfirmationResponse));
  }

  inline ProcessedTradeConfirmationResponse* ProcessTradeConfirmationResponse(const char* msg_ptr) {
    memcpy((void*)processed_trade_conf_response_.symbol,
           (void*)(msg_ptr + NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
           NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SYMBOL_LENGTH);

    memcpy((void*)processed_trade_conf_response_.series,
           (void*)(msg_ptr + NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SERIES_OFFSET),
           NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SERIES_LENGTH);

    processed_trade_conf_response_.order_number =
        ntoh64((*((int64_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_ORDER_NUM_OFFSET))));
    
    processed_trade_conf_response_.last_activity_reference =
	ntoh64((*((int64_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_LASTACTIVITYREFERENCE_OFFSET))));

    processed_trade_conf_response_.price = ntoh32(*((int32_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_FILLPRICE_OFFSET)));

    processed_trade_conf_response_.size_executed =
        ntoh32(*((int32_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_FILLQUANTITY_OFFSET)));

    processed_trade_conf_response_.size_remaining =
        ntoh32(*((int32_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_REMAININGVOLUME_OFFSET)));

    processed_trade_conf_response_.buy_sell =
        ntoh16(*((int16_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_BUYSELLINDICATOR_OFFSET)));

    processed_trade_conf_response_.activity_time =
        ntoh32(*((int32_t*)(msg_ptr + NSE_CM_TRADE_RESPONSE_ACTIVITY_TIME_OFFSET)));

    return &processed_trade_conf_response_;
  }
};
}
}
