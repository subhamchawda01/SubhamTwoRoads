/**
    \file Indicators/future_to_spot_pricing.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/FuturesUtils/SpotFutureIndicatorUtils.hpp"

namespace HFSAT {

class IndexFutureToSpotPricing : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  SecurityMarketView* dep_market_view_;
  CommonIndicator* synthetic_index_;
  PriceType_t price_type_;

  int days_to_expiry_;
  int tradingdate_;

  bool dep_interrupted_;
  bool indep_interrupted_;

  double current_dep_price_;
  double current_synthetic_price_;

  double moving_average_diff_;
  double last_diff_recorded_;
  double current_diff_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static IndexFutureToSpotPricing* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     const std::vector<const char*>& _tokens_,
                                                     PriceType_t _basepx_pxtype_);

  static IndexFutureToSpotPricing* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                     SecurityMarketView* _dep_market_view_, double _history_secs_,
                                                     PriceType_t _price_type_);

 protected:
  IndexFutureToSpotPricing(DebugLogger& _dbglogger_, const Watch& _watch_,
                           const std::string& concise_indicator_description_, SecurityMarketView* _dep_market_view_,
                           double _history_secs_, PriceType_t _price_type_);
  void ComputePriceDiff();

 public:
  ~IndexFutureToSpotPricing() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                         const double& _new_value_nochange_, const double& _new_value_increase_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}  ///< from CommonIndicator::PortfolioPriceChangeListener
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }

  static std::string VarName() { return "IndexFutureToSpotPricing"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
