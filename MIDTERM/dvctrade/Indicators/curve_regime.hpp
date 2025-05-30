/**
   \file Indicators/curve_regime.hpp

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

/**
 * Takes 3 poducts in the argument, say P1,P2,P3
 * P1 > P2 > P3 => Regime 1
 * P1 > P2 < P3 => Regime 2
 * P1 < P2 > P3 => Regime 3
 * P1 < P2 < P3 => Regime 4
 */
class CurveRegime : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& prod1_market_view_;
  const SecurityMarketView& prod2_market_view_;
  const SecurityMarketView& prod3_market_view_;

  double prod1_px_;
  double prod2_px_;
  double prod3_px_;

  bool prod1_updated_;
  bool prod2_updated_;
  bool prod3_updated_;

  PriceType_t price_type_;

 protected:
  CurveRegime(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _concise_indicator_description_,
              const SecurityMarketView& _prod1_market_view_, const SecurityMarketView& _prod2_market_view_,
              const SecurityMarketView& _prod3_market_view_, PriceType_t _basepx_pxtype_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static CurveRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static CurveRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const SecurityMarketView& _prod1_market_view_,
                                        const SecurityMarketView& _prod2_market_view_,
                                        const SecurityMarketView& _prod3_market_view_, PriceType_t _basepx_pxtype_);

  ~CurveRegime() {}

  static std::string VarName() { return "CurveRegime"; }

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
