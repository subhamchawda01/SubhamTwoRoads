/**
    \file Indicators/mult_mid_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_MULT_MID_PRICE_H
#define BASE_INDICATORS_MULT_MID_PRICE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/book_info_manager.hpp"

namespace HFSAT {

/// Indicator that returns EWMA bid price + EWMA ask price / 2
class MultMidPrice : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;
  PriceType_t price_type_;
  unsigned int num_levels_;
  double decay_factor_;
  double stdev_duration_;

  // computational variables
  std::vector<double> decay_vector_;

  BookInfoManager& book_info_manager_;
  BookInfoManager::BookInfoStruct* book_info_struct_;

 public:
  // functions
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static MultMidPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static MultMidPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                         double _decay_factor_, PriceType_t _price_type_, double _stdev_duration_);

  MultMidPrice(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
               SecurityMarketView& _indep_market_view_, unsigned int _num_levels_, double _decay_factor_,
               PriceType_t _price_type_, double _stdev_duration_);

  ~MultMidPrice() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "MultMidPrice"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
#endif  // BASE_INDICATORS_MULT_MKT_PRICE_H
