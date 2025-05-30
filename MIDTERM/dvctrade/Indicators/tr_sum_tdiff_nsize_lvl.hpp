/**
    \file Indicators/tr_sum_tdiff_nsize_lvl.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TR_SUM_TDIFF_NSIZE_LVL_H
#define BASE_INDICATORS_TR_SUM_TDIFF_NSIZE_LVL_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"

namespace HFSAT {

class TRSumTDiffNSizeLvl : public CommonIndicator {
 protected:
  const SecurityMarketView& indep_market_view_;
  TradeDecayedTradeInfoManager& trade_decayed_trade_info_manager_;

 protected:
  TRSumTDiffNSizeLvl(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                     SecurityMarketView& _indep_market_view_, int _num_trades_halflife_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TRSumTDiffNSizeLvl* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static TRSumTDiffNSizeLvl* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               SecurityMarketView& _indep_market_view_, int _num_trades_halflife_,
                                               PriceType_t _basepx_pxtype_);

  ~TRSumTDiffNSizeLvl() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "TRSumTDiffNSizeLvl"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_TR_SUM_TDIFF_NSIZE_LVL_H
