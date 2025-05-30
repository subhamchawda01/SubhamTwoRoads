#pragma once

#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"
#include "dvctrade/Indicators/slow_stdev_trend_calculator.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

namespace HFSAT {

class DynamicWeightPortTrend : public IndicatorListener, public CommonIndicator, public RecentSimpleVolumeListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  std::string portfolio_descriptor_shortcode_;
  bool is_ready_;
  std::vector<double> security_id_weight_vec_;
  std::vector<double> current_security_id_weight_vec_;
  PriceType_t price_type_;
  double trend_history_secs_;
  double indicator_duration_;
  bool is_pca_;
  bool is_stdev_;
  bool is_vol_;
  double indicator_n_;
  bool recompute_indicator_value_;
  double indicator_d_;
  bool first_update_;
  std::vector<bool> is_ready_vec_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;
  std::vector<bool> data_interrupted_vec_;
  std::vector<double> trend_vec_;
  std::vector<double> stdev_vec_;
  std::vector<double> vol_vec_;
  std::vector<SlowStdevTrendCalculator*> p_stdev_trend_indicator_vec_;
  std::vector<SimpleTrend*> p_simple_trend_indicator_vec_;
  std::vector<RecentSimpleVolumeMeasure*> p_volume_indicator_vec_;
  unsigned int num_products_;
  std::vector<bool> vol_updated_vec_;
  std::vector<bool> stdev_updated_vec_;
  std::vector<bool> trend_updated_vec_;
  bool is_updated_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DynamicWeightPortTrend* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const std::vector<const char*>& _tokens_,
                                                   PriceType_t _basepx_pxtype_);

  static DynamicWeightPortTrend* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                   const std::string& t_portfolio_descriptor_shortcode_,
                                                   const std::string& _stdev_vol_, const double _indicator_duration_,
                                                   const double _trend_duration_, const PriceType_t _price_type_);

 protected:
  DynamicWeightPortTrend(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                         const std::string& concise_indicator_description_,
                         const std::string& t_portfolio_descriptor_shortcode_, const std::string& _stdev_vol_,
                         const double _indicator_duration_, const double _trend_duration_,
                         const PriceType_t& _price_type_);

 public:
  ~DynamicWeightPortTrend() {}
  void OnVolumeUpdate(const unsigned int _indicator_index_, const double _new_value_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                         const double& _new_value_nochange_, const double& _new_value_increase_) {}
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  void OnPortfolioPriceChange(double _new_price_) {}
  void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  static std::string VarName() { return "DynamicWeightPortTrend"; }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  bool is_ready() { return is_ready_; }

 protected:
  bool AreAllReady() {
    for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
      if (is_ready_vec_[i] == false) return false;
    }
    return true;
  }
  bool AreAllUpdated() {
    for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
      if (!vol_updated_vec_[i] || !stdev_updated_vec_[i] || !trend_updated_vec_[i]) return false;
    }
    return true;
  }
};
}
