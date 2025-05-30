/**
    \file Indicators/convex_positioning.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_INDICATORS_CONVEX_POSITIONING_H
#define BASE_INDICATORS_CONVEX_POSITIONING_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/positioning.hpp"
#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Indicator returning trade decayed EW Sum of trade type ...
/// sort of a very shrot term indicator of which way trades are happening
/// perhaps we should ignore 1-sized trades ?
class ConvexPositioning : public IndicatorListener, public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;

  Positioning* p_st_indicator_;
  Positioning* p_lt_indicator_;

  double st_positioning_value_;
  double lt_positioning_value_;

 protected:
  ConvexPositioning(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                    SecurityMarketView& _indep_market_view_, int _bucket_size_short_, int _bucket_size_long_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ConvexPositioning* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static ConvexPositioning* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              SecurityMarketView& _indep_market_view_, int _bucket_size_short_,
                                              int _bucket_size_long_, PriceType_t _basepx_pxtype_);

  ~ConvexPositioning() {}

  // listener interface
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {}
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  // functions
  static std::string VarName() { return "ConvexPositioning"; }
};
}

#endif  // BASE_INDICATORS_CONVEX_POSITIONING_H
