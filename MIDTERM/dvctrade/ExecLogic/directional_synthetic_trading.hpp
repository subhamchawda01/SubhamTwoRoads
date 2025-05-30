/**
   \file ExecLogic/arb_trading.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvctrade/ExecLogic/exec_interface.hpp"
#include "baseinfra/SmartOrderRouting/combined_pnl_synthetic.hpp"
#include "dvccode/Utils/synthetic_security_manager.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/tradevarset_builder.hpp"

#include "dvccode/Utils/CPUAffinity.hpp"

#define MAX_POS_MAP_SIZE 64

namespace HFSAT {

struct DirectionalSyntheticTradingParams {};

class DirectionalSyntheticTrading : public ExecInterface, public TimePeriodListener {
 private:
  const SecurityMarketView& dep_market_view_;
  std::vector<const SecurityMarketView*> smv_vec_;

  std::vector<HFSAT::SmartOrderManager*> leg_om_vec_;

  int synth_sec_id_;
  std::vector<std::string> shortcode_vec_;
  std::map<unsigned int, int> secid_to_idx_map_;

  int map_pos_increment_;  ///< = (int)std::max ( 1, ( param_set_.max_position_ / MAX_POS_MAP_SIZE ) ) )
  std::vector<int> map_pos_increment_vec_;
  /// idx=2*MAX_POS_MAP_SIZE corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
  /// idx=0 corresponds to position = ( MAX_POS_MAP_SIZE * map_pos_increment_ )
  /// position P corresponds to idx= MAX_POS_MAP_SIZE + ( P / map_pos_increment_ )

  TradeVarSetBuilder* tradevarset_builder_;
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
  int param_index_to_use_;

  int my_position_;
  int spread_adj_leg1_pos_;
  int spread_adj_leg2_pos_;

  bool is_synth_flat_;
  bool is_outright_risk_;

  std::vector<int> leg_positions_vec_;
  std::vector<int> leg_max_position_vec_;
  std::map<int, int> leg1_to_leg2_pos_map_;
  std::map<int, int> leg2_to_leg1_pos_map_;

  double best_nonself_bid_price_;
  int best_nonself_bid_int_price_;
  int best_nonself_bid_size_;
  double best_nonself_ask_price_;
  int best_nonself_ask_int_price_;
  int best_nonself_ask_size_;

  std::vector<int> leg_bid_int_price_vec_;
  std::vector<int> leg_ask_int_price_vec_;
  std::vector<double> leg_bid_price_vec_;
  std::vector<double> leg_ask_price_vec_;
  std::vector<unsigned int> leg_bid_size_vec_;
  std::vector<unsigned int> leg_ask_size_vec_;
  std::vector<unsigned int> leg_bid_orders_vec_;
  std::vector<unsigned int> leg_ask_orders_vec_;

  std::vector<int> uts_vec_;
  std::vector<int> leg_bid_trade_size_vec_;
  std::vector<int> leg_ask_trade_size_vec_;
  std::vector<int> leg_bid_imp_trade_size_vec_;
  std::vector<int> leg_ask_imp_trade_size_vec_;
  std::vector<double> dv01_vec_;
  bool is_dv01_not_updated_;
  bool is_di_spread_;

  ttime_t start_time_;
  ttime_t end_time_;
  int tradingdate_;

  int runtime_id_;

  bool is_ready_;

  double target_price_;
  double targetbias_numbers_;
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
  bool closeout_extra_pos_long_;
  bool placed_asks_this_round_;
  bool canceled_asks_this_round_;
  bool closeout_extra_pos_short_;

  bool bid_improve_keep_;
  bool ask_improve_keep_;

  int last_buy_msecs_;
  int last_sell_msecs_;

  std::vector<int> last_agg_buy_msecs_vec_;
  std::vector<int> last_agg_sell_msecs_vec_;

  // For logging purpose
  CombinedPnlSynthetic* combined_pnl_;
  double min_unrealized_pnl_;

  std::vector<MktStatus_t> leg_mkt_status_vec_;
  bool external_getflat_;
  bool getflat_due_to_maxloss_;
  bool getflat_due_to_mkt_status_;

  bool is_affined_;
  int last_affine_attempt_msecs_;
  bool is_alert_raised_;

  bool ShouldGetFlat();
  void GetFlatLogic();
  void GetFlat(unsigned int _index_);

  void UpdateVariables(int security_id);
  void TradingLogic();
  void SetBuyDirectives();
  void SetSellDirectives();

  bool ValidPrices();
  bool IsLastIOCOrderIncomplete();

  void PlaceCancelBuyAggOrders();
  void PlaceCancelBuyImpOrders();
  void PlaceCancelBuyBestOrders();

  void PlaceCancelSellAggOrders();
  void PlaceCancelSellImpOrders();
  void PlaceCancelSellBestOrders();

  void CancelCloseoutExtraPos();
  void UpdatePositions();

  void UpdateDV01();
  void UpdateDV01Vars();

 public:
  DirectionalSyntheticTrading(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& _dep_market_view_,
                              const SecurityMarketView* leg1_smv, const SecurityMarketView* leg2_smv,
                              SmartOrderManager* leg1_order_manager, SmartOrderManager* leg2_order_manager,
                              const std::string& paramfilename, const bool livetrading, ttime_t start_time,
                              ttime_t end_time, int runtime_id, PnlWriter* pnl_writer);

  static void CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                   const std::string& dep_shortcode, std::vector<std::string>& source_shortcode_vec,
                                   std::vector<std::string>& ors_source_needed_vec,
                                   std::vector<std::string>& dependant_shortcode_vec);

  static void GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec);

  static std::string StrategyName() { return "DirectionalSyntheticTrading"; }

  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info);
  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                    const MarketUpdateInfo& market_update_info);

  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int _security_id_);
  void OnPositionChange(int new_position, int position_diff, const unsigned int security_id);

  void OnMarketStatusChange(const unsigned int security_id, const MktStatus_t new_market_status);
  void BuildTradeVarSets();
  void TradeVarSetLogic(int t_position);

  // TODO: To be implemented useful functions
  void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int security_id);
  void OnGlobalPNLChange(double new_global_PNL);
  void ReportResults(HFSAT::BulkFileWriter& trades_writer, bool conservative_close);

  virtual void OnTimePeriodUpdate(const int num_pages_to_add_);

  // Not useful virtual functions
  void OnWakeUpifRejectDueToFunds();

  inline void OnRiskManagerUpdate(const unsigned int security_id, double current_risk_mapped_to_product_position) {}

  void OnOrderChange();
  void OnGlobalPositionChange(const unsigned int security_id, int new_global_position);
  void OnControlUpdate(const ControlMessage& control_message, const char* symbol, const int trader_id);

  bool UpdateTarget(double new_target, double targetbias_numbers, int modelmath_index = 0);
  bool UpdateTarget(double prob_decrease, double prob_nochange, double prob_increase, int modelmath_index = 0) {
    return false;
  }

  inline int my_position() const { return my_position_; }
  void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) {}

  void OnFokReject(const TradeType_t buysell, const double price, const int int_price, const int size_remaining);
  void TargetNotReady();

  void OnCancelReject(const TradeType_t buysell, const double price, const int int_price, const int _security_id_) {}
  void OnNewMidNight() {}
  void OnFokFill(const TradeType_t buysell, const double price, const int int_price, const int size_exec) {}
  void OnRejectDueToFunds(const TradeType_t buysell) {}
  void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {}
  void OnGetFreezeDueToORSRejects() {}
  void OnResetByManualInterventionOverRejects() {}

 protected:
  void AllocateCPU() {
    if (is_affined_ || watch_.msecs_from_midnight() - last_affine_attempt_msecs_ < 20000) {
      return;
    }

    std::vector<std::string> affinity_process_list_vec;
    process_type_map process_and_type = AffinityAllocator::parseProcessListFile(affinity_process_list_vec);

    std::ostringstream t_temp_oss;
    t_temp_oss << "tradeinit-" << runtime_id_;

    int32_t core_assigned = CPUManager::allocateFirstBestAvailableCore(process_and_type, affinity_process_list_vec,
                                                                       getpid(), t_temp_oss.str(), false);
    is_affined_ = (core_assigned >= 0);

    // If we don't get a core even after 5 mins, raise an alert over slack
    // TODO - A lot of the code here has been duplicated across functions and classes,
    // Ideally we should have a parent class for all affinity management
    if (!is_affined_ && (watch_.msecs_from_midnight() - start_time_.getmfm() > 300000)) {
      if (!is_alert_raised_) {
        std::ostringstream t_temp_oss;
        t_temp_oss << "ALERT : UNABLE TO ALLOCATE A CORE TO THE TRADEINIT WITH PID : " << getpid()
                   << " QID : " << runtime_id_;
        DBGLOG_CLASS_FUNC_LINE_ERROR << t_temp_oss.str() << DBGLOG_ENDL_FLUSH;
        is_alert_raised_ = true;
      }
    }

    last_affine_attempt_msecs_ = watch_.msecs_from_midnight();
    DBGLOG_CLASS_FUNC_LINE_INFO << " AFFINED TO : " << core_assigned << " PID : " << getpid() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
};
}
