/**
    \file SmartOrderRoutingCode/prom_pnl_indicator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/SmartOrderRouting/prom_pnl_indicator.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

namespace HFSAT {

PromPNLIndicator* PromPNLIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      PromOrderManager& t_prom_order_manager_,
                                                      SecurityMarketView& t_dep_market_view_) {
  static std::map<int, PromPNLIndicator*> sid_to_prom_pnl_indicator_map_;
  if (sid_to_prom_pnl_indicator_map_.find(t_dep_market_view_.security_id()) == sid_to_prom_pnl_indicator_map_.end()) {
    sid_to_prom_pnl_indicator_map_[t_dep_market_view_.security_id()] =
        new PromPNLIndicator(t_dbglogger_, r_watch_, t_prom_order_manager_, t_dep_market_view_);
  }
  return sid_to_prom_pnl_indicator_map_[t_dep_market_view_.security_id()];
}

PromPNLIndicator::PromPNLIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   PromOrderManager& t_prom_order_manager_, SecurityMarketView& _dep_market_view_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      dep_market_view_(_dep_market_view_),
      pnl_(0),
      realized_pnl_(0),
      opentrade_unrealized_pnl_(0),
      total_pnl_(0),
      last_closing_trade_pnl_(0),
      position_(0),
      commish_dollars_per_unit_(BaseCommish::GetCommishPerContract(_dep_market_view_.shortcode(), watch_.YYYYMMDD())),
      di_reserves_(SecurityDefinitions::GetDIReserves(watch_.YYYYMMDD(), dep_market_view_.shortcode())),
      numbers_to_dollars_(
          SecurityDefinitions::GetContractNumbersToDollars(_dep_market_view_.shortcode(), watch_.YYYYMMDD())),
      mid_price_(0) {
  t_prom_order_manager_.AddGlobalOrderExecListener(this);  // Only the global execution

  _dep_market_view_.ComputeMidPrice();            // Since we will need mid_price in OnMarketUpdae
  _dep_market_view_.subscribe_tradeprints(this);  // attaching itself as a general market data listener to l1 events or
                                                  // l1 price changes ( not size changes )
}

PromPNLIndicator::~PromPNLIndicator() {}

void PromPNLIndicator::OnGlobalOrderExec(const unsigned int security_id_, const TradeType_t _buysell_, const int _size_,
                                         const double _trade_px_) {
  int r_new_position_ = 0;
  // int _exec_quantity_ = _size_;
  double this_trade_price_ = _trade_px_;
  if (this_trade_price_ <= (kInvalidPrice + 0.5)) {
    if (!dep_market_view_.is_ready() ||
        dep_market_view_.mid_price() <=
            (kInvalidPrice + 0.5)) {  // don't have a valid price ... no point in continuing to compute pnl
      return;
    }
    this_trade_price_ = dep_market_view_.mid_price();
  }

  if (_buysell_ == kTradeTypeBuy)
    r_new_position_ = position_ + _size_;
  else
    r_new_position_ = position_ - _size_;

  int prev_total_pnl_ = total_pnl_;

  int abs_change_position_ = abs(r_new_position_ - position_);
  mid_price_ =
      this_trade_price_;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

  if (dep_market_view_.shortcode().find("DI1") != std::string::npos)  // BR_DI_1 we have a generic_ticker
  {
    numbers_to_dollars_ = -GetDIContractNumbersToDollars();
  }

  else if (dep_market_view_.exch_source() == kExchSourceBMFEQ) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBMFEquityCommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _size_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _size_;
  }

  else if (dep_market_view_.exch_source() == kExchSourceNSE) {
    commish_dollars_per_unit_ =
        BaseCommish::GetNSECommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _size_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _size_;
  } else if (dep_market_view_.exch_source() == kExchSourceBSE) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBSECommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _size_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _size_;
  }

  if (_buysell_ == kTradeTypeBuy) {  // buy was filled
    if ((position_ < 0) &&
        (position_ * r_new_position_ <= 0)) {  // we were short and position and new position are of differetn sign
      // hence we are changing sign
      // break trade into two parts
      // first part to close out earlier position
      int trade1size = -1 * position_;
      pnl_ -= (trade1size * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (trade1size * commish_dollars_per_unit_);

      last_closing_trade_pnl_ = pnl_ - realized_pnl_;
      realized_pnl_ = pnl_;

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);

      if (trade2size == 0) total_pnl_ = pnl_;
      if (trade2size > 0) {
        pnl_ -= (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
      }
    } else {
      pnl_ -= (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
    }
  } else {  // _buysell_ == TradeTypeSell ... sell trade
    if ((position_ > 0) &&
        (position_ * r_new_position_ <= 0)) {  // we were long and position and new position are of differetn sign
      // hence we are changing sign

      // break trade into two parts
      // first part to close out earlier position
      int trade1size = position_;
      pnl_ += (trade1size * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (trade1size * commish_dollars_per_unit_);

      // since trade closed update realized pnl estimates
      last_closing_trade_pnl_ = pnl_ - realized_pnl_;
      realized_pnl_ = pnl_;

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);
      if (trade2size == 0) total_pnl_ = pnl_;
      if (trade2size > 0) {
        pnl_ += (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
      }
    } else {
      pnl_ += (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
    }
  }
  position_ = r_new_position_;

  if (total_pnl_ != prev_total_pnl_) {  // inform the listeners
    for (auto i = 0u; i < global_PNL_change_listener_vec_.size(); i++) {
      global_PNL_change_listener_vec_[i]->OnGlobalPNLChange(total_pnl_);
    }
  }

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_FUNC << "Position " << position_ << " sid: " << security_id_ << " " << GetTradeTypeChar(_buysell_) << " "
                << _size_ << " @ " << _trade_px_ << " POSTPNL = " << total_pnl_ << DBGLOG_ENDL_FLUSH;
  }
}
}
