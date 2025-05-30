/**
    \file Indicators/tr_sum_ttype.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TR_AVG_TDIFF_SQRT_TSIZE_VOLFACTOR_H
#define BASE_INDICATORS_TR_AVG_TDIFF_SQRT_TSIZE_VOLFACTOR_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/recent_simple_trades_measure.hpp"
#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"

namespace HFSAT {

/// Indicator returning trade decayed EW Sum of trade type ...
/// sort of a very shrot term indicator of which way trades are happening
/// perhaps we should ignore 1-sized trades ?
class TRAvgTDiffSqrtTSizeVolfactor : public CommonIndicator, public IndicatorListener {
 protected:
  const SecurityMarketView& indep_market_view_;
  double volfactor_percentile_;
  const PriceType_t price_type_;
  double decay_page_factor_;
  double cutoff_trades_;
  double sumtdiffsqrtsz_;
  double sumsqrtsz_;
  double trades_measure_;

  RecentSimpleTradesMeasure* trades_measure_indc_;

 protected:
  TRAvgTDiffSqrtTSizeVolfactor(DebugLogger& _dbglogger_, const Watch& _watch_,
                               const std::string& concise_indicator_description_,
                               SecurityMarketView& _indep_market_view_, int _num_trades_halflife_,
                               double _volfactor_percentile_, PriceType_t _basepx_pxtype_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TRAvgTDiffSqrtTSizeVolfactor* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                         const std::vector<const char*>& _tokens_,
                                                         PriceType_t _basepx_pxtype_);

  static TRAvgTDiffSqrtTSizeVolfactor* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         int _num_trades_halflife_, double _volfactor_percentile_,
                                                         PriceType_t _basepx_pxtype_);

  ~TRAvgTDiffSqrtTSizeVolfactor() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "TRAvgTDiffSqrtTSizeVolfactor"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}

#endif  // BASE_INDICATORS_TR_SUM_TTYPE_H
