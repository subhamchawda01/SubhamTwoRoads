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
#include "baseinfra/SmartOrderRouting/combined_pnl.hpp"

namespace HFSAT {

struct ArbTradingParams {};

class ArbTrading : public ExecInterface {
 private:
  const SecurityMarketView& dol_smv_;
  const SecurityMarketView& wdo_smv_;

  SmartOrderManager& dol_om_;
  SmartOrderManager& wdo_om_;

  int dol_secid_;
  int wdo_secid_;

  int dol_bid_int_price_;
  int dol_ask_int_price_;
  int dol_bid_size_;
  int dol_ask_size_;
  int dol_bid_orders_;
  int dol_ask_orders_;

  int wdo_bid_int_price_;
  int wdo_ask_int_price_;
  int wdo_bid_size_;
  int wdo_ask_size_;
  int wdo_bid_orders_;
  int wdo_ask_orders_;

  ttime_t start_time_;
  ttime_t end_time_;

  int last_arb_start_msecs_;
  int last_wdo_arb_bid_int_price_;
  int last_wdo_arb_ask_int_price_;
  int last_dol_arb_bid_int_price_;
  int last_dol_arb_ask_int_price_;

  bool external_getflat_;
  int runtime_id_;
  bool getflat_due_to_maxloss_;

  // For logging purpose
  int num_arb_chances_;
  int last_arb_size_;

  CombinedPnl* combined_pnl_;
  int leading_secid_;

  MktStatus_t dol_mkt_status_;
  MktStatus_t wdo_mkt_status_;
  bool getflat_due_to_mkt_status_;

  bool ShouldGetFlat();
  void GetFlatLogic();

  void UpdateVariables(int security_id);
  void TradingLogic(int security_id);

  bool IsReadyToTakeArbTrade();
  bool StartArbTrade();
  bool CloseArbTrade();
  int ClosePositivePosition();
  int CloseNegativePosition();
  int SendDOLBid(int size, int int_price = 0);
  int SendDOLAsk(int size, int int_price = 0);
  int SendWDOBid(int size, int int_price = 0);
  int SendWDOAsk(int size, int int_price = 0);

  bool ValidPrices();
  std::string GetCurrentMarket();
  bool IsLastIOCOrderIncomplete();

 public:
  ArbTrading(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& dol_smv,
             const SecurityMarketView& wdo_smv, SmartOrderManager& dol_order_manager,
             SmartOrderManager& wdo_order_manager, const std::string& paramfilename, const bool livetrading,
             ttime_t start_time, ttime_t end_time, int runtime_id, PnlWriter* pnl_writer);

  static void CollectORSShortCodes(DebugLogger& dbglogger, const std::string& strategy_name,
                                   const std::string& dep_shortcode, std::vector<std::string>& source_shortcode_vec,
                                   std::vector<std::string>& ors_source_needed_vec,
                                   std::vector<std::string>& dependant_shortcode_vec);

  static void GetDepShortcodes(const std::string& dep_shortcode, std::vector<std::string>& dep_shortcode_vec);

  static std::string StrategyName() { return "ArbTrading"; }

  void OnMarketUpdate(const unsigned int security_id, const MarketUpdateInfo& market_update_info);
  void OnTradePrint(const unsigned int security_id, const TradePrintInfo& trade_print_info,
                    const MarketUpdateInfo& market_update_info);

  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int _security_id_);
  void OnPositionChange(int new_position, int position_diff, const unsigned int security_id);

  inline int my_position() const {
    if (combined_pnl_) {
      return combined_pnl_->GetTotalPosition();
    }
    return 0;
  }

  void OnMarketStatusChange(const unsigned int security_id, const MktStatus_t new_market_status);

  // TODO: To be implemented useful functions
  void OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive);
  void OnMarketDataResumed(const unsigned int security_id);
  void OnGlobalPNLChange(double new_global_PNL);
  void ReportResults(HFSAT::BulkFileWriter& trades_writer, bool conservative_close);

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
