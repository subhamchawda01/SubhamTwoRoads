/**
    \file Indicators/convex_trend_port.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/simple_trend_port.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class ConvexTrendPort : public IndicatorListener, public CommonIndicator {
 protected:
  double st_trend_value_;
  double lt_trend_value_;

  SimpleTrendPort* p_st_indicator_;
  SimpleTrendPort* p_lt_indicator_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static ConvexTrendPort* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static ConvexTrendPort* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::string& _portfolio_descriptor_shortcode_,
                                            double _fractional_st_seconds_, double _fractional_lt_seconds_,
                                            PriceType_t _price_type_);

 protected:
  ConvexTrendPort(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                  const std::string& _portfolio_descriptor_shortcode_, double _fractional_st_seconds_,
                  double _fractional_lt_seconds_, PriceType_t _price_type_);

 public:
  ~ConvexTrendPort() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
  }  ///< from CommonIndicator::SecurityMarketViewChangeListener

  inline void OnPortfolioPriceChange(double _new_price_) {}  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    if (p_st_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_st_indicator_);
    }

    if (p_lt_indicator_ != NULL) {
      market_update_manager_.AddMarketDataInterruptedListener(p_lt_indicator_);
    }
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // functions
  static std::string VarName() { return "ConvexTrendPort"; }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void WhyNotReady();

 protected:
};
}
