/**
    \file Indicators/mult_mkt_complex_price_topoff.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_MULT_MKT_COMPLEX_PRICE_TOPOFF_H
#define BASE_INDICATORS_MULT_MKT_COMPLEX_PRICE_TOPOFF_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/stdev_ratio_calculator.hpp"

namespace HFSAT {

/// similar to MultMktComplexPrice but ignores top level
class MultMktComplexPriceTopOff : public CommonIndicator, public StdevRatioListener {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;
  PriceType_t price_type_;
  unsigned int num_levels_;
  double decay_factor_;

  // computational variables
  std::vector<double> decay_vector_;
  double current_dep_stdev_ratio_;
  int last_decay_factor_update_msecs_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static MultMktComplexPriceTopOff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      const std::vector<const char*>& _tokens_,
                                                      PriceType_t _basepx_pxtype_);

  static MultMktComplexPriceTopOff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                      SecurityMarketView& _indep_market_view_,
                                                      unsigned int _num_levels_, double _decay_factor_,
                                                      double stdev_duration, PriceType_t _price_type_);

 protected:
  MultMktComplexPriceTopOff(DebugLogger& _dbglogger_, const Watch& _watch_,
                            const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                            unsigned int _num_levels_, double _decay_factor_, double stdev_duration,
                            PriceType_t _price_type_);

 public:
  ~MultMktComplexPriceTopOff() {}

  void OnStdevRatioUpdate(const unsigned int index_to_send, const double& r_new_scaled_volume_value);

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "MultMktComplexPriceTopOff"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  /// This is important here since if the dependant's basepx has changed
  /// and the shortcode of indep and dep of the model is the same,
  /// then it would be better for the model to start comparing the target price against the current dependant basepx
};
}

#endif  // BASE_INDICATORS_MULT_MKT_COMPLEX_PRICE_TOPOFF_H
