/**
   \file ExecLogic/base_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <map>
#include <stdio.h>

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/TradeUtils/price_volatility_listener.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvccode/CommonTradeUtils/feature_predictor.hpp"
#include "dvccode/CommonTradeUtils/throttle_manager.hpp"
#include "dvccode/CommonTradeUtils/trading_stage_manager.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/indicator_helper.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/tradevarset_builder.hpp"
#include "dvctrade/ExecLogic/base_trading_defines.hpp"
#include "dvctrade/ExecLogic/exec_interface.hpp"
#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvctrade/ExecLogic/instrument_info.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"

#include "dvccode/Utils/CPUAffinity.hpp"

namespace HFSAT {

/** @brief Class to trade with a target price
 */
class BaseTrading : public ExecInterface,
                    public SlowStdevCalculatorListener,
                    public TimePeriodListener,
                    public SecurityMarketViewOnReadyListener,
                    public IndicatorHelperListener {
 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& _strategy_name_,
                                   const std::string& r_dep_shortcode_, std::vector<std::string>& source_shortcode_vec_,
                                   std::vector<std::string>& ors_source_needed_vec_);

  static void CollectShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                std::vector<std::string>& source_shortcode_vec_);

  static int class_var_counter_;

 protected:
  const unsigned int security_id_;

  /// Used for sending FOK
  PromOrderManager* p_prom_order_manager_;

  MulticastSenderSocket* p_strategy_param_sender_socket_;
  InstrumentInfo* product_;
  IndicatorHelper* exec_logic_indicators_helper_;
  TradeVarSetBuilder* tradevarset_builder_;

  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  const int runtime_id_;

  const std::vector<std::string> this_model_source_shortcode_vec_;
  std::map<std::string, int> this_model_source_shortcode_to_datainterrupt_map_;  // 0 dont care, not in core shortcode,
                                                                                 // 2 core shortcode but data
                                                                                 // interrupted, 1 data avaialable

  // variables
  double current_risk_mapped_to_product_position_;  ///< 1/2 * ( Del Squared_Risk / Del Position ) or a way to compute
  /// how much ( 1/2 * squared risk ) would increase per unit increase
  /// of position
  int my_position_;           ///< current position in the security traded
  int my_combined_position_;  ///< for pair_trading etc.
  int my_risk_;
  int my_global_position_;
  double my_combined_flat_pos_;
  /// mapping position to tradevarset
  /// 0 position corresponds to index MAX_POS_MAP_SIZE,
  int map_pos_increment_;  ///< = (int)std::max ( 1, ( param_set_.max_position_ / MAX_POS_MAP_SIZE ) ) )
  std::vector<int> map_pos_increment_vec_;
  /// idx=2*MAX_POS_MAP_SIZE corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
  /// idx=0 corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
  /// position P corresponds to idx= MAX_POS_MAP_SIZE + ( P / map_pos_increment_ )

  PositionTradeVarSetMap position_tradevarset_map_;
  std::vector<PositionTradeVarSetMap> position_tradevarset_map_vec_;
  const unsigned int P2TV_zero_idx_;

  TradeVars_t closeout_zeropos_tradevarset_;
  TradeVars_t closeout_long_tradevarset_;
  TradeVars_t closeout_short_tradevarset_;

  TradeVars_t current_tradevarset_;
  TradeVars_t current_bid_tradevarset_;
  TradeVars_t current_ask_tradevarset_;
  TradeVars_t current_bid_keep_tradevarset_;
  TradeVars_t current_ask_keep_tradevarset_;

  unsigned int current_position_tradevarset_map_index_;

  bool should_be_getting_flat_;  ///< a precalculated answer of ShouldBeGettingFlat

  bool fok_flat_active_;

  bool getflat_due_to_external_getflat_;  ///< internal flag set to true when we get a message to get flat from a user
  /// message
  bool getflat_due_to_external_agg_getflat_;
  bool getflat_due_to_funds_rej_;
  bool getflat_due_to_close_;         ///< internal flag set to true when the time of day exceeds endtime of trading
  bool getflat_due_to_max_position_;  ///< internal flag set to true when position rises much above max pos ... at that
  /// time we want to cut our losses and get flat
  bool getflat_due_to_max_loss_;         ///< internal flag set to true when total_pnl_ falls below paramset_.max_loss_
  bool getflat_due_to_max_pnl_;          ///< internal flag set to true when total_pnl_ goes above paramset_.max_pnl_
  bool getflat_due_to_global_max_loss_;  ///< internal flag set to true when total_global_pnl_ falls below
  /// paramset_.global_max_loss_
  bool getflat_due_to_short_term_global_max_loss_;  ///< internal flag set to true when total_global_pnl_ falls below
  /// paramset_.short_term_global_max_loss_
  bool getflat_due_to_max_opentrade_loss_;  ///< internal flag set to true when total_pnl_ falls below
  /// paramset_.max_opentrade_loss_
  bool getflat_due_to_max_drawdown_;
  bool getflat_due_to_economic_times_;  ///< internal flag set to true when economic events happening

  bool enable_market_data_interrupt_;
  bool getflat_due_to_market_data_interrupt_;  ///< internal flag set to true when market data interrupted
  bool getflat_due_to_market_status_;
  bool enable_non_standard_check_;
  bool getflat_due_to_non_standard_market_conditions_;  // Very wide spreads / uni-directional trades.
  bool getflat_due_to_non_tradable_events_;
  bool getflat_due_to_allowed_economic_event_;
  unsigned int last_allowed_event_index_;
  int last_getflat_due_to_non_standard_market_conditions_triggered_at_msecs_;

  bool getflat_due_to_lpm_;  ///< getflat due to large_price_mmovement, Currently used in Alphaflash execs
  bool getflat_due_to_regime_indicator_;

  std::map<int, int> non_standard_market_conditions_mode_prices_;
  int non_standard_market_condition_spread_;
  std::vector<int> non_standard_market_condition_bid_size_;
  std::vector<int> non_standard_market_condition_ask_size_;
  std::vector<int> non_standard_market_condition_bid_orders_;
  std::vector<int> non_standard_market_condition_ask_orders_;

  bool external_getflat_;  ///< external flag to not build position and place orders to cut down position and getflat
  bool rej_due_to_funds_;
  bool external_freeze_trading_;       ///< external flag to stop all order routing activity
  bool freeze_due_to_exchange_stage_;  /// if freezing due to pre-open or no-cancel period in exchange
  bool freeze_due_to_funds_reject_;    /// if freezing due to exchange rejection due to funds

  bool external_cancel_all_outstanding_orders_;

  bool agg_getflat_;

  ///< msecs_from_midnight of the last time max_opentrade_loss was triggered
  int last_getflat_due_to_max_opentrade_loss_triggered_at_msecs_;
  int last_getflat_due_to_short_term_global_max_loss_triggered_at_msecs_;
  int break_msecs_on_max_opentrade_loss_;  ///< the number of msecs to wait when max_opentrade_loss is triggered

  double best_nonself_bid_price_;
  int best_nonself_bid_int_price_;
  int best_nonself_bid_size_;

  double best_nonself_ask_price_;
  int best_nonself_ask_int_price_;
  int best_nonself_ask_size_;

  double best_nonself_mid_price_;
  double best_nonself_mkt_price_;

  double bestbid_queue_hysterisis_;
  double bestask_queue_hysterisis_;

  double dat_bid_reduce_l1bias_;
  double dat_ask_reduce_l1bias_;

  // these are from modelmath
  double target_price_;
  double targetbias_numbers_;

  // computation for online model stdev
  bool read_compute_model_stdev_;
  double model_stdev_decay_page_factor_;
  double model_stdev_inv_decay_sum_;
  double moving_avg_sumvars_;
  double moving_avg_squared_sumvars_;
  double model_stdev_;
  double online_model_scale_fact_;

  // we compute avg(targetbias_numbers_)
  // avg(targetbias_numbers_*targetbias_numbers_)

  std::vector<double> pos_to_thresh_bid_place_;
  std::vector<double> pos_to_thresh_ask_place_;
  std::vector<double> pos_to_thresh_bid_keep_;
  std::vector<double> pos_to_thresh_ask_keep_;

  bool flatfok_mode_;
  int send_next_fok_time_;
  bool getflatfokmode_;

  // probabilities from model math
  double prob_decrease_;
  double prob_nochange_;
  double prob_increase_;

  bool is_ready_;
  MktStatus_t market_status_;

  // top level directives
  bool top_bid_place_;
  bool top_bid_keep_;
  bool top_ask_place_;
  bool top_ask_keep_;

  bool top_bid_improve_;  ///< if true then we should improve and place a bid of l1bid_trade_size_ at
  /// best_nonself_bid_price_ + min_price_increment_, unless already there
  bool top_ask_lift_;     ///< if true then we should lift ( buy ) l1bid_trade_size_ at best_nonself_ask_price_
  bool top_ask_improve_;  ///< if true then we should improve and place an ask of l1ask_trade_size_ at
  /// best_nonself_ask_price_ - min_price_increment_, unless already there
  bool top_bid_hit_;  ///< if true then we should hit ( sell ) l1ask_trade_size_ at best_nonself_bid_price_
  bool placed_bids_this_round_;
  bool canceled_bids_this_round_;
  bool placed_asks_this_round_;
  bool canceled_asks_this_round_;

  bool bid_improve_keep_;
  bool ask_improve_keep_;

  int last_non_best_level_om_msecs_;
  int last_full_logging_msecs_;

  unsigned int non_best_level_order_management_counter_;

  GenericControlReplyStruct control_reply_struct_;
  EconomicEventsManager& economic_events_manager_;
  std::vector<EconomicZone_t> ezone_vec_;
  ThrottleManager* throttle_manager_;

  // to prevent successive trades very soon
  int last_buy_msecs_;
  int last_buy_int_price_;
  int last_sell_msecs_;
  int last_sell_int_price_;

  // for agg/improve cooloff
  int last_agg_buy_msecs_;
  int last_agg_sell_msecs_;

  int last_exec_msecs_;
  int last_agg_msecs_;

  // debug crap
  bool dump_inds;

  double stdev_;
  double stdev_scaled_capped_in_ticks_;
  unsigned int num_stdev_calls_;
  double sum_stdev_calls_;

  double severity_to_getflat_on_base_;
  double severity_to_getflat_on_;
  int severity_change_end_msecs_;
  double applicable_severity_;

  double short_term_loss_cutoff_decay_page_factor_;
  double moving_avg_pnl_;  ///< only needed for short term loss cutoff

  bool zero_logging_mode_;
  bool override_zero_logging_mode_once_for_external_cmd_;

  /// keep trak of open trade time
  int last_flip_msecs_;
  double our_global_pnl_;                    ///< to see the global PNL
  double our_short_term_global_pnl_;         ///< to see short term global PNL ( duration dictated by parameters )
  double our_short_term_global_pnl_latest_;  // maintaining two instead of much larger list of pnls at different
                                             // reference points
  double last_short_term_global_pnl_updated_msecs_;

  // Keep track of the bid_int_price
  // and ask_int_price at which to keep orders on
  // idea is to maintain orders at prices
  // avg-position-trade-price + BX ticks = ticks_to_keep_bid_int_price_
  // avg-position-trade-price - AX ticks = ticks_to_keep_ask_int_price_
  int ticks_to_keep_bid_int_price_;
  int ticks_to_keep_ask_int_price_;

  HFSAT::ttime_t last_update_ttime_;

  std::vector<std::string> last_5_trade_prices_;
  int sec_counter_for_INDINFO;
  bool is_event_based_;
  bool allowed_events_present_;

  CommonIndicator* p_dep_indep_based_regime_;
  CommonIndicator* p_moving_bidask_spread_;

  int min_msecs_to_switch_param_;
  int param_index_to_use_;
  int last_param_index_update_mfm_;

  // bool last_di_bid_mkt_tilt_cancel_msecs_;
  // bool last_di_ask_mkt_tilt_cancel_msecs_;

  double l1_bias_;
  double l1_order_bias_;

  double trade_bias_sell_;
  double trade_bias_buy_;

  double cancel_bias_sell_;
  double cancel_bias_buy_;
  double l1_ask_trade_bias_;
  double l1_bid_trade_bias_;
  double cancellation_model_bid_bias_;
  double cancellation_model_ask_bias_;

  double short_positioning_bias_;
  double long_positioning_bias_;

  int volume_adj_max_pos_;

  bool is_pair_strategy_;
  bool is_structured_strategy_;
  bool is_structured_general_strategy_;
  int num_opentrade_loss_hits_;
  int max_opentrade_loss_;

  // time decay factors
  // look back history is paramaterized
  // currently using BigTimePeriodUpdate ( window )
  double moving_avg_dep_bidask_spread_;
  int last_spread_recorded_;
  //

  bool start_not_given_;  // just to check if start was given once or not in live
  int minimal_risk_position_;

  int total_agg_flat_size_;

  bool getflat_mult_ord_;
  int max_orders_;
  bool computing_trade_prices_;

  int* secid_global_pos_map_;
  double* pc1_proj_ratio_;
  bool is_combined_getflat_;

  int tradingdate_;
  SlowStdevCalculator* stdev_calculator_;
  bool should_increase_thresholds_in_volatile_times_;
  bool should_check_worst_pos_after_placecxl_;

  std::vector<int> pnl_sampling_timestamps_;
  std::vector<int> pnl_samples_;
  unsigned int sample_index_;

  bool is_affined_;                 // to avoid affining more than once
  int last_affine_attempt_msecs_;   // to avoid too frequent affining attempts
  int first_affine_attempt_msecs_;  // to keep track of our first affine attempt

  std::string pid_file_;
  bool exit_bool_set_;
  bool first_obj_;
  unsigned int exit_cool_off_;

  int position_to_add_at_start_;  // For the case we send a request for add position and book is not ready, we store the
                                  // value in this variable

  int last_bigtrades_bid_cancel_msecs_;
  int last_bigtrades_ask_cancel_msecs_;
  int last_bigtrades_bid_place_msecs_;
  int last_bigtrades_ask_place_msecs_;
  int last_bigtrades_bid_aggress_msecs_;
  int last_bigtrades_ask_aggress_msecs_;

  bool cancel_l1_bid_ask_flow_buy_;
  bool cancel_l1_bid_ask_flow_sell_;

  int32_t last_freeze_time_;
  bool freeze_due_to_rejects_;
  double last_day_vol_ratio_;

  bool quoting_;
  unsigned int last_msecs_quoted_;
  unsigned int total_time_quoted_;

  bool is_alert_raised_;
  bool implied_mkt_bid_flag_;
  bool implied_mkt_ask_flag_;
  bool is_nse_earning_day_;

  unsigned int improve_cancel_counter_;
  int getflat_due_to_feature_model_;

  std::vector<std::string> improve_buy_time_stamp_vec_;
  std::vector<std::string> improve_sell_time_stamp_vec_;

  bool are_log_vec_dumped_;
  int pnl_at_close_;
  int position_at_close_;

 public:
  BaseTrading(DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& t_dep_market_view_,
              SmartOrderManager& t_order_manager_, const std::string& r_paramfilename_, const bool r_livetrading_,
              MulticastSenderSocket* _p_strategy_param_sender_socket_,
              EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
              const int t_trading_end_utc_mfm_, const int r_runtime_id_,
              const std::vector<std::string> _this_model_source_shortcode_vec_, bool _is_structured_strategy_ = false);

  virtual ~BaseTrading() {}

  /* External data based callbacks */
  void OnNewMidNight();

  // This needs to be virtual if child classes are going to extend it ?
  virtual void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

  virtual void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_);
  void LoadOvernightPositions();
  void UpdatePC1Risk(int sec_id, int new_position);
  void InitialisePC1Risk();
  void OnGlobalPNLChange(double _new_global_PNL_);

  inline void OnRiskManagerUpdate(const unsigned int r_security_id_,
                                  double r_current_risk_mapped_to_product_position_) {
    current_risk_mapped_to_product_position_ = r_current_risk_mapped_to_product_position_;
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << " r_current_risk_mapped_to_product_position_ = "
                             << r_current_risk_mapped_to_product_position_ << DBGLOG_ENDL_FLUSH;
    }
    if (is_ready_) TradeVarSetLogic(my_position_);
  }

  void OnFokReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                   const int size_remaining_);
  void OnFokFill(const TradeType_t _buysell_, const double _price_, const int r_int_price_, const int size_exec_);

  void OnOrderChange();

  void OnRejectDueToFunds(const TradeType_t _buysell_);
  void OnWakeUpifRejectDueToFunds();

  void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason);
  void HandleFreeze();
  void ComputeCurrentSeverity();

  void OnGetFreezeDueToORSRejects();
  void OnResetByManualInterventionOverRejects();

  virtual void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                              const int _security_id_);

  virtual void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                      const double _price_, const int r_int_price_, const int _security_id_);

  void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                       const int trader_id);  ///< Called from ControlMessageManager

  virtual void OnMarketUpdate(const unsigned int _security_id_,
                              const MarketUpdateInfo& _market_update_info_);  ///< Called from SecurityMarketView

  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_);  ///< Called from SecurityMarketView
  virtual void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_);
  void SMVOnReady();  /// Called from SecurityMarketView

  bool UsingCancellationModel();
  // virtual bool CheckUseCancellationModelParam();
  void SetupVariablesBeforeTradingLogic(double target_price, double targetbias_numbers);
  virtual bool UpdateTarget(double _new_target_, double _targetbias_numbers_,
                            int _modelmath_index_ = 0);  ///< Called from BaseModelMath
  bool UpdateTarget(double _prob_decrease_, double _prob_nochange_, double _prob_increase_, int _modelmath_index_ = 0);
  void UpdateCancelSignal(double predicted_class_, int bid_or_ask);
  void TargetNotReady();  ///< Called from BaseModelMath

  inline void OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_val_) {
    // raw stdev, mainly used for comparing with param_set_.low_stdev_lvl_ to check market volatility while placing
    // order
    stdev_ = _new_stdev_val_;
    double t_new_stdev_val_ = _new_stdev_val_;

    if (param_set_.use_sqrt_stdev_) {
      // to slowdown the impact of stdev on threshholds
      // Optimizing here , actual formula was tick_value * sqrt ( stdev/min_price )
      t_new_stdev_val_ = (sqrt(dep_market_view_.min_price_increment() * _new_stdev_val_));
    }

    // stdev_scaled_capped_in_ticks_ and stdev_scaled_capped_in_ticks_, used for mainly scaling threshholds
    stdev_scaled_capped_in_ticks_ = std::min(
        param_set_.stdev_overall_cap_,
        std::min(param_set_.stdev_cap_, param_set_.stdev_fact_ticks_ * t_new_stdev_val_) * last_day_vol_ratio_);
  }

  virtual void ProcessTimePeriodUpdate(const int num_pages_to_add_);

  /// currently called by watch ... TODO ... call this on select timeout in livetrading
  virtual void OnTimePeriodUpdate(const int num_pages_to_add_);

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  inline int my_position() const { return my_position_; }
  void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) {}
  inline int GetPosition() const { return my_position_; }
  inline bool get_flag_getflat_due_to_allowed_economic_event() const { return getflat_due_to_allowed_economic_event_; }

 protected:
  void ChangeThresholdsByInterdayScalingFactor();
  virtual void CheckParamSet();  ///< checks if the paramset is complete from the perspective of this strategy

  int GetPositonToTradeVarsetMapIndex(int t_position_);
  void BuildTradeVarSets();  ///< calling the functions to build tradevar sets and the map from position to TradeVars_t

  void UpdateThresholds();
  void BuildPosToThreshMap();  /// < For logistic regression

  ///< Function to map position to map_index and then the map_index is used to load
  virtual void TradeVarSetLogic(int t_position);

  /// the current_tradevarset_

  virtual void TradingLogic(){};  ///< All the strategy based trade execution is written here

  virtual void GetFlatTradingLogic();  ///< Trade Selection and execution during getflat mode

  virtual void GetFlatAggTradingLogic();

  virtual void GetFlatFokTradingLogic();  ///< Getflat mode with sending fok orders for internal matching as well
  /// passive orders on the exchange market

  void SendTradeAndLog(double _px_, int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_,
                       const std::string& _logging_str_);

  void SetGetFlatFokMode();  ///< sets the getflatfokmode_ flag
  void GetFlatSendFok();     ///< Sends fok orders for each iteration in fok flat mode

  void NonSelfMarketUpdate();  ///< Only update internal variables that depict the topmost bid price and topmost ask
  /// price where our own orders are not a huge part

  virtual void ProcessGetFlat();       ///< Calls ShouldBeGettingFlat and stores in should_be_getting_flat
  virtual bool ShouldBeGettingFlat();  ///< See if we need to get flat

  inline void UpdateMaxGlobalRisk() {}

  virtual void GetProductListToGetFlatMultOrder();

  bool SetPositionOffset(int t_position_offset_);
  virtual void UpdateBetaPosition(const unsigned int sec_id_, int new_pos);
  virtual void CallPlaceCancelNonBestLevels();
  virtual void PlaceCancelNonBestLevels();
  virtual void PlaceNonBestBids();
  virtual void CancelNonBestBids();
  virtual void PlaceNonBestAsks();
  virtual void CancelNonBestAsks();

  // change one side of the
  virtual void CancelAndClose();
  void ProcessRegimeChange();

  void ProcessIndicatorHelperUpdate();

  void ProcessAllowedEco();

  virtual int GetControlChars() const;      ///< sets the field query_control_bits_ of ParamSetSendStruct
  virtual void LogControlChars(const int);  ///< given query_control_bits_ prints what the state is
  virtual void ShowParams();                ///< log params, broadcast them with qid
  void LogParamSetSendStruct(ParamSetSendStruct& param_set_send_struct_);

  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_,
                             bool _conservative_close_);  ///< reports tradingdate pnl volume and other statistics ...

  virtual void PrintFullStatus(){};

  void LogFullStatus();

  void ShowBook();

  void SetComputeTradePrice(bool set_);
  bool CanPlaceNextOrder(int _int_price_, TradeType_t _trade_type_);
  void ComputeTradeBias();
  void ComputeCancellationModelBias(double predicted_class, int bid_or_ask);

  void SendMail(const std::string& mail_content, const std::string& mail_subject);

  virtual void SetStartTrading(bool _set_) { external_getflat_ = !_set_; }
  void AllocateCPU() {
    // Retry every 10 secs
    if (is_affined_ || watch_.msecs_from_midnight() - last_affine_attempt_msecs_ < 10000) {
      return;
    }

    if (first_affine_attempt_msecs_ == 0) {
      first_affine_attempt_msecs_ = watch_.msecs_from_midnight();
    }

    std::vector<std::string> affinity_process_list_vec;
    process_type_map process_and_type = AffinityAllocator::parseProcessListFile(affinity_process_list_vec);

    // Construct name by which we'll identify a process in affinity tracking
    std::ostringstream t_temp_oss;
    t_temp_oss << "tradeinit-" << runtime_id_;

    int32_t core_assigned = CPUManager::allocateFirstBestAvailableCore(process_and_type, affinity_process_list_vec,
                                                                       getpid(), t_temp_oss.str(), false);
    is_affined_ = (core_assigned >= 0);

    // If we don't get a core even after 50 secs of first attempt , then exit from the query.
    // TODO - A lot of the code here has been duplicated across functions and classes,
    // Ideally we should have a parent class for all affinity management
    if (!is_affined_ && (watch_.msecs_from_midnight() - first_affine_attempt_msecs_ > 50000)) {
      DBGLOG_CLASS_FUNC_LINE_INFO << " PID : " << getpid()
                                  << " not getting affined even after 50 secs. Stopping the query."
                                  << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      ExitVerbose(kExitErrorCodeGeneral, "Query not able to get affined");
    }

    last_affine_attempt_msecs_ = watch_.msecs_from_midnight();
    DBGLOG_CLASS_FUNC_LINE_INFO << " AFFINED TO : " << core_assigned << " PID : " << getpid() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  void ModifyThresholdsAsPerVolatility() {
    if ((stdev_scaled_capped_in_ticks_ > 1) &&
        should_increase_thresholds_in_volatile_times_) {  // more than normally volatile
      if (param_set_.increase_thresholds_symm_) {
        current_tradevarset_.MultiplyBidsBy(stdev_scaled_capped_in_ticks_);
        current_tradevarset_.MultiplyAsksBy(stdev_scaled_capped_in_ticks_);
      } else {
        // only scaling increasing thresholds
        if (param_set_.increase_thresholds_continuous_) {
          /*
           * Will remove this in Next PR
          double bid_factor =
              1 - std::max(0.0, std::min(1.0, (double)(current_tradevarset_.l1bid_trade_size_ - my_position_) /
                                                  (double)current_tradevarset_.l1bid_trade_size_));

          double ask_factor =
              1 - std::max(0.0, std::min(1.0, (double)(current_tradevarset_.l1ask_trade_size_ + my_position_) /
                                                  (double)current_tradevarset_.l1ask_trade_size_));
*/

          // Allowing scaling factor to get to 1 even when position is 0 and < 1 only when positions are in opposite

          /*
           * Not differentiating between small change in positions e.g. If UTS is 100 then in
           * terms of scaling the thresholds  +1 and -1 positions are equivalent.
           *
           * Also given we scale the thresholds even when position is 0,
           * we would want to decrease scale thresholds as position goes from 0 to UTS on opposite side
           */
          // side

          // using our size to UTS instead or bid_trade_size, as bid-trade-size can be 0
          double bid_factor = std::max(0.0, std::min(1.0, double(param_set_.unit_trade_size_ + my_position_) /
                                                              (double)(param_set_.unit_trade_size_)));

          double ask_factor = std::max(0.0, std::min(1.0, double(param_set_.unit_trade_size_ - my_position_) /
                                                              (double)(param_set_.unit_trade_size_)));

          // Multiply only increasing values
          // Dont let factor go below 1 ( not scaling)
          current_tradevarset_.MultiplyBidsBy(std::max(1.00, stdev_scaled_capped_in_ticks_ * bid_factor));
          current_tradevarset_.MultiplyAsksBy(std::max(1.00, stdev_scaled_capped_in_ticks_ * ask_factor));

        } else {
          if (my_position_ >= 0) {
            current_tradevarset_.MultiplyBidsBy(stdev_scaled_capped_in_ticks_);
          }
          if (my_position_ <= 0) {
            current_tradevarset_.MultiplyAsksBy(stdev_scaled_capped_in_ticks_);
          }
        }
      }
    }
  }

  void ModifyThresholdsAsPerModelStdev() {
    if ((param_set_.online_model_stdev_) && (online_model_scale_fact_ > 0)) {  // more than normally volatile
      if (param_set_.increase_thresholds_symm_) {
        current_tradevarset_.MultiplyBidsBy(online_model_scale_fact_);
        current_tradevarset_.MultiplyAsksBy(online_model_scale_fact_);
      } else {
        // only scaling increasing thresholds
        if (param_set_.increase_thresholds_continuous_) {
          double bid_factor = std::max(0.0, std::min(1.0, double(param_set_.unit_trade_size_ + my_position_) /
                                                              (double)(param_set_.unit_trade_size_)));

          double ask_factor = std::max(0.0, std::min(1.0, double(param_set_.unit_trade_size_ - my_position_) /
                                                              (double)(param_set_.unit_trade_size_)));

          // Multiply only increasing values
          // Dont let factor go below 1 ( not scaling)
          current_tradevarset_.MultiplyBidsBy(std::max(1.00, online_model_scale_fact_ * bid_factor));
          current_tradevarset_.MultiplyAsksBy(std::max(1.00, online_model_scale_fact_ * ask_factor));

        } else {
          if (my_position_ >= 0) {
            current_tradevarset_.MultiplyBidsBy(online_model_scale_fact_);
          }
          if (my_position_ <= 0) {
            current_tradevarset_.MultiplyAsksBy(online_model_scale_fact_);
          }
        }
      }
    }
  }

  void ModifyThresholdsAsPerPreGetFlat() {
    if (param_set_.use_pre_getflat_) {
      int msecs_since_pre_getflat_ =
          watch_.msecs_from_midnight() - (trading_end_utc_mfm_ - param_set_.pre_getflat_msecs_);
      if (msecs_since_pre_getflat_ >= 0) {
        // double factor = exp(0.000001*param_set_.pre_getflat_multiplier_ * msecs_since_pre_getflat_) ;
        // AB: I think we can approximate exp with following linear fn : e^x ~ 1+x for x~0, to be confirmed aashay
        double factor =
            1 + param_set_.pre_getflat_multiplier_ * (double(msecs_since_pre_getflat_) / param_set_.pre_getflat_msecs_);

        if (my_position_ >= 0) {
          current_tradevarset_.MultiplyBidsBy(factor);
        }
        if (my_position_ <= 0) {
          current_tradevarset_.MultiplyAsksBy(factor);
        }
      }
    }
  }

  void ModifyThresholdsAsPerMktConditions() {
    if ((exec_logic_indicators_helper_->avoid_long_market() ||
         exec_logic_indicators_helper_->avoid_long_market_aggressively()) &&
        my_position_ >= 0) {
      current_tradevarset_.AddBidsBy(HIGH_THRESHOLD_VALUE);
    }
    if ((exec_logic_indicators_helper_->avoid_short_market() ||
         exec_logic_indicators_helper_->avoid_short_market_aggressively()) &&
        my_position_ <= 0) {
      current_tradevarset_.AddAsksBy(HIGH_THRESHOLD_VALUE);
    }
  }

  virtual int GetPositionToClose() {
    int t_position_ = my_position_;
    if (is_combined_getflat_ && getflat_due_to_close_ && !getflat_due_to_external_getflat_) {
      t_position_ = std::round(my_combined_flat_pos_);
    }
    return t_position_;
  }

  void SetDisclosedFactor(double disclosed_size_factor) {
    order_manager_.SetDisclosedSizeFactor(disclosed_size_factor);
  }
};
}
