#ifndef BASE_EXECLOGIC_SGX_NK_SPREAD_MM_TRADING_H
#define BASE_EXECLOGIC_SGX_NK_SPREAD_MM_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"

namespace HFSAT {
class SgxNKSpreadMMTrading : public virtual BaseTrading, public IndicatorListener {
 protected:
  bool to_quote_;
  unsigned int total_time_to_quote_;
  unsigned int total_time_quoted_;
  int last_msecs_stopped_quoting_;
  int last_msecs_started_quoting_;
  double rate_;
  int position_get_flat_;
  const SecurityMarketView& nkm0_market_view_;
  const SecurityMarketView& nkm1_market_view_;

 public:
  SgxNKSpreadMMTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                       SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                       MulticastSenderSocket* _p_strategy_param_sender_socket_,
                       EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                       const int t_trading_end_utc_mfm_, const int _runtime_id_,
                       const std::vector<std::string> _this_model_source_shortcode_vec_,
                       const SecurityMarketView& _nkm0_market_view_, const SecurityMarketView& _nkm1_market_view_);

  ~SgxNKSpreadMMTrading() {}

  static std::string StrategyName() { return "SgxNKSpreadMMTrading"; }

  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (_indicator_index_ == 1) {
      rate_ = _new_value_;
    }
  }

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  virtual void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                 const double& new_value_nochange, const double& new_value_increase){};

  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                            std::vector<std::string>& source_shortcode_vec_,
                            std::vector<std::string>& ors_source_needed_vec_);

  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_);

  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);

  void OnTimePeriodUpdate(const int num_pages_to_add_);

 protected:
  bool toTrade();
  bool throttle_control();
  void TradingLogic();  ///< All the strategy based trade execution is written here
  void CallPlaceCancelNonBestLevels() {}
  void PlaceCancelNonBestLevels() {}

  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_) {}

  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  void PrintFullStatus() {}
  void CheckORSMDSDelay() {}
  void PlaceBidOrdersAtPrice(const int bid_int_price_to_quote_, const int size_to_place_);
  void PlaceAskOrdersAtPrice(const int ask_int_price_to_quote_, const int size_to_place_);
  void SetQuoteToFalse();
  void SetQuoteToTrue();
};
}
#endif  // BASE_EXECLOGIC_SGX_NK_SPREAD_MM_TRADING_H
