/**
    \file Indicators/volume_traded.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_VOLUME_TRADED_H
#define BASE_INDICATORS_VOLUME_TRADED_H

#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

/// Indicator that returns TimeDecayedTradeInfoManager::sumtdifffsz_ or Sum ( tradepx_mktpx_diff_ * trade_impact_ *
/// size_traded_ )
class VolumeTraded : public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;

 protected:
  VolumeTraded(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
               SecurityMarketView& _indep_market_view_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static VolumeTraded* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static VolumeTraded* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         SecurityMarketView& _indep_market_view_);

  ~VolumeTraded() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "VolumeTraded"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_VOLUME_TRADED_H
