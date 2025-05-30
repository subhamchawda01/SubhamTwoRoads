/**
    \file Indicators/volume_pca_deviation_pairs_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/recent_simple_volume_listener.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

/// Indicator takes the depshortcode and the portfolio to use for princomp
/// Calculates eigen1_move = Sum (over i) { ( (simple_trend_i / stdev_i) * eigen1_i * RiskVolume_i^2 ) / Sum (
/// eigen1_i^2 * RV_i^2 ) }
/// Returns eigen1_move*eigen1_dep*stdev_dep - simple_trend_dep
class VolumePCADeviationPairsPort : public IndicatorListener,
                                    public CommonIndicator,
                                    public RecentSimpleVolumeListener {
 protected:
  // variables
  const SecurityMarketView &dep_market_view_;

  int dep_index_in_portfolio_;
  std::vector<SecurityMarketView *> indep_market_view_vec_;  ///< right now only used in WhyNotReady
  const std::vector<double> eigen_components_;
  const std::vector<double> stdev_each_constituent_;
  std::vector<double> risk_volume_;      ///< vector of values that we use for weights on the curve fitting
  std::vector<double> risk_multiplier_;  ///< vector of values that we multiply by the observed volume or periodic
  /// volume to get risk_volume_
  std::vector<double> st_weight_vec_;  ///< vector of values that we multiply by the observed volume or periodic volume
  /// to get risk_volume_
  std::vector<double> trend_vec_;  ///< vector of trends ... to calc difference

  bool weighted_eigen_ready_;
  // functions

  std::vector<bool> is_ready_vec_;
  std::vector<bool> data_interrupted_vec_;
  bool recompute_indicator_value_;
  std::vector<CommonIndicator *> p_indep_indicator_vec_;

 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static VolumePCADeviationPairsPort *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                        const std::vector<const char *> &_tokens_,
                                                        PriceType_t _basepx_pxtype_);

  static VolumePCADeviationPairsPort *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                        SecurityMarketView &_dep_market_view_,
                                                        const std::string &_portfolio_descriptor_shortcode_,
                                                        const double _fractional_seconds_,
                                                        const PriceType_t _price_type_);

 protected:
  VolumePCADeviationPairsPort(DebugLogger &_dbglogger_, const Watch &_watch_,
                              const std::string &concise_indicator_description_, SecurityMarketView &_dep_market_view_,
                              const std::string &_portfolio_descriptor_shortcode_, const double _fractional_seconds_,
                              const std::vector<double> &t_eigen_components_, const PriceType_t _price_type_);

 public:
  ~VolumePCADeviationPairsPort() {}

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};

  void WhyNotReady();

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);
  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {}

  void OnVolumeUpdate(unsigned int t_index_, double _new_volume_value_);  ///< updates from RecentSimpleVolumeMeasure

  inline void SubscribeDataInterrupts(MarketUpdateManager &market_update_manager_) {
    for (auto i = 0u; i < p_indep_indicator_vec_.size(); i++) {
      if (p_indep_indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(p_indep_indicator_vec_[i]);
      }
    }
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (indep_market_view_vec_[i] != NULL) {
        if (indep_market_view_vec_[i]->security_id() == _security_id_) {
          data_interrupted_vec_[i] = true;
          data_interrupted_ = true;

          indicator_value_ = 0.0;
          trend_vec_[i] = 0.0;
          risk_volume_[i] = 0.0;
          recompute_indicator_value_ = true;

          NotifyIndicatorListeners(indicator_value_);

          break;
        }
      }
    }
  }

  inline void OnMarketDataResumed(const unsigned int _security_id_) {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (indep_market_view_vec_[i] != NULL) {
        if (indep_market_view_vec_[i]->security_id() == _security_id_) {
          data_interrupted_vec_[i] = false;
          data_interrupted_ = VectorUtils::LinearSearchValue(data_interrupted_vec_, true);
          break;
        }
      }
    }
  }

  inline bool AreAllReady() {
    return (VectorUtils::CheckAllForValue(is_ready_vec_, true) && dep_market_view_.is_ready_complex(2));
  }

  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "VolumePCADeviationPairsPort"; }

 protected:
  void InitializeValues(int _index_);
};
}
