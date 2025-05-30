/**
    \file Indicators/positioning.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_INDICATORS_POSITIONING_H
#define BASE_INDICATORS_POSITIONING_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Indicator returning trade decayed EW Sum of trade type ...
/// sort of a very shrot term indicator of which way trades are happening
/// perhaps we should ignore 1-sized trades ?
class Positioning : public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;
  int bucket_size_;
  double alpha_;

 protected:
  Positioning(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              SecurityMarketView& _indep_market_view_, int _bucket_size_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static Positioning* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static Positioning* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        SecurityMarketView& _indep_market_view_, int _bucket_size_,
                                        PriceType_t _basepx_pxtype_);

  ~Positioning() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};
  inline double GetPositioning() { return indicator_value_; }

  // functions
  static std::string VarName() { return "Positioning"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_POSITIONING_H
