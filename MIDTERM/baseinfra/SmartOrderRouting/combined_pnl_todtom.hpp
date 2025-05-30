/**
    \file SmartOrderRouting/combined_pnl_todtom.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/SmartOrderRouting/pnl_writer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {
class CombinedPnlTodTom {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityMarketView& tod_smv_;
  const SecurityMarketView& tom_smv_;
  const SecurityMarketView& tod_tom_smv_;

  int tod_position_;
  int tom_position_;
  int tod_tom_position_;
  int spread_position_;

  double tod_commish_;
  double tom_commish_;
  double tod_tom_commish_;

  double tod_n2d_;
  double tom_n2d_;
  double tod_tom_n2d_;

  double tod_mkt_price_;
  double tom_mkt_price_;
  double tod_tom_mkt_price_;

  double tod_average_open_price_;
  double tom_average_open_price_;
  double tod_tom_average_open_price_;

  double total_pnl_;
  double tod_pnl_;
  double tod_total_pnl_;
  double tom_pnl_;
  double tom_total_pnl_;
  double tod_tom_pnl_;
  double tod_tom_total_pnl_;

  char open_or_flat_;

  PnlWriter* pnl_writer_;

 public:
  CombinedPnlTodTom(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& tod_smv,
                    const SecurityMarketView& tom_smv, const SecurityMarketView& tod_tom_smv_, PnlWriter* pnl_writer);

  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int _security_id_);

  double GetUnrealizedPnl();
  double GetUnrealizedPnl(int _security_id_, double _pnl_, int _tradesize_);

  int GetTODPosition();
  int GetTOMPosition();
  int GetTODTOMPosition();
  inline int GetTODPnl() { return tod_total_pnl_; }
  inline int GetTOMPnl() { return tom_total_pnl_; }
  inline int GetTODTOMPnl() { return tod_tom_total_pnl_; }
  int GetSpreadPosition();
};
}
