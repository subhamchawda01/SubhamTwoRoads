/**
    \file MinuteBar/minute_bar_order_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "baseinfra/MinuteBar/minute_bar_order_manager.hpp"

namespace HFSAT {

MinuteBarOrderManager::MinuteBarOrderManager(DebugLogger& dbglogger, const Watch& watch,
                                             SecurityNameIndexer& sec_name_indexer, BaseTrader& sim_trader,
                                             MinuteBarSecurityMarketView& minute_smv, int runtime_id,
                                             const bool live_trading, int first_client_order_seq)
    : BaseOrderManager(dbglogger, watch, sec_name_indexer, sim_trader, minute_smv.shortcode(), minute_smv.security_id(),
                       minute_smv.secname(), minute_smv.min_price_increment(), first_client_order_seq),
      minute_smv_(minute_smv) {}

void MinuteBarOrderManager::Buy(int size) {
  double ask_price = minute_smv_.GetCurrentMinuteBar().close_ask_px_;
  SendTradeDblPx(ask_price, size, kTradeTypeBuy, 'A');
}

void MinuteBarOrderManager::Sell(int size) {
  double bid_price = minute_smv_.GetCurrentMinuteBar().close_bid_px_;
  SendTradeDblPx(bid_price, size, kTradeTypeSell, 'A');
}
}
