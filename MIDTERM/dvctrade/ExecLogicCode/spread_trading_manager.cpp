/*
 * spread_trading_manager.cpp
 *
 *  Created on: 12-May-2014
 *      Author: archit
 */

#include "dvctrade/ExecLogic/spread_trading_manager.hpp"

namespace HFSAT {

SpreadTradingManager::SpreadTradingManager(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const SpreadMarketView& _spread_market_view_)
    : TradingManager(_dbglogger_, _watch_),
      spread_market_view_(_spread_market_view_),
      shortcode_vec_(_spread_market_view_.GetShcVec()),
      uts_vec_(_spread_market_view_.GetUtsVec()),
      stm_listeners_() {
  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    secid_to_idx_map_[HFSAT::SecurityNameIndexer::GetUniqueInstance().GetIdFromString(shortcode_vec_[i])] = i;
    stm_listeners_.push_back(NULL);
  }
}

bool SpreadTradingManager::AddStmListener(SpreadTradingManagerListener* _listener_) {
  unsigned int t_sec_id_ = _listener_->GetSecId();
  if (secid_to_idx_map_.find(t_sec_id_) != secid_to_idx_map_.end()) {
    stm_listeners_[secid_to_idx_map_[t_sec_id_]] = _listener_;
    return true;
  }
  return false;
}

void SpreadTradingManager::OnPositionUpdate(unsigned int _securit_id_, int _new_position_) {
  unsigned int t_update_idx_ = secid_to_idx_map_[_securit_id_];

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << "t_update_idx_: " << t_update_idx_ << " shc_: " << shortcode_vec_[t_update_idx_]
                                << " _new_position_: " << _new_position_ << DBGLOG_ENDL_FLUSH;
  }

  for (auto i = 0u; i < stm_listeners_.size(); i++) {
    int t_spread_pos_ = (uts_vec_[i] / double(uts_vec_[t_update_idx_])) * _new_position_;

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << "shc_: " << shortcode_vec_[i] << " t_spread_pos_: " << t_spread_pos_
                                  << DBGLOG_ENDL_FLUSH;
    }

    stm_listeners_[i]->OnPositionUpdate(t_spread_pos_);
  }
}

void SpreadTradingManager::ReportResults(HFSAT::BulkFileWriter& trades_writer_) {
  int t_total_volume_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;

  int t_total_pnl_ = stm_listeners_[0]->order_manager().base_pnl().mult_total_pnl();

  for (auto i = 0u; i < stm_listeners_.size(); i++) {
    const SmartOrderManager& t_order_manager_ = stm_listeners_[i]->order_manager();
    t_total_volume_ += t_order_manager_.trade_volume();
    t_supporting_orders_filled_ += t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
    t_bestlevel_orders_filled_ += t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
    t_improve_orders_filled_ += t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
    t_aggressive_orders_filled_ += t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();
  }

  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}

} /* namespace HFSAT */
