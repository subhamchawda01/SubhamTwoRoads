// =====================================================================================
//
//       Filename:  LogonResponseCashMarket.hpp
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

#include "infracore/NSET/NSETemplates/LogonResponse.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"

namespace HFSAT {
namespace NSE {

class NSELogonResponseCashMarket : public NSELogonResponse {
 public:
  NSELogonResponseCashMarket() {}

  inline ProcessedLogonResponse *ProcessLogon(char const *msg_ptr) {
    processed_logon_response_.user_id = ntoh32(*((int32_t *)(msg_ptr + NSE_CM_LOGON_RESPONSE_USERID_OFFSET)));

    memcpy((void *)processed_logon_response_.trader_name, (void *)(msg_ptr + NSE_CM_LOGON_RESPONSE_TRADERNAME_OFFSET),
           NSE_CM_LOGON_RESPONSE_TRADERNAME_LENGTH);

    processed_logon_response_.last_password_changed_date_time =
        ntoh32(*((int32_t *)(msg_ptr + NSE_CM_LOGON_RESPONSE_LASTPASSWORDCHANGEDATE_OFFSET)));

    memcpy((void *)processed_logon_response_.broker_id, (void *)(msg_ptr + NSE_CM_LOGON_RESPONSE_BROKERID_OFFSET),
           NSE_CM_LOGON_RESPONSE_BROKERID_LENGTH);

    processed_logon_response_.branch_id = ntoh16(*((int16_t *)(msg_ptr + NSE_CM_LOGON_RESPONSE_BRANCHID_OFFSET)));
    processed_logon_response_.end_time = ntoh32(*((int32_t *)(msg_ptr + NSE_CM_LOGON_RESPONSE_BATCH2STARTTIME_OFFSET)));
    processed_logon_response_.user_type = ntoh16(*((int16_t *)(msg_ptr + NSE_CM_LOGON_RESPONSE_USERTYPE_OFFSET)));
    processed_logon_response_.sequence_number = *((double *)(msg_ptr + NSE_CM_LOGON_RESPONSE_SEQUENCENUMBER_OFFSET));
    processed_logon_response_.broker_status = *((char *)(msg_ptr + NSE_CM_LOGON_RESPONSE_BROKERSTATUS_OFFSET));

    processed_logon_response_.broker_eligibility_per_mkt =
        ntoh16(*((int16_t *)(msg_ptr + NSE_CM_LOGON_RESPONSE_STBROKERELIGIBILITYPERMKT_STRUCT_FIELD1_OFFSET)));

    memcpy((void *)processed_logon_response_.broker_name, (void *)(msg_ptr + NSE_CM_LOGON_RESPONSE_BROKERNAME_OFFSET),
           NSE_CM_LOGON_RESPONSE_BROKERNAME_LENGTH);

    return &processed_logon_response_;
  }

  int32_t GetLogonErrorMsgLength() { return NSE_CM_LOGON_RESPONSE_ERROR_MSG_LENGTH; }
  int32_t GetLogonErrorMsgOffset() { return NSE_CM_LOGON_RESPONSE_ERROR_MSG_OFFSET; }
};
}
}
