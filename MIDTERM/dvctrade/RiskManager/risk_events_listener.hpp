/**
   \file RiskManager/risk_events_listener.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/RiskManager/risk_notifier.hpp"

namespace HFSAT {

/// brief Listens to events affecting risk values (our executions: OrderExecutedListener, and mkt_price changes:
/// SecurityMarketViewChangeListener)
class RiskEventsListener : public OrderExecutedListener, public SecurityMarketViewChangeListener {
 protected:
  SecurityMarketView& dep_market_view_;
  DebugLogger& dbglogger_;
  const HFSAT::Watch& watch_;

 public:
  RiskEventsListener(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityNameIndexer& _sec_name_indexer_,
                     SecurityMarketView& _dep_market_view_)
      : dep_market_view_(_dep_market_view_), dbglogger_(_dbglogger_), watch_(_watch_) {
    dep_market_view_.subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice);
  }

  ~RiskEventsListener() {}

  /// @brief SecurityMarketViewChangeListener callback : update place in line of all orders at the best market
  /// Listener to smv's kPriceTypeMktSizeWPrice
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    RiskNotifier& risk_notifier_ = HFSAT::RiskNotifier::getInstance();
    if (risk_notifier_.in_recovery) return;  // during recovery, we do not account for unrealised pnl changes
    risk_notifier_.OnMktPriceChange(dep_market_view_.shortcode(), dep_market_view_.mkt_size_weighted_price());
  }

  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {
    // Do nothing here (assuming mkt trades don't affect risk)
  }

  /// @brief combined_mds_messages_ors_reply_processor callback: called on our order executions
  void OrderExecuted(const int _server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const TradeType_t _buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int _int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const ttime_t time_set_by_server) {
    RiskNotifier& risk_notifier_ = HFSAT::RiskNotifier::getInstance();
    std::stringstream ss;
    ss << watch_.tv();
    // IMPORTANT: We start recovery of all past trades after receiving first execution update for any product.
    // This ensures we buffer all subsequent executions for all products during recovery from  ORS broadcast files.
    if (!risk_notifier_.in_recovery && !risk_notifier_.recovered) {
      dbglogger_ << "Starting risk monitor recovery using past trades at " << ss.str() << " recieved trade for "
                 << dep_market_view_.shortcode() << " saci: " << _server_assigned_client_id_
                 << " sams: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();
      risk_notifier_.in_recovery = true;
      risk_notifier_.LoadPastTradesFromOrsBinFile();
    }

    risk_notifier_.OnOrderExecuted(_server_assigned_client_id_, dep_market_view_.shortcode(), _buysell_,
                                   _size_executed_, _price_, _global_position_, _server_assigned_order_sequence_,
                                   ss.str(), server_assigned_message_sequence);
  }
};
}
