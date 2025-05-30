/**
   \file ExecLogic/arb_trading_todtom.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "dvctrade/ExecLogic/exec_interface.hpp"
#include "baseinfra/SmartOrderRouting/combined_pnl_todtom.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

struct ArbTradingTodTomParams {};

class ArbTradingTodTom : public ExecInterface {
 private:
  const SecurityMarketView& tod_smv_;
  const SecurityMarketView& tom_smv_;
  const SecurityMarketView& tod_tom_smv_;

  SmartOrderManager& tod_om_;
  SmartOrderManager& tom_om_;
  SmartOrderManager& tod_tom_om_;

  int tod_secid_;
  int tom_secid_;
  int tod_tom_secid_;

  int tod_bid_int_price_;
  int tod_ask_int_price_;
  double tod_bid_price_;
  double tod_ask_price_;
  int tod_mid_int_price_;
  int tod_bid_ask_spread_;
  int tod_bid_size_;
  int tod_ask_size_;

  int tom_bid_int_price_;
  int tom_ask_int_price_;
  double tom_bid_price_;
  double tom_ask_price_;
  int tom_mid_int_price_;
  int tom_bid_ask_spread_;
  int tom_bid_size_;
  int tom_ask_size_;

  int tod_tom_bid_int_price_;
  int tod_tom_ask_int_price_;
  double tod_tom_bid_price_;
  double tod_tom_ask_price_;
  int tod_tom_mid_int_price_;
  int tod_tom_bid_ask_spread_;
  int tod_tom_bid_size_;
  int tod_tom_ask_size_;

  int tod_position_;
  int tom_position_;
  int tod_tom_position_;

  int spread_adj_tod_position_;
  int spread_adj_tom_position_;

  int spread_position_;
  int accumulated_spread_position_;
  int excess_tod_position_;
  int excess_tom_position_;

  bool tod_best_tom_agg_;
  bool tom_best_tod_agg_;

  double synth_spread_buy_;
  double synth_spread_sell_;
  bool synth_spread_buy_thresh_flag_;
  bool synth_spread_sell_thresh_flag_;
  bool synth_spread_buy_flag_;
  bool synth_spread_sell_flag_;

  int num_lvl_order_keep_;

  int last_new_page_msecs_;

  int unit_trade_size_;
  double arb_place_thresh_;

  int desired_spread_position_;

  ttime_t start_time_;
  ttime_t end_time_;

  bool external_getflat_;
  int runtime_id_;
  bool getflat_due_to_maxloss_;

  // For logging purpose
  int num_arb_chances_;
  int last_arb_size_;

  CombinedPnlTodTom* combined_pnl_;

  MktStatus_t tod_mkt_status_;
  MktStatus_t tom_mkt_status_;
  MktStatus_t tod_tom_mkt_status_;
  bool getflat_due_to_mkt_status_;

  bool is_ready_;

  bool ShouldGetFlat();
  void GetFlatTradingLogic(int t_position_, SmartOrderManager& order_manager_, const SecurityMarketView& t_smv_);
  void GetFlatTradingLogic();

  void UpdateVariables(int security_id);
  void TradingLogic(int security_id);
  void OrderPlacingLogic();
  int SendSiBid(int size, int int_price = 0);
  int SendSiAsk(int size, int int_price = 0);
  int SendTOMBid(int size, int int_price = 0);
  int SendTOMAsk(int size, int int_price = 0);

  bool ValidPrices();
  std::string GetCurrentMarket();
  bool IsLastIOCOrderIncomplete();

 public:
  ArbTradingTodTom(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& tod_smv,
                   const SecurityMarketView& tom_smv, const SecurityMarketView& tod_tom_smv,
                   SmartOrderManager& tod_order_manager, SmartOrderManager& tom_order_manager,
                   SmartOrderManager& tod_tom_order_manager, const std::string& paramfilename, const bool livetrading,
                   ttime_t start_time, ttime_t end_time, int runtime_id, PnlWriter* pnl_writer);

  static void CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                   const std::string& dep_shortcode, std::vector<std::string>& source_shortcode_vec,
                                   std::vector<std::string>& ors_source_needed_vec,
                                   std::vector<std::string>& dependant_shortcode_vec);

  static void GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec);

  static std::string StrategyName() { return "ArbTradingTodTom"; }

  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info);
  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                    const MarketUpdateInfo& market_update_info);

  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int _security_id_);
  void OnPositionChange(int new_position, int position_diff, const unsigned int security_id);

  inline int my_position() const { return 0; }

  void OnMarketStatusChange(const unsigned int security_id, const MktStatus_t new_market_status);

  // TODO: To be implemented useful functions
  void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int security_id);
  void OnGlobalPNLChange(double new_global_PNL);
  void ReportResults(HFSAT::BulkFileWriter& trades_writer, bool conservative_close);

  void SendAggBuy(const SecurityMarketView& _smv_, SmartOrderManager& _om_, int _price_, int _mid_int_price_,
                  int _size_);
  void SendAggSell(const SecurityMarketView& _smv_, SmartOrderManager& _om_, int _price_, int _mid_int_price_,
                   int _size_);
  void ReconcilePositions();

  void get_positions(std::vector<std::string>& instrument_vec, std::vector<int>& position_vec);

  // Not useful virtual functions
  void OnWakeUpifRejectDueToFunds();
  void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {}
  void OnGetFreezeDueToORSRejects() {}
  void OnResetByManualInterventionOverRejects() {}

  inline void OnRiskManagerUpdate(const unsigned int security_id, double current_risk_mapped_to_product_position) {}

  void OnOrderChange();
  void OnGlobalPositionChange(const unsigned int security_id, int new_global_position);
  void OnControlUpdate(const ControlMessage& control_message, const char* symbol, const int trader_id);

  bool UpdateTarget(double new_target, double targetbias_numbers, int modelmath_index = 0);
  bool UpdateTarget(double prob_decrease, double prob_nochange, double prob_increase, int modelmath_index = 0) {
    return false;
  }

  void OnFokReject(const TradeType_t buysell, const double price, const int int_price, const int size_remaining);
  void TargetNotReady();

  void OnCancelReject(const TradeType_t buysell, const double price, const int int_price, const int _security_id_) {}
  void OnNewMidNight() {}
  void OnFokFill(const TradeType_t buysell, const double price, const int int_price, const int size_exec) {}
  void OnRejectDueToFunds(const TradeType_t buysell) {}
};
}
