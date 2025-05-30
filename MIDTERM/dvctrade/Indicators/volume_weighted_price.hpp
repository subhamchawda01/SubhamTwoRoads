/**
    \file Indicators/volume_weighted_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"

// to make life easier, lets compute trade decayed vwap !
// we borrow, with x no of trades as half life (and exponential decay),
// price*sz  and sz variables from trade_decayed_trade_info_manager
// all we do here is to compute price*sz/sz

//  f(vwap(x1), vwap(x2)) Vs vwap(f(x1, x2))
// all we know is +ve convexity 2 > 1  and 1 > 2 otherwise !

namespace HFSAT {

class VolumeWeightedPrice : public CommonIndicator {
 protected:
  // we create an instance of trade_decayed_trade_info_manager
  // we then set compute_sumpxsz_ and compute_sumsz_ to true
  // these two get updated on every trade, that is the rate
  // at which we get these values and fwd the ratio to
  // this listener

  // variables
  const SecurityMarketView& sec_market_view_;
  TradeDecayedTradeInfoManager& trade_decayed_trade_info_manager_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static VolumeWeightedPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static VolumeWeightedPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                SecurityMarketView& _sec_market_view_, int t_halflife_trades_,
                                                PriceType_t _price_type_);

 protected:
  VolumeWeightedPrice(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      SecurityMarketView& _sec_market_view_, int t_halflife_trades_);

 public:
  ~VolumeWeightedPrice() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }
  // functions
  static std::string VarName() { return "VolumeWeightedPrice"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
