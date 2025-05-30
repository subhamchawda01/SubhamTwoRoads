/*i*
    \file Indicators/moving_avg_trade_size.hpp

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
#include <deque>

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class BigTradeVolumeVersion : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  int window_msecs_;
  int last_buysell_;  // 0:buy side,1:sell side
  int current_int_trade_price_;
  int last_int_trade_price_;
  int last_bestbid_int_price_;
  int last_bestask_int_price_;

  double current_trade_size_;

  struct TradeElem {
    int time_msecs_;
    double size_;
    int buysell_;
    TradeElem(int _time_msecs_, double _size_, int _buysell_) : time_msecs_(_time_msecs_), size_(_size_), buysell_(_buysell_) {}
  };

  std::deque<TradeElem*> trades_queue_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static BigTradeVolumeVersion* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static BigTradeVolumeVersion* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                               SecurityMarketView& _indep_market_view_, int _window_msecs_ );

 protected:
  BigTradeVolumeVersion(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                     SecurityMarketView& _indep_market_view_, int _window_msecs_ );

 public:
  ~BigTradeVolumeVersion() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  inline double GetTradeSizeFactor() { return indicator_value_; }
  // functions
  static std::string VarName() { return "BigTradeVolumeVersion"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
