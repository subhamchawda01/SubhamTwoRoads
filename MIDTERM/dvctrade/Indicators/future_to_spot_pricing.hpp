/**
    \file Indicators/future_to_spot_pricing.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/FuturesUtils/SpotFutureIndicatorUtils.hpp"

namespace HFSAT {

class FutureToSpotPricing : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& dep_market_view_;
  SecurityMarketView& indep_market_view_;

  PriceType_t price_type_;
  PriceType_t dep_price_type_;
  // computational variables

  double diff_daily_interest_rates_;
  int days_to_expiry_;

  int tradingdate_;

  double current_dep_price_;
  double current_indep_price_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static FutureToSpotPricing* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static FutureToSpotPricing* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                SecurityMarketView& _dep_market_view_,
                                                SecurityMarketView& _indep_market_view_, PriceType_t _price_type_,
                                                PriceType_t _dep_price_type_);

 protected:
  FutureToSpotPricing(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      SecurityMarketView& _dep_market_view_, SecurityMarketView& _indep_market_view_,
                      PriceType_t _price_type_, PriceType_t _dep_price_type_);

 public:
  ~FutureToSpotPricing() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
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

  static std::string VarName() { return "FutureToSpotPricing"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
