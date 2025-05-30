/**
    \file Indicators/time_window_pair_trade_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_WINDOW_PAIR_DECAYED_TRADE_INFO_MANAGER_H
#define BASE_INDICATORS_WINDOW_PAIR_DECAYED_TRADE_INFO_MANAGER_H

#include <map>
#include <queue>
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/time_window_trade_info_manager.hpp"
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
class TimeWindowPairTradeInfoManager : public SecurityMarketViewChangeListener,
                                       public SlowStdevCalculatorListener,
                                       public TimePeriodListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  TimeWindowTradeInfoManager& indep1_time_window_trade_info_manager_;
  TimeWindowTradeInfoManager& indep2_time_window_trade_info_manager_;

  SecurityMarketView& indep_market_view1_;
  SecurityMarketView& indep_market_view2_;

  bool is_ready1_;
  bool is_ready2_;

  int last_market_update_msecs_;
  double indep1_last_mkt_price_;
  double indep2_last_mkt_price_;

  double indep1_min_price_increment_;
  double indep2_min_price_increment_;

  // Set of booleans showing whether the corresponding variable needs to be computed or not
  bool computing_beta_;
  bool computing_correlation_;
  std::queue<double> px_prod_bwq_;
  std::queue<double> indep1_px_bwq_;
  std::queue<double> indep1_px2_bwq_;
  std::queue<int> indep1_sz_bwq_;
  std::queue<double> indep2_px_bwq_;
  std::queue<double> indep2_px2_bwq_;
  std::queue<int> indep2_sz_bwq_;

 public:
  int window_size_;
  unsigned int num_bw_;
  int bw_count_;
  int bw_size_;
  double basic_window_;
  double indep1_sum_bw_sz_;
  double indep1_sum_bw_px_;
  double indep1_sum_bw_px2_;
  double indep2_sum_bw_sz_;
  double indep2_sum_bw_px_;
  double indep2_sum_bw_px2_;
  double sum_bw_px_prod_;
  double sumpxprod_;
  double indep1_sumpx_;
  double indep1_sumpx2_;
  double indep1_avgpx_;
  double indep1_avgpx2_;
  double indep1_sumsz_;
  double indep1_sdvpx_;
  double indep2_sumpx_;
  double indep2_sumpx2_;
  double indep2_avgpx_;
  double indep2_avgpx2_;
  double indep2_sumsz_;
  double indep2_sdvpx_;
  double beta_;
  double correlation_;

 protected:
  TimeWindowPairTradeInfoManager(DebugLogger& _dbglogger_, const Watch& _watch_,
                                 SecurityMarketView& _indep_market_view1_, SecurityMarketView& _indep_market_view2_,
                                 double _seconds_);

 public:
  static TimeWindowPairTradeInfoManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                           SecurityMarketView& _indep_market_view1_,
                                                           SecurityMarketView& _indep_market_view2_, double _seconds_);

  void compute_beta();
  void compute_correlation();

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
