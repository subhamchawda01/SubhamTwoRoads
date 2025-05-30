/**
   \file Indicators/moore_penrose_fx_sf.hpp

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
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

// Moore Penrose type of indicator for the approximate spot future pricing where 3 contracts are there,
// as in case of TOD, TOM, Si
class MoorePenroseFxSf : public CommonIndicator {
 protected:
  const SecurityMarketView& dep_market_view_;
  PriceType_t price_type_;
  int dte_;  // days to expiry
  SecurityMarketView* tod_smv_;
  SecurityMarketView* tom_smv_;
  SecurityMarketView* fut_smv_;
  double tod_coeff_;
  double tom_coeff_;
  double fut_coeff_;
  double last_recorded_tod_price_;
  double last_recorded_tom_price_;
  double last_recorded_fut_price_;
  std::vector<bool> is_data_interrupted_vec_;

  void ResetIndicatorValue();

 public:
  // functions

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static MoorePenroseFxSf* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static MoorePenroseFxSf* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             SecurityMarketView& _indep_market_view_,
                                             std::string moore_penrose_filename, PriceType_t _price_type_);

  MoorePenroseFxSf(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   SecurityMarketView& _indep_market_view_, std::string moore_penrose_filename,
                   PriceType_t _price_type_);

  ~MoorePenroseFxSf() {}

  void Initialize(std::string moore_penrose_filename);
  std::vector<std::string> Get_shrtcodes_from_spread(std::string spread_shrtcode);
  // listener interface
  //  void  OnTimePeriodUpdate ( const int num_pages_to_add_ ) ;
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "MoorePenroseFxSf"; }
  void Get_spread_shortcode_mapping();
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
