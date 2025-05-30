/**
    \file Indicators/diff_tdsize_l1size_ratio.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_DIFF_TDSIZE_L1SIZE_RATIO_H
#define BASE_INDICATORS_DIFF_TDSIZE_L1SIZE_RATIO_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

class DiffTDSizeL1SizeRatio : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  // computional variables
  double moving_avg_l1_size_;

  double last_l1_size_recorded_;
  double current_l1_size_;

  TimeDecayedTradeInfoManager& time_decayed_trade_info_manager_;

 protected:
  DiffTDSizeL1SizeRatio(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                        double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DiffTDSizeL1SizeRatio* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<const char*>& _tokens_,
                                                  PriceType_t _basepx_pxtype_);

  static DiffTDSizeL1SizeRatio* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                                  PriceType_t _basepx_pxtype_);

  ~DiffTDSizeL1SizeRatio() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  void OnTimePeriodUpdate(const int _num_pages_to_add_);

  // functions
  static std::string VarName() { return "DiffTDSizeL1SizeRatio"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_DIFF_TDAVG_TPX_BASEPX_H
