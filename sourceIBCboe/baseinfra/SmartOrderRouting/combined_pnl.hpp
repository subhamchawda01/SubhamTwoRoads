/**
    \file SmartOrderRouting/combined_pnl.hpp

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
class CombinedPnl {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityMarketView& dol_smv_;
  const SecurityMarketView& wdo_smv_;

  int dol_position_;
  int wdo_position_;
  int total_position_;

  double dol_commish_;
  double wdo_commish_;

  double dol_n2d_;
  double wdo_n2d_;

  double pnl_;
  double realized_pnl_;
  double dol_pnl_;
  double wdo_pnl_;

  PnlWriter* pnl_writer_;

  void ComputePnl(const int security_id, int prev_position, int prev_dol_pos, int prev_wdo_pos, double price);

 public:
  CombinedPnl(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& dol_smv,
              const SecurityMarketView& wdo_smv, PnlWriter* pnl_writer);

  void OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
              const int int_price, const int _security_id_, const int _caos_);

  double GetDOLPnl();
  double GetWDOPnl();
  double GetUnrealizedPnl();

  int GetDOLPosition();
  int GetWDOPosition();
  int GetTotalPosition();
};
}
