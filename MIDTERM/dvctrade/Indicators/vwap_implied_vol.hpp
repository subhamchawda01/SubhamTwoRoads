/**
   \file Indicators/vwap_implied_vol.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/volume_weighted_price.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"

// we received update vwap prices of future and option on
// trades in either of instrument
// we updated the iv then forward?

namespace HFSAT {

class VWAPImpliedVol : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  SecurityMarketView& option_market_view_;
  SecurityMarketView& future_market_view_;

  VolumeWeightedPrice* fut_vwap_indicator_;
  VolumeWeightedPrice* opt_vwap_indicator_;

  double current_options_vwap_;
  double current_fut_vwap_;

  OptionObject* option_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static VWAPImpliedVol* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static VWAPImpliedVol* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           SecurityMarketView& _indep_market_view_, int t_halflife_trades_,
                                           PriceType_t _price_type_);

 protected:
  VWAPImpliedVol(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 SecurityMarketView& _indep_market_view_, int t_halflife_trades_, PriceType_t _price_type_);

 public:
  ~VWAPImpliedVol() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "VWAPImpliedVol"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

 protected:
  void InitializeValues();
};
}
