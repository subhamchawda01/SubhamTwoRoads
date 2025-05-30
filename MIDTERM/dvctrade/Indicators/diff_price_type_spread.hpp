/**
    \file Indicators/diff_price_type_spread.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_INDICATORS_DIFF_PRICE_TYPE_SPREAD_H
#define BASE_INDICATORS_DIFF_PRICE_TYPE_SPREAD_H

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/spread_market_view.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "baseinfra/MarketAdapter/shortcode_spread_market_view_map.hpp"

namespace HFSAT {

/// Indicator returns indep_market_view_::price_type_ - indep_market_view_::basepx_pxtype_
class DiffPriceTypeSpread : public CommonIndicator, public SpreadMarketViewListener {
 protected:
  SpreadMarketView& spread_market_view_;

  PriceType_t base_price_type_;
  PriceType_t price_type_;

  std::vector<bool> data_interrupted_vec_;

 public:
  // functions

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DiffPriceTypeSpread* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static DiffPriceTypeSpread* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                SpreadMarketView& _spread_market_view_, PriceType_t _price_type_,
                                                PriceType_t _basepx_pxtype_);

  DiffPriceTypeSpread(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      SpreadMarketView& _spread_market_view_, PriceType_t _price_type_, PriceType_t _base_pxtype_);

  ~DiffPriceTypeSpread() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "DiffPriceTypeSpread"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void OnSpreadMarketViewUpdate(const SpreadMarketView& _spread_market_view_);

 protected:
};
}

#endif  // BASE_INDICATORS_DIFF_PRICE_TYPE_SPREAD_H
