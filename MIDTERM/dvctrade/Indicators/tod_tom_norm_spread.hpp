/**
    \file Indicators/tod_tom_norm_spread.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
       Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TOD_TOM_NORM_SPREAD_H
#define BASE_INDICATORS_TOD_TOM_NORM_SPREAD_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

class TodTomNormSpread : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& todtom_market_view_;
  const SecurityMarketView& tod_market_view_;
  const PriceType_t price_type_;

  int tom_term_;

  double current_todtom_price_;
  double current_tod_price_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TodTomNormSpread* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static TodTomNormSpread* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const SecurityMarketView& _todtom_market_view_,
                                             const SecurityMarketView& _tom_market_view_,
                                             const SecurityMarketView& _tod_market_view_, PriceType_t _price_type_);

 protected:
  TodTomNormSpread(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   const SecurityMarketView& _todtom_market_view_, const SecurityMarketView& _tom_market_view_,
                   const SecurityMarketView& _tod_market_view_, PriceType_t _price_type_);

 public:
  ~TodTomNormSpread() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }
  // functions
  static std::string VarName() { return "TodTomNormSpread"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_TOD_TOM_NORM_SPREAD_H
