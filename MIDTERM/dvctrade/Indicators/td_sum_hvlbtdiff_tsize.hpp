/**
    \file Indicators/td_sum_hvlbtdiff_tsize.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TD_SUM_HVLBTDIFF_TSIZE_H
#define BASE_INDICATORS_TD_SUM_HVLBTDIFF_TSIZE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Indicator that returns TimeDecayedTradeInfoManager::sumlbtdiffsz_ or Sum ( last_book_tdiff_ * size_traded_ )
class TDSumHVLBTDiffTSize : public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;
  TimeDecayedTradeInfoManager& time_decayed_trade_info_manager_;

 protected:
  TDSumHVLBTDiffTSize(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                      SecurityMarketView& _indep_market_view_, double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TDSumHVLBTDiffTSize* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static TDSumHVLBTDiffTSize* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                                PriceType_t _basepx_pxtype_);

  ~TDSumHVLBTDiffTSize() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "TDSumHVLBTDiffTSize"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_TD_SUM_HVLBTDIFF_TSIZE_H
