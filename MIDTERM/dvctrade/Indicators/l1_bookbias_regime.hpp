/**
   \file Indicators/l1_bookbias_regime.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

/// regime based spread and abs( mkt - mid ). Three regimes suppported
/// 1 - spread > 1 tick
/// 2 - | bid - mkt | or | ask - mkt | < threshold
/// 3 - otherwise
class L1BookbiasRegime : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& dep_market_view_;
  double tolerance_;  // to induce hysterisis in indicator value if desired
  double threshold_;  // threshold value ( see above ) -- note it is in absolute value not %tage of tick.

 protected:
  L1BookbiasRegime(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _concise_indicator_description_,
                   const SecurityMarketView& _dep_market_view_, double _thresh_, double _tolerance_,
                   PriceType_t _basepx_pxtype_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static L1BookbiasRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static L1BookbiasRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const SecurityMarketView& _dep_market_view_, double _thresh_,
                                             double _tolerance_, PriceType_t _basepx_pxtype_);

  ~L1BookbiasRegime() {}

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

  static std::string VarName() { return "L1BookbiasRegime"; }

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(this);
  }

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  /// following are not important for this indicator
  void OnPortfolioPriceChange(double _new_price_){};
  void OnPortfolioPriceReset(double t_new_value_, double t_old_price_, unsigned int is_data_interrupted_){};

 protected:
  void InitializeValues();
};
}
