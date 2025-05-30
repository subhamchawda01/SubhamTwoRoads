/**
   \file MinuteBar/minute_bar_security_market_view.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <iostream>
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <ctime>
#include <math.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/safe_array.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MinuteBar/minute_bar_security_market_view.hpp"

namespace HFSAT {

MinuteBarSecurityMarketView::MinuteBarSecurityMarketView(DebugLogger &dbglogger, const Watch &watch,
                                                         SecurityNameIndexer &sec_name_indexer,
                                                         const std::string &shortcode, const char *exchange_symbol,
                                                         const unsigned int security_id, const ExchSource_t exch_source)
    : dbglogger_(dbglogger),
      watch_(watch),
      sec_name_indexer_(sec_name_indexer),
      shortcode_(shortcode),
      secname_(exchange_symbol),
      security_id_(security_id),
      exch_source_(exch_source),
      min_price_increment_(SecurityDefinitions::GetContractMinPriceIncrement(shortcode_, watch_.YYYYMMDD())),
      min_order_size_(SecurityDefinitions::GetContractMinOrderSize(shortcode_, watch_.YYYYMMDD())),
      fast_price_convertor_(SecurityDefinitions::GetContractMinPriceIncrement(shortcode_, watch_.YYYYMMDD())),
      is_ready_(false),
      minute_bar_(),
      minute_bar_listeners_() {}

void MinuteBarSecurityMarketView::SubscribeMinuteBars(MinuteBarSMVListener *new_listener) const {
  VectorUtils::UniqueVectorAdd(minute_bar_listeners_, new_listener);
}

void MinuteBarSecurityMarketView::NotifyMinuteBarListeners(const unsigned int security_id, const DataBar &minute_bar) {
  for (auto listener : minute_bar_listeners_) {
    listener->OnBarUpdate(security_id, minute_bar);
  }
}
}
