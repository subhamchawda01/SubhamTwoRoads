/**
    \file Indicators/diff_price_ebs.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

#define DECAY_START_MSECS 10

namespace HFSAT {

/// Indicator returns indep_market_view_::price_type_ - indep_market_view_::basepx_pxtype_
/// Also basepx_pxtype_ changes by set_basepx_pxtype ( ) call
class DiffPriceEBS : public CommonIndicator {
 protected:
  const SecurityMarketView& indep1_market_view_;
  const SecurityMarketView& indep2_market_view_;  // by convention 2nd security in indicator description is a EBS symbol
  unsigned int last_ebs_update_msecs_;
  PriceType_t price_type_;
  bool indep1_data_interrupted_;
  bool indep2_data_interrupted_;
  double decay_factor_;
  unsigned int ebs_security_id_;
  double current_indep1_price_;
  double current_indep2_price_;

 public:
  // functions

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static DiffPriceEBS* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static DiffPriceEBS* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         SecurityMarketView& _indep1_market_view_,
                                         SecurityMarketView& _indep2_market_view_, PriceType_t _price_type_);

  DiffPriceEBS(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
               SecurityMarketView& _indep1_market_view_, SecurityMarketView& _indep2_market_view_,
               PriceType_t _price_type_);

  ~DiffPriceEBS() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "DiffPriceEBS"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
};
}
