/**
   \file CommonTradeUtils/market_update_manager_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_COMMONTRADEUTILS_MARKET_UPDATE_MANAGER_LISTENER_H
#define BASE_COMMONTRADEUTILS_MARKET_UPDATE_MANAGER_LISTENER_H

namespace HFSAT {
class MarketDataInterruptedListener {
 public:
  virtual void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) = 0;
  virtual void OnMarketDataResumed(const unsigned int _security_id_) = 0;
};
}

#endif  // BASE_COMMONTRADEUTILS_MARKET_UPDATE_MANAGER_LISTENER_H
