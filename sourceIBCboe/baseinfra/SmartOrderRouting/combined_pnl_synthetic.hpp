/**
    \file SmartOrderRouting/combined_pnl_synthetic.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/SmartOrderRouting/pnl_writer.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {
class CombinedPnlSynthetic : public SecurityMarketViewChangeListener, public TimePeriodListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityMarketView& leg1_smv_;
  const SecurityMarketView& leg2_smv_;

  std::vector<std::string> shortcode_vec_;

  int leg1_position_;
  int leg2_position_;
  int spread_position_;

  double leg1_commish_;
  double leg2_commish_;

  int leg1_reserves_;
  int leg2_reserves_;

  bool is_leg1_di1_;
  bool is_leg2_di1_;

  double leg1_n2d_;
  double leg2_n2d_;

  double leg1_mkt_price_;
  double leg2_mkt_price_;

  std::vector<double> dv01_vec_;

  double leg1_average_open_price_;
  double leg2_average_open_price_;

  double total_pnl_;
  double leg1_pnl_;
  double leg1_total_pnl_;
  double leg2_pnl_;
  double leg2_total_pnl_;

  char open_or_flat_;

  PnlWriter* pnl_writer_;

  int tradingdate_;

 public:
  CombinedPnlSynthetic(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& leg1_smv,
                       const SecurityMarketView& leg2_smv, PnlWriter* pnl_writer);

  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int _security_id_, const int _caos_);

  double GetUnrealizedPnl();
  double GetUnrealizedPnl(int _security_id_, double _pnl_, int _tradesize_);

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  virtual void OnTimePeriodUpdate(const int num_pages_to_add_);

  inline double GetDIContractNumbersToDollars(double _price_, int _reserves_);

  int GetLeg1Position();
  int GetLeg2Position();
  inline int GetLeg1Pnl() { return leg1_total_pnl_; }
  inline int GetLeg2Pnl() { return leg2_total_pnl_; }
  int GetSpreadPosition();
};
}
