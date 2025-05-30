/**
    \file ExecLogic/event_directional_agrressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_EVENTBIAS_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_EVENTBIAS_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_aflash_processor.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"
#include "dvctrade/OfflineUtils/periodic_stdev_utils.hpp"

#define VAL_DECAY_MAX_LEN 600
#define VAL_EPSILON_INT 1e-2
#define DIFF_EPSILON_INT 1e-3
#define GETFLAT_COOLOFF 1000

namespace HFSAT {

/* @brief: Class to trade during events using Alphaflash feeds with Aggressive Orders
 * Currently, it relies on linear regression for finding the prediction price changes
 * It sends Aggressive Order until it is executed,
 * If excuted, it waits for cooloff period
 * Else, it sends again
 * Order Size is proportional to the predicted pxchange magnitude */

/*  stuct to contain the vital information regarding the event */
struct EventVals {
  std::map<uint8_t, double> actual_vals_;
  std::map<uint8_t, double> estimate_vals_;
  std::map<uint8_t, double> scale_beta_;
  uint16_t cat_id_;
  std::vector<uint8_t> cat_datum_;
  double mfm_received_;
  int event_mfm_;          // event-time in mfm (loaded from estimate file)
  double getflat_margin_;  // if abs(current_price - prior_price) >= getflat_margin in the opp directiom we getflat
  double max_uts_pxch_;    // the Predicted PriceChange at which we take maximum Position
  double order_scale_;     // the Scaling factor for determining position_to_take
};

class EventBiasAggressiveTrading : public virtual BaseTrading, AflashListener, public IndicatorListener {
 protected:
  double event_pr_st_msecs_;  // the prior_price is computed as average price since event_pr_st_msecs_ prior to event
  int last_viewed_msecs_;     // last time the event_signal was updated

  /*  variables for computing the avgpx_pr_  */
  double moving_sumpx_pr_;
  double count_px_pr_;
  double avgpx_pr_;          // avg_prior_price
  double max_pxch_;          // maximum price change observed post-event in the predicted direction
  double dd_margin_;         // margin of the drawdown
  double max_source_pxch_;   // maximum price change observed post-event in the predicted direction
  double source_dd_margin_;  // margin of the source drawdown

  /*  variables for computing the avgpx_pr_ of the trend source */
  bool use_source_trend_;
  double source_moving_sumpx_pr_;
  double source_avgpx_pr_;
  double source_pxch_pred_;
  int source_pred_sign_;
  std::map<uint8_t, double> source_scale_beta_;
  SimplePriceType *source_price_indicator_;

  /* variables for trade indicator */
  bool use_source_trade_;
  double source_trade_secs_;
  int source_trade_index_;
  TDSumTDiffTSize *source_trade_indicator_;

  TimeDecayedTradeInfoManager *dep_decayed_trade_info_manager_;
  TimeDecayedTradeInfoManager *source_decayed_trade_info_manager_;

  /*  time-decayed version of predicted price_change */
  double event_signal_;
  double prev_event_signal_;
  double pxch_pred_;
  int pred_sign_;
  bool is_active_;
  bool af_feeds_recv_;
  int position_to_take_;

  /* staggered_getflat: to lay off positions slowly
   * if we mention getflat_mins as: 2,5,10
   * then position_to_take (P) is reduced to: 2P/3, P/3, 0 at the end of 2,5,10 mins */
  std::vector<int> staggered_getflat_mfms_;
  std::vector<int> staggered_getflat_maxpos_;

  AF_MSGSPECS::Category *catg_;
  EventVals event_;
  std::vector<double> val_decay_vec_;
  std::vector<double> drawdown_decay_vec_;
  double abs_indicator_value_;

  double VAL_EPSILON;
  double last_getflat_mfm_;

  double last_bidorder_msecs_;
  double last_askorder_msecs_;

  double dep_stdev_;

  bool getflat_aggress_on_dd_;
  bool getflat_yet_;

 public:
  EventBiasAggressiveTrading(DebugLogger &_dbglogger_, const Watch &_watch_, SecurityMarketView &_dep_market_view_,
                             SmartOrderManager &_order_manager_, const std::string &_paramfilename_,
                             const bool _livetrading_, MulticastSenderSocket *_p_strategy_param_sender_socket_,
                             EconomicEventsManager &t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                             const int t_trading_end_utc_mfm_, const int _runtime_id_,
                             const std::vector<std::string> _this_model_source_shortcode_vec_);

  /// destructor not made virtual ... please do so when making child classes
  ~EventBiasAggressiveTrading(){};

  static std::string StrategyName() {
    return "EventBiasAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  virtual void ProcessTimePeriodUpdate(const int num_pages_to_add_);

  void readEventsScale();
  void readEstimateEvents();
  void readMarginFile();
  void onAflashMsgNew(int uid_, timeval time_, char symbol_[4], uint8_t type_, uint8_t version_, uint8_t nfields_,
                      AFLASH_MDS::AFlashDatum fields[AFLASH_MDS::MAX_FIELDS], uint16_t category_id_);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                    const MarketUpdateInfo &_market_update_info_);

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_) {}
  virtual void OnIndicatorUpdate(const unsigned int &indicator_index, const double &new_value_decrease,
                                 const double &new_value_nochange, const double &new_value_increase){};

 protected:
  void OnControlUpdate(const ControlMessage &control_message, const char *symbol, const int trader_id);

  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
  void ProcessSignalUpdate(const int num_pages_to_add_);

  void TradingLogic();  ///< All the strategy based trade execution is written here
  void GetFlatTradingLogic();
  void GetFlatTradingLogic(int t_position_);

  void PrintFullStatus();
};
}
#endif  // BASE_EXECLOGIC_EVENT_DIRECTIONAL_AGGRESSIVE_TRADING_H
