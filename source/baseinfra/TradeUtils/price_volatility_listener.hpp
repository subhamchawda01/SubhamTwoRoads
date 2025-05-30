/**
   \file VolatileTradingInfo/economic_events_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_VOLATILETRADINGINFO_PRICE_VOLATILITY_LISTENER_H
#define BASE_VOLATILETRADINGINFO_PRICE_VOLATILITY_LISTENER_H

#include <string>

#include "dvccode/CDef/defines.hpp"

namespace HFSAT {
typedef enum {
  kLargePriceChange,
  kLargeOrderSize,
  kLargeDirectionalTrades,
  kNormalBook,
  kNormalPrice,
  kUnknownEvent
} PriceVolatileEvent_t;

inline const std::string PriceVolatileEventToString(const PriceVolatileEvent_t t_event_type_) {
  switch (t_event_type_) {
    case kLargePriceChange:
      return "LargePriceChange";
      break;

    case kLargeOrderSize:
      return "LargeOrderSize";
      break;

    case kLargeDirectionalTrades:
      return "LargeDirectionalTrades";
      break;

    case kNormalBook:
      return "NormalBook";
      break;

    case kNormalPrice:
      return "NormalPrice";
      break;

    case kUnknownEvent:
    default:
      return "UnknownEvent";
      break;
  }
}

// Price event listeners should implement these.
class PriceVolatilityListener {
 public:
  // Absolute change in price from last time frame is
  // outside of acceptable threshold.
  // Abnormal volatility.
  virtual void OnLargePriceChangeEvent(const unsigned int security_id_, const double price_dev_factor_) {
    std::cerr << "PriceVolatilityListener::OnLargePriceChangeEvent called." << std::endl;
  }

  // Abnormally large orders at best level.
  virtual void OnLargeOrderSizeEvent(const unsigned int t_security_id_, const int t_level_,
                                     const TradeType_t t_trade_type_, const int t_large_order_size_) {
    std::cerr << "PriceVolatilityListener::OnLargeOrderSizeEvent called." << std::endl;
  }

  // Potentially directional agg. trades.
  virtual void OnLargeDirectionalTrades(const unsigned int t_security_id_, const TradeType_t t_trade_type_,
                                        const int t_price_levels_cleared_, const int t_cum_traded_size_) {
    std::cerr << "PriceVolatilityListener::OnLargeDirectionalTrades called." << std::endl;
  }

  virtual void OnNormalBook(const unsigned int t_security_id_) {
    std::cerr << "PriceVolatilityListener::OnNormalBook called." << std::endl;
  }

  virtual void OnNormalPrice(const unsigned int t_security_id_) {
    std::cerr << "PriceVolatilityListener::OnNormalPrice called." << std::endl;
  }
};
}

#endif  // BASE_VOLATILETRADINGINFO_ECONOMIC_EVENTS_LISTENER_H
