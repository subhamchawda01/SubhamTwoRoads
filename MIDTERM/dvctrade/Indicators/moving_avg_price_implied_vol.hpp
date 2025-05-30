/**
    \file Indicators/exponential_moving_average.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_MOVING_AVG_PRICE_IMPLIED_VOL_H
#define BASE_INDICATORS_MOVING_AVG_PRICE_IMPLIED_VOL_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/exponential_moving_average.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class MovingAvgPriceImpliedVol : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const SecurityMarketView& option_market_view_;
  const SecurityMarketView& future_market_view_;

  int price_history_secs_;
  const PriceType_t price_type_;

  // computational variable

  double last_implied_vol_recorded_;
  double average_option_price_;
  double average_future_price_;

  bool option_interrupted_;
  bool future_interrupted_;

  ExponentialMovingAverage* option_exponential_moving_average_;
  ExponentialMovingAverage* future_exponential_moving_average_;
  OptionObject* option_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static MovingAvgPriceImpliedVol* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const std::vector<const char*>& _tokens_,
                                                     PriceType_t _basepx_pxtype_);

  static MovingAvgPriceImpliedVol* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const SecurityMarketView& _indep_market_view_,
                                                     double _fractional_seconds_, PriceType_t _price_type_);

 protected:
  MovingAvgPriceImpliedVol(DebugLogger& _dbglogger_, const Watch& _watch_,
                           const std::string& concise_indicator_description_,
                           const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                           PriceType_t _price_type_);

 public:
  ~MovingAvgPriceImpliedVol() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if (!is_ready_) {
      if ((future_market_view_.is_ready_complex(2)) && (option_market_view_.is_ready_complex(2))) {
        is_ready_ = true;
      }
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}


  // functions
  static std::string VarName() { return "MovingAvgPriceImpliedVol"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_MOVING_AVG_PRICE_IMPLIED_VOL_H
