/**
    \file Indicators/book_size_diff.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/book_info_manager.hpp"

namespace HFSAT {

/// Book Pressure Indicator
/// returning ewma_bid_size - ewma_ask-size
/// the decay-factor being pow ( sqrt ( decay_factor ), 2 * ( number of ticks from mid ) )
/// or pow ( decay_factor, number of tikcs from mid )
class BookSizeDiff : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  unsigned int num_levels_;
  double decay_factor_;
  double stdev_duration_;

  // computational variables
  std::vector<double> decay_vector_;

  BookInfoManager& book_info_manager_;
  BookInfoManager::BookInfoStruct* book_info_struct_;

  // functions
 protected:
  BookSizeDiff(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
               SecurityMarketView& _indep_market_view_, unsigned int _num_levels_, double _decay_factor_,
               double _stdev_duration_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static BookSizeDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static BookSizeDiff* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                         double _decay_factor_, PriceType_t _basepx_pxtype_, double _stdev_duration_);

 public:
  ~BookSizeDiff() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "BookSizeDiff"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
