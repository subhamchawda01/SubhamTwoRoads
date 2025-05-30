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
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
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
 * Generic  interface for nse trade confirmation trimmed template.
 * Please extend this for both derivatives and cash market.
 */

class TradeConfirmationResponse {
 protected:
  ProcessedTradeConfirmationResponse processed_trade_conf_response_;

 public:
  TradeConfirmationResponse() {
    memset((void*)&processed_trade_conf_response_, 0, sizeof(ProcessedTradeConfirmationResponse));
  }
  virtual ~TradeConfirmationResponse() {}

  virtual ProcessedTradeConfirmationResponse* ProcessTradeConfirmationResponse(const char* msg_ptr) = 0;
};
}
}
