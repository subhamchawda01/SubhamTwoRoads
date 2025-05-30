#include "baseinfra/TradeUtils/sweep_info_manager.hpp"

namespace HFSAT {

SweepInfoManager::SweepInfoManager(SecurityMarketView* smv, const Watch& watch, DebugLogger& dbglogger,
                                   int level_cutoff, int msecs_to_cumulate)
    : smv_(smv),
      watch_(watch),
      dbglogger_(dbglogger),
      listeners_(),
      level_cutoff_(level_cutoff),
      msecs_to_cumulate_(msecs_to_cumulate),
      buy_sweep_(),
      sell_sweep_() {
  smv_->subscribe_tradeprints(this);
}

SweepInfoManager::~SweepInfoManager() {}

void SweepInfoManager::OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print,
                                    const MarketUpdateInfo& market_update) {
  HFSAT::TradeType_t trade_type = trade_print.buysell_;
  if (trade_type == HFSAT::kTradeTypeSell) {
    int this_trade_time = watch_.msecs_from_midnight();
    if (this_trade_time - sell_sweep_.sweep_start_time_ > msecs_to_cumulate_) {
      // Sweep which was going on ends here
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        if (sell_sweep_.is_sweep_going_on_) {
          DBGLOG_TIME << "Msecs: " << watch_.msecs_from_midnight()
                      << " SellTotalLevelSwept: " << sell_sweep_.current_sweep_level_
                      << " SellTotalLevelActuallySwept: " << sell_sweep_.actual_level_swept_
                      << " SellTotalVol: " << sell_sweep_.sweep_volume_traded_ << DBGLOG_ENDL_FLUSH;
        }
      }
      sell_sweep_.current_sweep_level_ = 0;
      sell_sweep_.is_sweep_going_on_ = false;
      sell_sweep_.sweep_volume_traded_ = 0;
      sell_sweep_.actual_level_swept_ = 0;

      sell_sweep_.sweep_start_time_ = this_trade_time;
      sell_sweep_.sweep_start_level_ = trade_print.int_trade_price_;
      sell_sweep_.last_int_price_swept_ = trade_print.int_trade_price_;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << "Msecs: " << this_trade_time << " : Resetting SellSweep" << DBGLOG_ENDL_FLUSH;
        DBGLOG_TIME << "Msecs: SellSweepStartTime: " << sell_sweep_.sweep_start_time_
                    << " SellSweepStartLevel: " << sell_sweep_.sweep_start_level_ << DBGLOG_ENDL_FLUSH;
      }
    } else {
      // sweep continues
      sell_sweep_.current_sweep_level_ = sell_sweep_.sweep_start_level_ - trade_print.int_trade_price_;
      sell_sweep_.sweep_volume_traded_ += trade_print.size_traded_;

      if (trade_print.int_trade_price_ < sell_sweep_.last_int_price_swept_) {
        sell_sweep_.actual_level_swept_++;
        sell_sweep_.last_int_price_swept_ = trade_print.int_trade_price_;
      }

      if (sell_sweep_.actual_level_swept_ > level_cutoff_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME << "Msecs: " << this_trade_time
                      << " IndepSellSweep Has Occured: Num : " << sell_sweep_.num_sweeps_ << DBGLOG_ENDL_FLUSH;
        }
        // we have sweep
        if (!sell_sweep_.is_sweep_going_on_) {
          sell_sweep_.num_sweeps_++;

          NotifyListeners(security_id, kTradeTypeSell);
        }

        if (!sell_sweep_.is_sweep_going_on_) {
          sell_sweep_.last_sweep_time_ = this_trade_time;
          sell_sweep_.is_sweep_going_on_ = true;
        }
      }
    }
  } else if (trade_type == HFSAT::kTradeTypeBuy) {
    // BUY
    int this_trade_time = watch_.msecs_from_midnight();
    if (this_trade_time - buy_sweep_.sweep_start_time_ > msecs_to_cumulate_) {
      // Sweep which was going on ends here
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        if (buy_sweep_.is_sweep_going_on_) {
          DBGLOG_TIME << "Msecs: " << watch_.msecs_from_midnight()
                      << " BuyTotalLevelSwept: " << buy_sweep_.current_sweep_level_
                      << " BuyTotalActualLevelSwept: " << buy_sweep_.actual_level_swept_
                      << " BuyTotalVol: " << buy_sweep_.sweep_volume_traded_ << DBGLOG_ENDL_FLUSH;
        }
      }
      buy_sweep_.current_sweep_level_ = 0;
      buy_sweep_.is_sweep_going_on_ = false;
      buy_sweep_.sweep_volume_traded_ = 0;
      buy_sweep_.last_int_price_swept_ = trade_print.int_trade_price_;
      buy_sweep_.actual_level_swept_ = 0;

      buy_sweep_.sweep_start_time_ = this_trade_time;
      buy_sweep_.sweep_start_level_ = trade_print.int_trade_price_;
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME << "Msecs: " << this_trade_time << " : Resetting BuySweep" << DBGLOG_ENDL_FLUSH;
        DBGLOG_TIME << "Msecs: BuySweepStartTime: " << buy_sweep_.sweep_start_time_
                    << " BuySweepStartLevel: " << buy_sweep_.sweep_start_level_ << DBGLOG_ENDL_FLUSH;
      }
    } else {
      // sweep continues
      buy_sweep_.current_sweep_level_ = trade_print.int_trade_price_ - buy_sweep_.sweep_start_level_;
      buy_sweep_.sweep_volume_traded_ += trade_print.size_traded_;

      if (trade_print.int_trade_price_ > buy_sweep_.last_int_price_swept_) {
        buy_sweep_.actual_level_swept_++;
        buy_sweep_.last_int_price_swept_ = trade_print.int_trade_price_;
      }

      if (buy_sweep_.actual_level_swept_ > level_cutoff_) {
        if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
          DBGLOG_TIME << "Msecs: " << this_trade_time << " BuySweep Has Occured: Num : " << buy_sweep_.num_sweeps_
                      << DBGLOG_ENDL_FLUSH;
        }
        // we have sweep
        if (!buy_sweep_.is_sweep_going_on_) {
          buy_sweep_.num_sweeps_++;

          NotifyListeners(security_id, kTradeTypeBuy);
        }
        if (!buy_sweep_.is_sweep_going_on_) {
          buy_sweep_.last_sweep_time_ = this_trade_time;
          buy_sweep_.is_sweep_going_on_ = true;
        }
      }
    }
  }
}
}
