/**
    \file Indicators/time_window_trade_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_WINDOW_DECAYED_TRADE_INFO_MANAGER_H
#define BASE_INDICATORS_WINDOW_DECAYED_TRADE_INFO_MANAGER_H

#include <map>
#include <queue>
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

namespace HFSAT {

/// Class that takes a decay factor, or rather a
/// _fractional_seconds_ time to fadeoff
/// and encapsulates the logic to compute trade based variables
/// like time decayed sum of tradepx - mktpx
/// Better than writing the logic in that indicator when
/// subcomputations of two required indicators is common
/// single repository for all the logic
/// multiple calls to the same variable
class TimeWindowTradeInfoManager : public SecurityMarketViewChangeListener,
                                   public SlowStdevCalculatorListener,
                                   public TimePeriodListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  SecurityMarketView& indep_market_view_;
  bool is_ready_;
  double last_mkt_price_;

  double min_price_increment_;

  // Set of booleans showing whether the corresponding variable needs to be computed or not
  bool computing_avgpx_;
  bool computing_sumsz_;
  bool computing_sdvpx_;
  std::queue<double> px_bwq_;
  std::queue<double> px2_bwq_;
  std::queue<int> sz_bwq_;

 public:
  int window_size_;
  unsigned int num_bw_;
  int bw_count_;
  int bw_size_;
  double basic_window_;
  double sum_bw_sz_;
  double sum_bw_px_;
  double sum_bw_px2_;
  int last_recorded_num_trades_;
  int num_trades_;
  double sumpx_;
  double sumpx2_;
  double avgpx_;
  double avgpx2_;
  double sumsz_;
  double sdvpx_;

 protected:
  TimeWindowTradeInfoManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _indep_market_view_,
                             double _seconds_);

 public:
  static TimeWindowTradeInfoManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                       SecurityMarketView& _indep_market_view_, double _seconds_);

  void compute_sumsz();
  void compute_avgpx();
  void compute_sdvpx();

  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  // functions
  static std::string VarName() { return "TimeWindowTradeInfoManager"; }

 protected:
  void InitializeValues();
  void AdjustResults();
  inline void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {}
};
}

#endif  // BASE_INDICATORS_TIME_WINDOW_TRADE_INFO_MANAGER_H
