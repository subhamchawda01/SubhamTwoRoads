/**
    \file Indicators/spread_pricing.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_SPREAD_PRICING_H
#define BASE_INDICATORS_SPREAD_PRICING_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/retail_data_defines.hpp"
#include "dvccode/LiveSources/retail_trading_listener.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

/// Indicator that takes two SecurityMarketView objects dep, indep and
/// computes roughly OnlineLRCoeff ( dep, indep ) * indep_price_trend - dep_price_trend
/// For the computation of OnlineLRCoeff ( dep_price_changes, indep_price_changes ) it uses the mean zero math
/// Sum ( dep * indep ) / Sum ( indep * indep )
class SpreadPricing : public CommonIndicator {
 protected:
  SecurityMarketView &first_leg_market_view_;
  SecurityMarketView &second_leg_market_view_;

  double current_leg1_price_;
  double current_leg1_bestbid_price_;
  int current_leg1_bestbid_size_;
  double current_leg1_bestask_price_;
  int current_leg1_bestask_size_;

  double current_leg2_price_;
  double current_leg2_bestbid_price_;
  int current_leg2_bestbid_size_;
  double current_leg2_bestask_price_;
  int current_leg2_bestask_size_;

  double spread_bestbid_price_;
  int spread_max_bid_size_;
  double spread_bestask_price_;
  int spread_max_ask_size_;

  double size_factor_;

  double size_threshold_;
  double price_threshold_;
  double spread_factor_;

  int max_size_;

  PriceType_t price_type_;

  std::vector<RetailTradingListener *> retail_offer_listeners_;

  HFSAT::CDef::RetailOffer last_retail_offer_;

  unsigned int last_retail_update_msecs_;

  int leg1_position_;
  int leg1_max_position_;
  int leg2_position_;
  int leg2_max_position_;

  const std::string spread_secname_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static SpreadPricing *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                          const std::vector<const char *> &_tokens_, PriceType_t _basepx_pxtype_);

  static SpreadPricing *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                          SecurityMarketView &_dep_market_view_,
                                          SecurityMarketView &_indep_market_view_, PriceType_t _price_type_);

 protected:
  SpreadPricing(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &concise_indicator_description_,
                SecurityMarketView &_dep_market_view_, SecurityMarketView &_indep_market_view_,
                PriceType_t _price_type_);

 public:
  ~SpreadPricing() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string &r_dep_shortcode_, const std::vector<const char *> &tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  static std::string VarName() { return "SpreadPricing"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
  void RefreshSizeFactor();
};
}

#endif  // BASE_INDICATORS_SPREAD_PRICING_PAIR_H
