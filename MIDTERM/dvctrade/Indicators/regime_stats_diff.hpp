/*
 * regime_stats_diff.hpp
 *
 *  Created on: 19-Nov-2015
 *      Author: raghuram
 */

#ifndef DVCTRADE_INDICATORS_REGIME_STATS_DIFF_HPP_
#define DVCTRADE_INDICATORS_REGIME_STATS_DIFF_HPP_

#include <time.h>
#include <fstream>

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/stdev_ratio_normalised.hpp"
#include "dvctrade/Indicators/exponential_moving_average.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class RegimeStatsDiff : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& indep_market_view_;

  // computational variables

  std::string data_file_ = "/spare/local/tradeinfo/VIX/VX_0_stats";
  std::string statistic_ = "PRICE_DIFF";
  int column_index_;

  int date_;

  double threshold_;
  double tolerance_;
  const Watch& watch_;
  double value_0_ = 0;
  double value_1_ = 0;
  const PriceType_t price_type_ = StringToPriceType_t("MidPrice");
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static RegimeStatsDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static RegimeStatsDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            const SecurityMarketView& _indep_market_view_, int _column_index_,
                                            double _threshold_, double _tolerance_);

 protected:
  RegimeStatsDiff(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _concise_indicator_description_,
                  const SecurityMarketView& _indep_market_view_, int _column_index_, double _threshold_,
                  double _tolerance_);

 public:
  ~RegimeStatsDiff() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "RegimeStatsDiff"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  // void InitializeValues();
};
}

#endif /* DVCTRADE_INDICATORS_REGIME_STATS_DIFF_HPP_ */
