/**
    \file ExecLogic/event_directional_agrressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_EVENT_DIRECTIONAL_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_EVENT_DIRECTIONAL_AGGRESSIVE_TRADING_H
#define SOURCE_EVENT_PRICE_FILE = "/spare/local/tradeinfo/event_source_price_file"
#include "dvctrade/ExecLogic/base_trading.hpp"

namespace HFSAT {

/// @brief Class to trade with a target price that is not dependant on top level sizes
///
/// Here we are trying to predit direction of trading. And compare the target price against the midprice.
class EventDirectionalAggressiveTrading : public virtual BaseTrading {
 protected:
  int evtrading_start_mfm_;
  std::vector<double> val_decay_vec_;
  std::string source_shortcode_;
  double source_shortcode_price_median_price_movement_;
  std::map<std::string, double> event_source_price_map_;
  bool scale_risk_based_on_source_;
  bool source_price_move_computed_;
  bool start_computing_source_price_;
  bool decrease_risk_;
  bool product_risk_decided_;
  bool min_risk_;
  double max_source_price_;
  double min_source_price_;
  double current_px_;
  const SecurityMarketView* source_shortcode_market_view_;
  SimplePriceType* source_price_indicator_;
  std::vector<EventLine> traded_events_of_the_day_;
  TimeDecayedTradeInfoManager* dep_decayed_trade_info_manager_;

 public:
  EventDirectionalAggressiveTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                    const SecurityMarketView& _dep_market_view_, SmartOrderManager& _order_manager_,
                                    const std::string& _paramfilename_, const bool _livetrading_,
                                    MulticastSenderSocket* _p_strategy_param_sender_socket_,
                                    EconomicEventsManager& t_economic_events_manager_,
                                    const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_,
                                    const int _runtime_id_,
                                    const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~EventDirectionalAggressiveTrading(){};

  static std::string StrategyName() {
    return "EventDirectionalAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  virtual void ProcessTimePeriodUpdate(const int num_pages_to_add_);

 protected:
  void TradingLogic();  ///< All the strategy based trade execution is written here
  double GetSourcePriceMedian(std::string event_string, std::string);
  void PrintFullStatus();

  void InititalizeSourcePriceMap(std::string source_shortcode_, double min_price_increment_) {
    // this map has the source price move for different events in ticks
    DBGLOG_TIME << "The source is: " << source_shortcode_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "The min price increment of the source is : " << min_price_increment_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME << "The event text : " << traded_events_of_the_day_[0].event_text_ << DBGLOG_ENDL_FLUSH;
    // for bond sources
    if (source_shortcode_ == std::string("ZN_0")) {
      event_source_price_map_["Retail_Sales_(MoM)_ZN_0"] = 9.1 * min_price_increment_;
      event_source_price_map_["CPI_(YoY)_ZN_0"] = 7.4 * min_price_increment_;
      event_source_price_map_["Consumer_Price_Index_(YoY)_ZN_0"] = 7.4 * min_price_increment_;
      event_source_price_map_["ISM_Manufacturing_PMI_ZN_0"] = 2.6 * min_price_increment_;
      event_source_price_map_["ISM_Non-Manufacturing_PMI_ZN_0"] = 3 * min_price_increment_;
      event_source_price_map_["Durable_Goods_Orders_ZN_0"] = 1.7 * min_price_increment_;
      event_source_price_map_["Gross_Domestic_Product_Annualized_ZN_0"] = 3.5 * min_price_increment_;
      event_source_price_map_["Nonfarm_Payrolls_ZN_0"] = 14 * min_price_increment_;
      event_source_price_map_["Fed_Interest_Rate_Decision_ZN_0"] = 9.5 * min_price_increment_;
    }
    // for equity sources
    else if (source_shortcode_ == std::string("ES_0")) {
      event_source_price_map_["Retail_Sales_(MoM)_USD_ES_0"] = 3.5 * min_price_increment_;
      event_source_price_map_["ISM_Manufacturing_PMI_ES_0"] = 2 * min_price_increment_;
      event_source_price_map_["ISM_Non-Manufacturing_PMI_ES_0"] = 2.27 * min_price_increment_;
      event_source_price_map_["Durable_Goods_Orders_ES_0"] = 1.2 * min_price_increment_;
      event_source_price_map_["Gross_Domestic_Product_Annualized_ES_0"] = 1.6 * min_price_increment_;
      event_source_price_map_["Nonfarm_Payrolls_ES_0"] = 9.55 * min_price_increment_;
    } else if (source_shortcode_ == std::string("CL_0")) {
      event_source_price_map_["EIA_Crude_Oil_Stocks_change_CL_0"] = 21 * min_price_increment_;
      event_source_price_map_["API_Weekly_Crude_Oil_Stock_CL_0"] = 23 * min_price_increment_;
    } else if (source_shortcode_ == std::string("RB_0")) {
      event_source_price_map_["EIA_Crude_Oil_Stocks_change_RB_0"] = 65 * min_price_increment_;
      event_source_price_map_["API_Weekly_Crude_Oil_Stock_RB_0"] = 60 * min_price_increment_;
    }
  }
};
}
#endif  // BASE_EXECLOGIC_EVENT_DIRECTIONAL_AGGRESSIVE_TRADING_H
