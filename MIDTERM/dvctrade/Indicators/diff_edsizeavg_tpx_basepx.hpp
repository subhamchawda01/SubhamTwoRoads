/**
    \file Indicators/diff_trsizeavg_tpx_basepx.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_DIFF_EDSIZEAVG_TPX_BASEPX_H
#define BASE_INDICATORS_DIFF_EDSIZEAVG_TPX_BASEPX_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/event_decayed_trade_info_manager.hpp"

namespace HFSAT {

class DiffEDSizeAvgTPxBasepx : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;
  PriceType_t price_type_;
  EventDecayedTradeInfoManager& event_decayed_trade_info_manager_;

 protected:
  DiffEDSizeAvgTPxBasepx(DebugLogger& _dbglogger_, const Watch& _watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                         int _num_trades_halflife_, PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DiffEDSizeAvgTPxBasepx* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const std::vector<const char*>& _tokens_,
                                                   PriceType_t _basepx_pxtype_);

  static DiffEDSizeAvgTPxBasepx* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   SecurityMarketView& _indep_market_view_, int _num_trades_halflife_,
                                                   PriceType_t _price_type_);

  ~DiffEDSizeAvgTPxBasepx() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "DiffEDSizeAvgTPxBasepx"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_DIFF_EDAVG_TPX_BASEPX_H
