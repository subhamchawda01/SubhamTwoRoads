/**
    \file ExecLogic/retail_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/LiveSources/retail_trade_listener.hpp"
#include "dvccode/LiveSources/retail_trading_listener.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"
#include "dvccode/CDef/math_utils.hpp"
// exec_logic_code / defines.hpp was empty
#include "dvccode/CDef/retail_data_defines.hpp"
#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/Indicators/exp_moving_sum_generic.hpp"

#define FAT_FINGER_FACTOR 5

namespace HFSAT {

class RetailTrading : public ExecInterface,
                      public FPOrderExecutedListener,
                      public SlowStdevCalculatorListener,
                      public TimePeriodListener {
 protected:
  const unsigned int security_id_;

  // offered prices/sizes
  HFSAT::CDef::RetailOffer last_retail_offer_;

  std::vector<RetailTradingListener*> retail_offer_listeners_;

  const double avg_stdev_;
  double retail_stdev_;

  int bom_position_;
  int fp_position_;
  int my_position_;
  int my_global_position_;

  PromOrderManager* p_prom_order_manager_;
  int last_retail_update_msecs_;

  int last_full_logging_msecs_;

  bool is_ready_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  const int runtime_id_;

  bool should_be_getting_flat_;  ///< a precalculated answer of ShouldBeGettingFlat

  bool start_not_given_;                  // just to check if start was given once or not in live
  bool getflat_due_to_external_getflat_;  ///< internal flag set to true when we get a message to get flat from a user
  /// message
  bool getflat_due_to_close_;           ///< internal flag set to true when the time of day exceeds endtime of trading
  bool getflat_due_to_max_loss_;        ///< internal flag set to true when total_pnl_ falls below paramset_.max_loss_
  bool getflat_due_to_economic_times_;  ///< internal flag set to true when economic events happening
  bool getflat_due_to_market_data_interrupt_;  ///< internal flag set to true when market data interrupted
  bool getflat_due_to_allowed_economic_event_;
  unsigned int last_allowed_event_index_;

  GenericControlReplyStruct control_reply_struct_;
  EconomicEventsManager& economic_events_manager_;
  std::vector<EconomicZone_t> ezone_vec_;

  double severity_to_getflat_on_base_;
  double severity_to_getflat_on_;
  int severity_change_end_msecs_;
  double applicable_severity_;
  bool allowed_events_present_;

  SlowStdevCalculator* stdev_calculator_;

  int bidsize_to_show_maxpos_limit;
  int asksize_to_show_maxpos_limit;
  int bidsize_to_show_global_maxpos_limit;
  int asksize_to_show_global_maxpos_limit;

  HFSAT::CDef::RetailUpdateType getflat_retail_update_type_;
  int last_fp_buy_order_msecs_;
  int last_fp_sell_order_msecs_;

  double target_price_;
  double targetbias_numbers_;

  int param_index_to_use_;  // would be always 0, just using this for compatibility of copied code from basetrading ,
                            // will be helpful if we ever have regime in retailtrading
  ExpMovingSumGeneric* p_exp_mov_fok_size_;

 public:
  RetailTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
                SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
                MulticastSenderSocket* _p_strategy_param_sender_socket_,
                EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
                const int t_trading_end_utc_mfm_, const int t_runtime_id_,
                const std::vector<std::string> _this_model_source_shortcode_vec_);

  ~RetailTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  inline static std::string StrategyName() {
    return "RetailTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  inline bool SubscribeRetailOfferUpdate(RetailTradingListener* _listener_) {
    return VectorUtils::UniqueVectorAdd(retail_offer_listeners_, _listener_);
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  void FPOrderExecuted(const char* _secname_, double _price_, TradeType_t r_buysell_, int _size_executed_);

  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_);

  void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_val_);

  bool UpdateTarget(double _new_target_, double _targetbias_numbers_,
                    int _modelmath_index_ = 0);  ///< Called from BaseModelMath
  inline bool UpdateTarget(double _prob_decrease_, double _prob_nochange_, double _prob_increase_,
                           int _modelmath_index_ = 0) {
    return false;
  }
  void TargetNotReady();  ///< Called from BaseModelMath

  void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  inline int my_position() const { return my_position_; }
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_);

  void OnFokFill(const TradeType_t _buysell_, const double _price_, const int intpx_, const int _size_exec_);

  // unimplemented functions of execlogic
  void OnNewMidNight() {}
  void OnGlobalPNLChange(double _new_global_pnl_) {}
  void OnRejectDueToFunds(const TradeType_t _buysell_) {}
  void OnWakeUpifRejectDueToFunds() {}
  void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {}
  void OnGetFreezeDueToORSRejects() {}
  void OnResetByManualInterventionOverRejects() {}

  void OnRiskManagerUpdate(const unsigned int r_security_id_, double new_risk_) {}
  void OnOrderChange() {}
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {}
  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_) {}
  void OnFokReject(const TradeType_t _buysell_, const double _price_, const int intpx_, const int _size_remaining_) {}
  void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {}
  void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) {}

 protected:
  void TradingLogic();  // only call via UpdateTarget
  bool TrySendingFOKOrders();
  void SendTradeAndLog(int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_);

  void AdjustMaxPosLimitOfferSizes();
  void AdjustGlobalMaxPosLimitOfferSizes();

  void ProcessGetFlat();
  bool RetailShouldbeGettingFlat(HFSAT::CDef::RetailUpdateType& _retail_update_type_);

  void SetOfferVars();  // only call via UpdateTarget
  void NotifyListeners(const HFSAT::CDef::RetailOffer& _retail_offer_);

  void PrintFullStatus();
  void LogFullStatus();

  void CheckParamSet();
  void ReconcileParams();
  void ShowParams() {}

  void UpdateGlobalRisk() {}
  void AllocateCPU();
};
}
