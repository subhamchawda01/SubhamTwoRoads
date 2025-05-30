/**
    \file Indicators/trade_book_adjusted_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_TRADE_BOOK_ADJUSTED_PRICE_H
#define BASE_INDICATORS_TRADE_BOOK_ADJUSTED_PRICE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"

namespace HFSAT {

/// price indicator adjusted based on the size
///
///
///
class TradeBookAdjustedPrice : public CommonIndicator, public IndicatorListener {
 protected:
  // variables

  const SecurityMarketView& indep_market_view_;
  PriceType_t price_type_;
  std::vector<bool> is_ready_vec_;
  std::vector<double> prev_value_vec_;

  double book_weight_;
  double trade_weight_;
  std::vector<CommonIndicator*> sub_indicator_vec_;

  // computational variables

  // functions
  TradeBookAdjustedPrice(DebugLogger& _dbglogger_, const Watch& _watch_,
                         const std::string& _concise_indicator_description_,
                         const SecurityMarketView& _indep_market_view_, double t_book_weight_, double t_trade_weight_,
                         PriceType_t _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static TradeBookAdjustedPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const std::vector<const char*>& _tokens_,
                                                   PriceType_t _basepx_pxtype_);

  static TradeBookAdjustedPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const SecurityMarketView& _indep_market_view_, double t_book_weight_,
                                                   double t_trade_weight_, PriceType_t _price_type_);

  ~TradeBookAdjustedPrice() {}

  // indicator listener interface
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  // dont have anything currently to add here
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  // security_view listener interface
  // there is nothing to do here with these updates, these updates are used by subindicators, subindicators calls
  // onindicator update function of this class
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < sub_indicator_vec_.size(); i++) {
      if (sub_indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(sub_indicator_vec_[i]);
      }
    }
  }

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "TradeBookAdjustedPrice"; }

 protected:
  void InitializeValues(unsigned int);
  bool AreAllReady();
};
}

#endif  // BASE_INDICATORS_TRADE_BOOK_ADJUSTED_PRICE_H
