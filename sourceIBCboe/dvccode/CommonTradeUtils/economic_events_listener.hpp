/**
   \file dvccode/CommonTradeUtils/economic_events_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#ifndef BASE_VOLATILETRADINGINFO_ECONOMIC_EVENTS_LISTENER_H
#define BASE_VOLATILETRADINGINFO_ECONOMIC_EVENTS_LISTENER_H

#include <string>

namespace HFSAT {
class EconomicEventsListener {
 public:
  EconomicEventsListener() {}

  // Economic event listeners should implement these.

  // This signals a complete retreat from the market
  // i.e. no best price orders & no supporting orders.
  virtual void OnCriticalEconomicEventStart(const unsigned int security_id_, const std::string &event_string_,
                                            const int severity_) {
    std::cerr << "EconomicEventsListener::OnCriticalEconomicEventStart called." << std::endl;
  }
  virtual void OnCriticalEconomicEventEnd(const unsigned int security_id_, const std::string &event_string_,
                                          const int severity_) {
    std::cerr << "EconomicEventsListener::OnCriticalEconomicEventEnd called." << std::endl;
  }

  // This signals an exclusion from the best bid / asks.
  // Supporting orders should still be kept on.
  virtual void OnUnsafeEconomicEventStart(const unsigned int security_id_, const std::string &event_string_,
                                          const int severity_) {
    std::cerr << "EconomicEventsListener::OnUnsafeEconomicEventStart called." << std::endl;
  }
  virtual void OnUnsafeEconomicEventEnd(const unsigned int security_id_, const std::string &event_string_,
                                        const int severity_) {
    std::cerr << "EconomicEventsListener::OnUnsafeEconomicEventEnd called." << std::endl;
  }

  // Absolute change in price from last time frame is
  // outside of acceptable threshold.
  // Abnormal volatility.
  virtual void OnPriceVolatileEconomicEvent(const unsigned int security_id_, const double price_diff_from_stddev_) {
    std::cerr << "EconomicEventsListener::OnPriceVolatileEconomicEvent called." << std::endl;
  }
};
}

#endif  // BASE_VOLATILETRADINGINFO_ECONOMIC_EVENTS_LISTENER_H
