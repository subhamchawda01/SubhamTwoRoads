// =====================================================================================
//
//       Filename:  OrderEntry.hpp
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
#include "infracore/NSET/NSETemplates/OrderResponses.hpp"

#define NSE_CM_ORDER_RESPONSE_LENGTH                            \
  (NSE_CM_ORDER_RESPONSE_STP_RESERVED_OFFSET+ \
   NSE_CM_ORDER_RESPONSE_STP_RESERVED_LENGTH)

namespace HFSAT {
namespace NSE {

/*
 * Defines Order response semantics for cash market(CM)
 * Extends generic orderResponse template for NSE trimmed structure.
 */

class OrderResponseCashMarket : public OrderResponse {
 private:
 public:
  OrderResponseCashMarket() {}

  inline ProcessedOrderResponse *ProcessOrderResponse(const char *msg_ptr) {
    memcpy((void *)processed_order_response_.symbol,
           (void *)(msg_ptr + NSE_CM_ORDER_RESPONSE_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
           NSE_CM_ORDER_RESPONSE_CONTRACT_SECINFO_STRUCT_SYMBOL_LENGTH);
        
    memcpy((void *)processed_order_response_.account_number, (void *)(msg_ptr +  NSE_CM_ORDER_RESPONSE_ACCOUNTNUMBER_OFFSET),
           NSE_CM_ORDER_RESPONSE_ACCOUNTNUMBER_LENGTH);

    memcpy((void *)processed_order_response_.settlor, (void *)(msg_ptr +  NSE_CM_ORDER_RESPONSE_SETTLOR_OFFSET),
           NSE_CM_ORDER_RESPONSE_SETTLOR_LENGTH);
    memcpy((void *)processed_order_response_.brokerid, (void *)(msg_ptr +  NSE_CM_ORDER_RESPONSE_BROKERID_OFFSET),
           NSE_CM_ORDER_RESPONSE_BROKERID_LENGTH);
   memcpy((void *)processed_order_response_.pan, (void *)(msg_ptr +  NSE_CM_ORDER_RESPONSE_PAN_OFFSET),
           NSE_CM_ORDER_RESPONSE_PAN_LENGTH);

    memcpy((void *)processed_order_response_.series,
           (void *)(msg_ptr + NSE_CM_ORDER_RESPONSE_CONTRACT_SECINFO_STRUCT_SERIES_OFFSET),
           NSE_CM_ORDER_RESPONSE_CONTRACT_SECINFO_STRUCT_SERIES_LENGTH);

    processed_order_response_.order_number =
        (int64_t)ntoh64(*((int64_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_ORDERNUMBER_OFFSET)));

    processed_order_response_.price =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_PRICE_OFFSET)));
    processed_order_response_.size =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_TOTALVOLUMEREMAINING_OFFSET)));

    processed_order_response_.disclosed_size =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_DISCLOSEDVOLUMEREMAINING_OFFSET)));

    processed_order_response_.entry_date_time =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_ENTRYDATETIME_OFFSET)));

    processed_order_response_.last_modified_date_time =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_LASTMODIFIED_OFFSET)));

    processed_order_response_.last_activity_reference = 
        ntoh64(*((int64_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_LASTACTIVITYREFERENCE_OFFSET )));

    processed_order_response_.trader_id =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_TRADERID_OFFSET)));
    
    processed_order_response_.saos =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_TRANSACTION_ID_OFFSET)));
    processed_order_response_.reason_code =
        ntoh16(*((int16_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_REASONCODE_OFFSET)));
    processed_order_response_.error_code =
        ntoh16(*((int16_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_ERRORCODE_OFFSET)));
    processed_order_response_.proclient =
        ntoh16(*((int16_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_PROCLIENTINDICATOR_OFFSET)));
   processed_order_response_.branch_id =
        ntoh16(*((int16_t *)(msg_ptr + NSE_CM_ORDER_RESPONSE_BRANCHID_OFFSET))); 
    return &processed_order_response_;
  }
};
}
}
