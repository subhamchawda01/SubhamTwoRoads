/**
   \file VolatileTradingInfo/sweep_info_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2016
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_VOLATILETRADINGINFO_SWEEP_INFO_MANAGER_H
#define BASE_VOLATILETRADINGINFO_SWEEP_INFO_MANAGER_H

#include <string>
#include "baseinfra/TradeUtils/sweep_info_listener.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

namespace HFSAT {

struct SweepInfo {
  int num_sweeps_;        // in last 10 msecs, 5 levels are cleared
  int num_bunch_sweeps_;  // continuous sweeps 10 secs
  int sweep_start_level_;
  int current_sweep_level_;
  int last_sweep_time_;
  int sweep_start_time_;
  bool is_sweep_going_on_;
  int sweep_volume_traded_;
  int actual_level_swept_;
  int last_int_price_swept_;

  SweepInfo()
      : num_sweeps_(0),
        num_bunch_sweeps_(0),
        sweep_start_level_(0),
        current_sweep_level_(0),
        last_sweep_time_(0),
        sweep_start_time_(0),
        is_sweep_going_on_(false),
        sweep_volume_traded_(0),
        actual_level_swept_(0),
        last_int_price_swept_(0) {}
};

// Sweep Info Manager
class SweepInfoManager : public SecurityMarketViewChangeListener {
 public:
  SecurityMarketView* smv_;
  const Watch& watch_;
  DebugLogger& dbglogger_;
  std::vector<SweepInfoListener*> listeners_;
  int level_cutoff_;
  int msecs_to_cumulate_;
  SweepInfo buy_sweep_;
  SweepInfo sell_sweep_;

  /*
   * Params: 1) Level Cutoff: How many levels to be swept to be considered a sweep
   * 		 2) Msecs_To_Cumulate: In how many msecs should the Level_Cutoff # of levels get cleared to be a sweep
   */
  SweepInfoManager(SecurityMarketView* smv, const Watch& watch, DebugLogger& dbglogger, int level_cutoff,
                   int msecs_to_cumulate);
  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print,
                    const MarketUpdateInfo& market_update);
  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info){};
  inline void AddListener(SweepInfoListener* listener) { HFSAT::VectorUtils::UniqueVectorAdd(listeners_, listener); }

  inline void NotifyListeners(int security_id, TradeType_t sweep_type) {
    for (auto listener : listeners_) {
      listener->OnSweep(security_id, sweep_type);
    }
  }

  virtual ~SweepInfoManager();
};
}

#endif  // BASE_VOLATILETRADINGINFO_SWEEP_INFO_MANAGER_H
