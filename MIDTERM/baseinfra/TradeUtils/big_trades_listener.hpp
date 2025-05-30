/**
   \file VolatileTradingInfo/economic_events_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_VOLATILETRADINGINFO_BIG_TRADES_LISTENER_H
#define BASE_VOLATILETRADINGINFO_BIG_TRADES_LISTENER_H

#include <string>

#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

// Big trades listeners should implement these.
class BigTradesListener {
 public:
  // Potentially directional agg. trades.
  virtual void OnLargeDirectionalTrades(const unsigned int t_security_id_, const TradeType_t t_trade_type_,
                                        const int t_min_price_levels_cleared_, const int last_traded_price_,
                                        const int t_cum_traded_size_, const bool is_valid_book_end_) = 0;
};
}

#endif  // BASE_VOLATILETRADINGINFO_BIG_TRADES_LISTENER_H
