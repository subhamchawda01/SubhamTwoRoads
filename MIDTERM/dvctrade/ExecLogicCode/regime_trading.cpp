// =====================================================================================
//
//       Filename:  regime_trading.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/05/2014 08:18:18 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvctrade/ExecLogic/regime_trading.hpp"

namespace HFSAT {
RegimeTrading::RegimeTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                             const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                             const std::string& _paramfilename_, const bool _livetrading_,
                             MulticastSenderSocket* _p_strategy_param_sender_socket_,
                             EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                             const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                             const std::vector<std::string> _this_model_source_shortcode_vec_,
                             std::string _strategy_name_)
    : BaseTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                  _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                  t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_),

      PriceBasedAggressiveTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_,
                                  _livetrading_, _p_strategy_param_sender_socket_, t_economic_events_manager_,
                                  t_trading_start_utc_mfm_, t_trading_end_utc_mfm_, t_runtime_id_,
                                  _this_model_source_shortcode_vec_),

      DirectionalAggressiveTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_,
                                   _livetrading_, _p_strategy_param_sender_socket_, t_economic_events_manager_,
                                   t_trading_start_utc_mfm_, t_trading_end_utc_mfm_, t_runtime_id_,
                                   _this_model_source_shortcode_vec_),

      PriceBasedVolTrading(_dbglogger_, _watch_, _dep_market_view_, _order_manager_, _paramfilename_, _livetrading_,
                           _p_strategy_param_sender_socket_, t_economic_events_manager_, t_trading_start_utc_mfm_,
                           t_trading_end_utc_mfm_, t_runtime_id_, _this_model_source_shortcode_vec_)

{
  // this gets  reset in PBVol so setting it back
  should_increase_thresholds_in_volatile_times_ = true;
  SetExecLogicForRegimes(_strategy_name_);
}

void RegimeTrading::SetExecLogicForRegimes(std::string _strategy_name_) {
  size_t pos;
  while (_strategy_name_.length() > 0) {
    pos = _strategy_name_.find('-');
    std::string _regime_exec_logic_name_ = "";
    if (pos == std::string::npos) {
      _regime_exec_logic_name_ = _strategy_name_;
    } else {
      _regime_exec_logic_name_ = _strategy_name_.substr(0, pos);
    }

    if (_regime_exec_logic_name_.compare("PriceBasedAggressiveTrading") == 0) {
      regime_to_execlogic_index_.push_back(0);
    } else if (_regime_exec_logic_name_.compare("DirectionalAggressiveTrading") == 0) {
      regime_to_execlogic_index_.push_back(1);
    } else if (_regime_exec_logic_name_.compare("PriceBasedVolTrading") == 0) {
      regime_to_execlogic_index_.push_back(2);
    } else {
      // ExitVerbose( kExitErrorCodeGeneral, dbglogger_, " Could no implementation for " + _regime_exec_logic_name_ + "
      // in RegimeTrading\n" );
    }
    DBGLOG_CLASS_FUNC_LINE << " Setting Index: " << _regime_exec_logic_name_ << regime_to_execlogic_index_.size() << " "
                           << regime_to_execlogic_index_[regime_to_execlogic_index_.size() - 1] << DBGLOG_ENDL_FLUSH;
    _strategy_name_.erase(0, pos + std::string("-").length());
    if (pos == std::string::npos) {
      break;
    }
  }
}

bool RegimeTrading::IsRegimeStrategy(std::string _strategy_name_) {
  if (_strategy_name_.find('-') != std::string::npos) {
    return true;
  }
  return false;
}

void RegimeTrading::TradingLogic() {
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << " Current Param : " << param_index_to_use_
                           << " ExecLogic: " << regime_to_execlogic_index_[param_index_to_use_] << DBGLOG_ENDL_FLUSH;
  }

  switch (regime_to_execlogic_index_[param_index_to_use_]) {
    case 0: {
      PriceBasedAggressiveTrading::TradingLogic();
    } break;
    case 1: {
      DirectionalAggressiveTrading::TradingLogic();
    } break;
    case 2: {
      PriceBasedVolTrading::TradingLogic();
    } break;
    case 3: {
    } break;
    default: { break; }
  }
}

void RegimeTrading::PrintFullStatus() {
  switch (regime_to_execlogic_index_[param_index_to_use_]) {
    case 0: {
      PriceBasedAggressiveTrading::PrintFullStatus();
    } break;
    case 1: {
      DirectionalAggressiveTrading::PrintFullStatus();
    } break;
    case 2: {
      PriceBasedVolTrading::PrintFullStatus();
    } break;
    case 3: {
    } break;
    default: { break; }
  }
}

void RegimeTrading::CallPlaceCancelNonBestLevels() {
  switch (regime_to_execlogic_index_[param_index_to_use_]) {
    case 2: {
      PriceBasedVolTrading::CallPlaceCancelNonBestLevels();
    } break;
    default: {
      BaseTrading::CallPlaceCancelNonBestLevels();
      break;
    }
  }
}

void RegimeTrading::PlaceCancelNonBestLevels() {
  switch (regime_to_execlogic_index_[param_index_to_use_]) {
    case 2: {
      PriceBasedVolTrading::PlaceCancelNonBestLevels();
    } break;
    default: {
      BaseTrading::PlaceCancelNonBestLevels();
      break;
    }
  }
}

void RegimeTrading::OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                           const double _price_, const int r_int_price_, const int _security_id_) {
  switch (regime_to_execlogic_index_[param_index_to_use_]) {
    case 2: {
      PriceBasedVolTrading::OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
    } break;
    default: {
      BaseTrading::OnExec(t_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_);
      break;
    }
  }
}

void RegimeTrading::OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                   const int _security_id_) {
  switch (regime_to_execlogic_index_[param_index_to_use_]) {
    case 2: {
      PriceBasedVolTrading::OnCancelReject(_buysell_, _price_, r_int_price_, _security_id_);
    } break;
    default: {
      BaseTrading::OnCancelReject(_buysell_, _price_, r_int_price_, _security_id_);
      break;
    }
  }
}
}
