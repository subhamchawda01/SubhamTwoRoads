/**
   \file MarketAdapter/minute_bar_security_market_view.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <vector>
#include <deque>
#include <string>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/data_bar_struct.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/safe_array.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/market_defines.hpp"

#include "dvccode/CDef/random_number_generator.hpp"

namespace HFSAT {

class MinuteBarSMVListener {
 public:
  virtual ~MinuteBarSMVListener(){};
  virtual void OnBarUpdate(const unsigned int security_id, const DataBar &minute_bar) = 0;
};
typedef std::vector<MinuteBarSMVListener *> SMVMinuteBarListenerVec;

struct MinuteBarSecurityMarketView {
  DebugLogger &dbglogger_;
  const Watch &watch_;
  const SecurityNameIndexer &sec_name_indexer_;

  std::string shortcode_;
  std::string secname_;
  int security_id_;
  ExchSource_t exch_source_;

  double min_price_increment_;
  int min_order_size_;
  const FastPriceConvertor fast_price_convertor_;

  bool is_ready_;

  DataBar minute_bar_;

  mutable SMVMinuteBarListenerVec minute_bar_listeners_;

  // functions
  MinuteBarSecurityMarketView(DebugLogger &dbglogger, const Watch &watch, SecurityNameIndexer &sec_name_indexer,
                              const std::string &shortcode, const char *exchange_symbol, const unsigned int security_id,
                              const ExchSource_t exch_source);

  virtual ~MinuteBarSecurityMarketView(){};

  bool operator==(const MinuteBarSecurityMarketView &other) const {
    return (shortcode().compare(other.shortcode()) == 0);
  }

  double min_price_increment() const { return min_price_increment_; }

  const DataBar &GetCurrentMinuteBar() { return minute_bar_; }

  int min_order_size() const { return min_order_size_; }

  bool is_ready() const { return is_ready_; }  ///< if the book is a consistent state

  const char *secname() const { return secname_.c_str(); }  ///< returns the exchange symbol for this instrument
  const std::string &shortcode() const { return shortcode_; }
  unsigned int security_id() const { return security_id_; }
  ExchSource_t exch_source() const { return exch_source_; }

  void NotifyMinuteBarListeners(const unsigned int t_security_id_, const DataBar &minute_bar);

  void SubscribeMinuteBars(MinuteBarSMVListener *t_new_listener_) const;
};
}
