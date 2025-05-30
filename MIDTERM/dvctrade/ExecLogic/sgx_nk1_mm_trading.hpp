#ifndef BASE_EXECLOGIC_SGX_NK1_MM_TRADING_H
#define BASE_EXECLOGIC_SGX_NK1_MM_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

namespace HFSAT {
class SgxNK1MMTrading : public virtual BaseTrading, public IndicatorListener {
 protected:
  const SecurityMarketView& nkm_market_view_;
  const SecurityMarketView& nkm0_market_view_;
  const SecurityMarketView& sgx_nk0_market_view_;
  const SecurityMarketView& spread_nk_market_view_;
  bool to_quote_;
  unsigned int total_time_to_quote_;
  unsigned int total_time_quoted_;
  int last_msecs_nk1_exec_;
  int last_msecs_nkm_update_;
  int last_msecs_stopped_quoting_;
  int last_msecs_started_quoting_;
  double rate_;

  int nk0_position_;
  int nk1_position_;
  int spread_nk_position_;
  int nk0_1_position_;
  int total_position_;
  int nk_min_price_increment_;
  SmartOrderManager& nk1_om_;
  SmartOrderManager& nk0_om_;
  SmartOrderManager& spread_nk_om_;

  unsigned int nkm1_trades_flag_;
  double nkm1_trades_threshold_;
  double nkm1_trades_duration_;
  MovingAvgTradeSize* nkm1_avg_trade_size_;

  unsigned int nkm0_trades_flag_;
  double nkm0_trades_threshold_;
  double nkm0_trades_duration_;
  MovingAvgTradeSize* nkm0_avg_trade_size_;
  bool nkm_data_interrupted_;
  /*unsigned int stdev_duration_;
  unsigned int stdev_flag_;
  double stdev_threshold_;
*/
 public:
  SgxNK1MMTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                  SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                  MulticastSenderSocket* _p_strategy_param_sender_socket_,
                  EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                  const int t_trading_end_utc_mfm_, const int _runtime_id_,
                  const std::vector<std::string> _this_model_source_shortcode_vec_,
                  const SecurityMarketView& _sgx_nk0_market_view_, SmartOrderManager& _nk0_om_,
                  const SecurityMarketView& _spread_nk_market_view_, SmartOrderManager& _spread_nk_om_,
                  const SecurityMarketView& _nkm_market_view_, const SecurityMarketView& _nkm0_market_view_);

  ~SgxNK1MMTrading() {}

  static std::string StrategyName() { return "SgxNK1MMTrading"; }

  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (_indicator_index_ == 1) {
      if (fabs(_new_value_) > nkm1_trades_threshold_)
        nkm1_trades_flag_ = 1;
      else
        nkm1_trades_flag_ = 0;
    } else if (_indicator_index_ == 2) {
      if (fabs(_new_value_) > nkm0_trades_threshold_)
        nkm0_trades_flag_ = 1;
      else
        nkm0_trades_flag_ = 0;
    }
  }
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  bool throttle_control();
  void GetFlatTradingLogic();
  void GetFlatInSpread();
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  virtual void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                 const double& new_value_nochange, const double& new_value_increase){};

  static void CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                   const std::string& dep_shortcode, std::vector<std::string>& source_shortcode_vec,
                                   std::vector<std::string>& ors_source_needed_vec,
                                   std::vector<std::string>& dependant_shortcode_vec);

  static void GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec);

  /*void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                            std::vector<std::string>& source_shortcode_vec_,
                            std::vector<std::string>& ors_source_needed_vec_);
*/
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_);

  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);

 protected:
  bool toTrade();
  void TradingLogic();  ///< All the strategy based trade execution is written here
  void CallPlaceCancelNonBestLevels() {}
  void PlaceCancelNonBestLevels() {}

  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_) {}

  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  void FlatLogic();
  void PrintFullStatus() {}
  void CheckORSMDSDelay() {}
  void PlaceBidOrdersAtPrice(const int bid_int_price_to_quote_, const int size_to_place_);
  void PlaceAskOrdersAtPrice(const int ask_int_price_to_quote_, const int size_to_place_);
  void SetQuoteToFalse();
  void SetQuoteToTrue();
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
};
}
#endif  // BASE_EXECLOGIC_SGX_NK1 MM_TRADING_H
