/**
    \file SmartOrderRoutingCode/prom_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/SmartOrderRouting/prom_pnl.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

namespace HFSAT {

PromPNL::PromPNL(DebugLogger& _dbglogger_, const Watch& _watch_, PromOrderManager& t_prom_order_manager_,
                 SecurityMarketView& _dep_market_view_, int t_runtime_id_, BulkFileWriter& t_trades_writer_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      prom_order_manager_(t_prom_order_manager_),
      dep_market_view_(_dep_market_view_),
      runtime_id_(t_runtime_id_),
      trades_writer_(t_trades_writer_),
      pnl_(0),
      previous_pnl(0),
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
  bzero(numbered_secname_, 24);
  snprintf(numbered_secname_, 24, "%s.%d", dep_market_view_.secname(), runtime_id_);
  for (size_t i = 0; i < strlen(numbered_secname_); ++i) {
    if (numbered_secname_[i] == ' ') {  // Liffe symbol naming crap
      numbered_secname_[i] = '~';
    }
  }

  prom_order_manager_.AddGlobalOrderExecListener(this);  // Only the global execution

  _dep_market_view_.ComputeMidPrice();            // Since we will need mid_price in OnMarketUpdae
  _dep_market_view_.subscribe_tradeprints(this);  // attaching itself as a general market data listener to l1 events or
                                                  // l1 price changes ( not size changes )
}

PromPNL::~PromPNL() {}

// void PromPNL::OnGlobalOrderExec ( const int r_new_position_, const int _exec_quantity_,
// 			 const TradeType_t _buysell_, const double this_trade_price_,
// 			 const int r_int_price_ )

double PromPNL::get_pnl_diff(int time_diff) {
  double pnl_diff_ = 0;
  bool computed_pnl_diff = false;
  while (!computed_pnl_diff && !pnl_time_queue.empty()) {
    if ((watch_.msecs_from_midnight() - pnl_time_queue.front().second) < time_diff) {
      pnl_diff_ = total_pnl_ - previous_pnl;
      computed_pnl_diff = true;
    } else if ((watch_.msecs_from_midnight() - pnl_time_queue.front().second) == time_diff) {
      pnl_diff_ = total_pnl_ - pnl_time_queue.front().first;
      computed_pnl_diff = true;
    } else {
      previous_pnl = pnl_time_queue.front().first;
      pnl_time_queue.pop();
    }
  }
  return pnl_diff_;
}

void PromPNL::OnGlobalOrderExec(const unsigned int security_id_, const TradeType_t _buysell_, const int _size_,
                                const double _trade_px_) {
  int r_new_position_ = 0;
  int _exec_quantity_ = _size_;
  double this_trade_price_ = _trade_px_;
  if (_buysell_ == kTradeTypeBuy)
    r_new_position_ = position_ + _size_;
  else
    r_new_position_ = position_ - _size_;

  int abs_change_position_ = abs(r_new_position_ - position_);
  mid_price_ =
      this_trade_price_;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

  if (dep_market_view_.shortcode().find("DI1") != std::string::npos)  // BR_DI_1 we have a generic_ticker
  {
    numbers_to_dollars_ = -GetDIContractNumbersToDollars();
  } else if (dep_market_view_.exch_source() == kExchSourceBMFEQ) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBMFEquityCommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  } else if (dep_market_view_.exch_source() == kExchSourceNSE) {
    commish_dollars_per_unit_ =
        BaseCommish::GetNSECommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  } else if (dep_market_view_.exch_source() == kExchSourceBSE) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBSECommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  }

  if (_buysell_ == kTradeTypeBuy) {  // buy was filled
    if ((position_ < 0) &&
        (r_new_position_ >= 0)) {  // we were short and position and new position are of differetn sign
      // hence we are changing sign
      // break trade into two parts
      // first part to close out earlier position
      int trade1size = -1 * position_;
      pnl_ -= (trade1size * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (trade1size * commish_dollars_per_unit_);

      last_closing_trade_pnl_ = pnl_ - realized_pnl_;
      realized_pnl_ = pnl_;

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d FLAT %s %c %4d %.6f %4d %8d %8d [ %5d %f X %f %5d ]", watch_.tv().tv_sec,
                watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade1size, this_trade_price_, 0,
                0, (int)round(realized_pnl_), dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(),
                dep_market_view_.bestask_price(), dep_market_view_.bestask_size());
        trades_writer_ << buf << '\n';
        trades_writer_.DumpCurrentBuffer();
      }

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);

      if (trade2size == 0) total_pnl_ = pnl_;
      if (trade2size > 0) {
        pnl_ -= (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
        pnl_time_queue.push(std::make_pair(total_pnl_, watch_.msecs_from_midnight()));
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

        {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.6f %4d %8d %8d [ %5d %f X %f %5d ]", watch_.tv().tv_sec,
                  watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade2size, this_trade_price_,
                  r_new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                  dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                  dep_market_view_.bestask_size());
          trades_writer_ << buf << '\n';
          trades_writer_.DumpCurrentBuffer();
        }
      }
    } else {
      pnl_ -= (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
      pnl_time_queue.push(std::make_pair(total_pnl_, watch_.msecs_from_midnight()));
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d OPEN %s %c %4d %.6f %4d %8d %8d [ %5d %f X %f %5d ]", watch_.tv().tv_sec,
                watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), _exec_quantity_, this_trade_price_,
                r_new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                dep_market_view_.bestask_size());
        trades_writer_ << buf << '\n';
        trades_writer_.DumpCurrentBuffer();
      }
    }
  } else {  // _buysell_ == TradeTypeSell ... sell trade
    if ((position_ > 0) &&
        (r_new_position_ <= 0)) {  // we were long and position and new position are of differetn sign
      // hence we are changing sign

      // break trade into two parts
      // first part to close out earlier position
      int trade1size = position_;
      pnl_ += (trade1size * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (trade1size * commish_dollars_per_unit_);

      // since trade closed update realized pnl estimates
      last_closing_trade_pnl_ = pnl_ - realized_pnl_;
      realized_pnl_ = pnl_;

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d FLAT %s %c %4d %.6f %4d %8d %8d [ %5d %f X %f %5d ]", watch_.tv().tv_sec,
                watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade1size, this_trade_price_, 0,
                0, (int)round(realized_pnl_), dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(),
                dep_market_view_.bestask_price(), dep_market_view_.bestask_size());
        trades_writer_ << buf << '\n';
        trades_writer_.DumpCurrentBuffer();
      }

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);
      if (trade2size == 0) total_pnl_ = pnl_;
      if (trade2size > 0) {
        pnl_ += (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
        pnl_time_queue.push(std::make_pair(total_pnl_, watch_.msecs_from_midnight()));
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

        {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.6f %4d %8d %8d [ %5d %f X %f %5d ]", watch_.tv().tv_sec,
                  watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade2size, this_trade_price_,
                  r_new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                  dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                  dep_market_view_.bestask_size());
          trades_writer_ << buf << '\n';
          trades_writer_.DumpCurrentBuffer();
        }
      }
    } else {
      pnl_ += (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (r_new_position_ * mid_price_ * numbers_to_dollars_);
      pnl_time_queue.push(std::make_pair(total_pnl_, watch_.msecs_from_midnight()));
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d OPEN %s %c %4d %.6f %4d %8d %8d [ %5d %f X %f %5d ]", watch_.tv().tv_sec,
                watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), _exec_quantity_, this_trade_price_,
                r_new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                dep_market_view_.bestask_size());
        trades_writer_ << buf << '\n';
        trades_writer_.DumpCurrentBuffer();
      }
    }
  }
  position_ = r_new_position_;
}
}
