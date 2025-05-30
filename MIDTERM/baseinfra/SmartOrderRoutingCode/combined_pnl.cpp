/**
    \file SmartOrderRoutingCode/combined_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/SmartOrderRouting/combined_pnl.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

namespace HFSAT {

CombinedPnl::CombinedPnl(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& dol_smv,
                         const SecurityMarketView& wdo_smv, PnlWriter* pnl_writer)
    : dbglogger_(dbglogger),
      watch_(watch),
      dol_smv_(dol_smv),
      wdo_smv_(wdo_smv),
      dol_position_(0),
      wdo_position_(0),
      total_position_(0),
      dol_commish_(BaseCommish::GetCommishPerContract(dol_smv_.shortcode(), watch_.YYYYMMDD())),
      wdo_commish_(BaseCommish::GetCommishPerContract(wdo_smv_.shortcode(), watch_.YYYYMMDD())),
      dol_n2d_(SecurityDefinitions::GetContractNumbersToDollars(dol_smv_.shortcode(), watch_.YYYYMMDD())),
      wdo_n2d_(SecurityDefinitions::GetContractNumbersToDollars(wdo_smv_.shortcode(), watch_.YYYYMMDD())),
      pnl_(0),
      realized_pnl_(0),
      dol_pnl_(0),
      wdo_pnl_(0),
      pnl_writer_(pnl_writer) {}

void CombinedPnl::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell, const double price,
                         const int int_price, const int _security_id_) {
  int prev_position = total_position_;
  int prev_dol_pos = dol_position_;
  int prev_wdo_pos = wdo_position_;

  // DOL
  if (_security_id_ == (int)dol_smv_.security_id()) {
    dol_position_ = new_position;
  } else if (_security_id_ == (int)wdo_smv_.security_id()) {
    wdo_position_ = new_position;

    // Check if these are corresponding to the arb trade
  }

  total_position_ = dol_position_ * 5 + wdo_position_;

  ComputePnl(_security_id_, prev_position, prev_dol_pos, prev_wdo_pos, price);
}

void CombinedPnl::ComputePnl(const int security_id, int prev_position, int prev_dol_pos, int prev_wdo_pos,
                             double price) {
  int leg_position = 0;
  double leg_pnl = 0;
  if (security_id == (int)dol_smv_.security_id()) {
    leg_position = dol_position_;

    int diff = dol_position_ - prev_dol_pos;
    dol_pnl_ -= diff * price * dol_n2d_;
    dol_pnl_ -= abs(diff) * dol_commish_;

    leg_pnl = dol_pnl_ + dol_position_ * price * dol_n2d_ - abs(dol_position_) * dol_commish_;
  } else if (security_id == (int)wdo_smv_.security_id()) {
    leg_position = wdo_position_;

    int diff = wdo_position_ - prev_wdo_pos;
    wdo_pnl_ -= diff * price * wdo_n2d_;
    wdo_pnl_ -= abs(diff) * wdo_commish_;

    leg_pnl = wdo_pnl_ + wdo_position_ * price * wdo_n2d_ - abs(wdo_position_) * wdo_commish_;
  }

  // This trade switches the total position
  if (abs(prev_position) > 0 && total_position_ * prev_position <= 0) {
    // First part of the trade

    pnl_ += prev_position * price * wdo_n2d_;
    pnl_ -= abs(prev_position) * wdo_commish_;

    // Since position becomes 0 here, this is the latest realized pnl
    realized_pnl_ = pnl_;

    TradeType_t buysell = kTradeTypeBuy;        // from -ve position to +ve position
    if (total_position_ - prev_position < 0) {  // from +ve position to -ve position
      buysell = kTradeTypeSell;
    }

    pnl_writer_->WriteTrade(security_id, watch_.tv(), abs(prev_position), price, leg_position, 0, (int)round(leg_pnl),
                            dol_smv_.bestbid_size(), dol_smv_.bestbid_price(), dol_smv_.bestask_price(),
                            dol_smv_.bestask_size(), 0, pnl_, 'F', GetTradeTypeChar(buysell));

    // Second part of the trade
    if (abs(total_position_) > 0) {
      pnl_ -= total_position_ * price * wdo_n2d_;
      pnl_ -= abs(total_position_) * wdo_commish_;
      double unrealized_pnl = pnl_ + total_position_ * price * wdo_n2d_ - abs(total_position_) * wdo_commish_;
      double opentrade_pnl = unrealized_pnl - realized_pnl_;

      pnl_writer_->WriteTrade(security_id, watch_.tv(), abs(total_position_), price, leg_position,
                              (int)round(opentrade_pnl), (int)round(leg_pnl), dol_smv_.bestbid_size(),
                              dol_smv_.bestbid_price(), dol_smv_.bestask_price(), dol_smv_.bestask_size(),
                              total_position_, unrealized_pnl, 'O', GetTradeTypeChar(buysell));
    }
  } else {
    int diff = total_position_ - prev_position;
    pnl_ -= diff * price * wdo_n2d_;
    pnl_ -= abs(diff) * wdo_commish_;
    double unrealized_pnl = pnl_ + total_position_ * price * wdo_n2d_ - abs(total_position_) * wdo_commish_;
    double opentrade_pnl = unrealized_pnl - realized_pnl_;
    TradeType_t buysell = kTradeTypeBuy;
    if (diff < 0) {
      buysell = kTradeTypeSell;
    }

    pnl_writer_->WriteTrade(security_id, watch_.tv(), abs(diff), price, leg_position, (int)round(opentrade_pnl),
                            (int)round(leg_pnl), dol_smv_.bestbid_size(), dol_smv_.bestbid_price(),
                            dol_smv_.bestask_price(), dol_smv_.bestask_size(), total_position_, unrealized_pnl, 'O',
                            GetTradeTypeChar(buysell));
  }
}

double CombinedPnl::GetDOLPnl() {
  return dol_pnl_ + dol_position_ * dol_smv_.mid_price() * dol_n2d_ - abs(dol_position_) * dol_commish_;
}

double CombinedPnl::GetWDOPnl() {
  return wdo_pnl_ + wdo_position_ * dol_smv_.mid_price() * wdo_n2d_ - abs(wdo_position_) * wdo_commish_;
}

double CombinedPnl::GetUnrealizedPnl() {
  double mid_price = dol_smv_.mid_price();
  return pnl_ + total_position_ * mid_price * wdo_n2d_ - abs(total_position_) * wdo_commish_;
}

int CombinedPnl::GetDOLPosition() { return dol_position_; }

int CombinedPnl::GetWDOPosition() { return wdo_position_; }

int CombinedPnl::GetTotalPosition() { return total_position_; }
}
