#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/dynamic_weight_port_trend.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/price_portfolio.hpp"

namespace HFSAT {

class StudPriceTrendDiffDynamicWeightPort : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView* dep_market_view_;
  DynamicWeightPortTrend* indep_portfolio_trend_;
  SimpleTrend* dep_trend_;
  double ratio_duration_;
  double trend_duration_;
  int lrdb_sign_;
  double trend_dep_;
  double trend_indep_;
  bool stdev_dep_updated_;
  bool stdev_indep_updated_;
  bool trend_indep_updated_;
  bool trend_dep_updated_;
  bool dep_interrupted_;
  bool indep_interrupted_;
  double stdev_dep_;
  double stdev_indep_;
  bool is_dep_portfolio_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static StudPriceTrendDiffDynamicWeightPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                const std::vector<const char*>& _tokens_,
                                                                PriceType_t _basepx_pxtype_);

  static StudPriceTrendDiffDynamicWeightPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                std::string _shortcode_,
                                                                const std::string& _portfolio_descriptor_shortcode_,
                                                                const std::string& _stdev_vol_, double _ratio_duration_,
                                                                double _trend_duration_, PriceType_t _price_type_);

 protected:
  StudPriceTrendDiffDynamicWeightPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                                      const std::string& concise_indicator_description_, std::string _shortcode_,
                                      const std::string& _portfolio_descriptor_shortcode_,
                                      const std::string& _stdev_vol_, double _ratio_duration_, double _trend_duration_,
                                      PriceType_t _price_type_);

 public:
  ~StudPriceTrendDiffDynamicWeightPort() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  // functions
  static std::string VarName() { return "StudPriceTrendDiffDynamicWeightPort"; }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) override;
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) override {
    return;
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
