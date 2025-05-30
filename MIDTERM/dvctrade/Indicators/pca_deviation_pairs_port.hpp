/**
    \file Indicators/pca_deviation_pairs_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_PCA_DEVIATION_PAIRS_PORT_H
#define BASE_INDICATORS_PCA_DEVIATION_PAIRS_PORT_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"

namespace HFSAT {

/// Indicator takes the given portfolio to compute an aggregate price through PCAPrice
/// It gets the lr_coeff_ from OfflineReturnsLRDB of _portfolio_descriptor_shortcode_ -> dep_
/// From PCAPrice it computes the PCA_DEVIATION_SCORE of a shortcode with respect to the porfolio
///  this is a part of or otherwise
/// PCAPortPrice returns Sum ( over i ) { eigen_i * ( simple_trend_i / stdev_i ) } / NF
/// Here we compute prjected trend as this indep_trend_ * NF
/// Then we project it by multiplying eigen_dep
/// Finally to compare ahgainst simple_trend ( dep ) and not simple_trend(dep)/stdev_dep
/// we need to multiply the LHS with stdev_dep. Hence final math is
/// ( indep_trend_ * NF * eigen_dep * stdev_dep ) - simple_trend ( dep )
class PCADeviationPairsPort : public IndicatorListener, public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  PCAPortPrice& indep_pca_price_;

  double dep_price_trend_;
  double indep_price_trend_;

  double current_projected_trend_;
  std::vector<double> eigen_components_;
  int dep_index_in_portfolio_;
  const std::vector<double>& stdev_each_constituent_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static PCADeviationPairsPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<const char*>& _tokens_,
                                                  PriceType_t _basepx_pxtype_);

  static PCADeviationPairsPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  SecurityMarketView& _dep_market_view_,
                                                  const std::string& _portfolio_descriptor_shortcode_,
                                                  const double _fractional_seconds_,
                                                  const std::vector<double>& eigen_component_,
                                                  const PriceType_t _price_type_);

 protected:
  PCADeviationPairsPort(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_, SecurityMarketView& _dep_market_view_,
                        const std::string& _portfolio_descriptor_shortcode_, const double _fractional_seconds_,
                        const std::vector<double>& eigen_component_, const PriceType_t _price_type_);

 public:
  ~PCADeviationPairsPort() {}

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};

  void WhyNotReady();

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(&indep_pca_price_);
  }
  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions
  /// Used in ModelCreator to see which variable is in the model file
  static std::string VarName() { return "PCADeviationPairsPort"; }

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_PCA_DEVIATION_PAIRS_PORT_H
