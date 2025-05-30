/**
    \file MinuteBar/minute_bar_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "basetrade/MinuteBar/minute_bar_pnl.hpp"

namespace HFSAT {

MinuteBarPNL::MinuteBarPNL(DebugLogger& _dbglogger_, Watch& _watch_, MinuteBarOrderManager& order_manager,
                           int t_runtime_id_, std::string shortcode, std::string exchange_symbol,
                           BulkFileWriter& t_trades_writer_)
    : trades_writer_(t_trades_writer_),
      dbglogger_(_dbglogger_),
      watch_(_watch_),
      order_manager_(order_manager),
      runtime_id_(t_runtime_id_),
      shortcode_(shortcode),
      exch_source_(SecurityDefinitions::GetContractExchSource(shortcode_, watch_.YYYYMMDD())),
      pnl_(0),
      realized_pnl_(0),
      opentrade_unrealized_pnl_(0),
      min_pnl_till_now_(0),
      total_pnl_(0),
      last_closing_trade_pnl_(0),
      position_(0),
      max_pnl_(0.0),
      drawdown_(0.0),
      commish_dollars_per_unit_(BaseCommish::GetCommishPerContract(shortcode, watch_.YYYYMMDD())),
      retail_commish_dollars_per_unit_(BaseCommish::GetRetailCommishPerContract(shortcode, watch_.YYYYMMDD())),
      di_reserves_(SecurityDefinitions::GetDIReserves(watch_.YYYYMMDD(), shortcode)),
      asx_reserves_(SecurityDefinitions::GetASXReserves(watch_.YYYYMMDD(), shortcode)),
      numbers_to_dollars_(SecurityDefinitions::GetContractNumbersToDollars(shortcode, watch_.YYYYMMDD())),
      current_price_(0),
      average_open_price_(0.0),
      last_bid_price_(0.0),
      last_ask_price_(0.0),
      last_7_trade_lines_(),
      is_shc_di1_(false),
      is_asx_shc_(false),
      asx_bond_face_value_(-1),
      asx_term_(10),
      is_asx_bond_(false),
      is_asx_ir_(false) {
  bzero(numbered_secname_, 24);
  snprintf(numbered_secname_, 24, "%s.%d", exchange_symbol.c_str(), runtime_id_);

  for (size_t i = 0; i < strlen(numbered_secname_); ++i) {
    if (numbered_secname_[i] == ' ') {  // Liffe symbol naming crap
      numbered_secname_[i] = '~';
    }
  }

  if (shortcode_.find("DI1") != std::string::npos && exch_source_ == HFSAT::kExchSourceBMF) {
    is_shc_di1_ = true;
  }

  if (shortcode_ == "XT_0" || shortcode_ == "YT_0") {
    is_asx_shc_ = true;
    is_asx_bond_ = true;
    asx_bond_face_value_ = 100000;

    if (shortcode_ == "XT_0") {
      asx_term_ = 10;
    } else if (shortcode_ == "YT_0") {
      asx_term_ = 3;
    }
  }

  if (shortcode_ == "IR_0" || shortcode_ == "IR_1" || shortcode_ == "IR_2" || shortcode_ == "IR_3" ||
      shortcode_ == "IR_4") {
    is_asx_shc_ = true;
    is_asx_ir_ = true;
    asx_bond_face_value_ = 1000000;
  }

  order_manager_.AddExecutionListener(this);  // On Create ataching itself as a listener to execution events
}

void MinuteBarPNL::OnExec(const int r_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                          const double this_trade_price_, const int r_int_price_, const int _security_id_) {
  int new_position_ = r_new_position_;
  int abs_change_position_ = abs(new_position_ - position_);
  current_price_ =
      this_trade_price_;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

  if (is_shc_di1_)  // BR_DI_1 we have a generic_ticker
  {
    numbers_to_dollars_ = -GetDIContractNumbersToDollars();
  }

  if (exch_source_ == kExchSourceNSE) {
    commish_dollars_per_unit_ = BaseCommish::GetNSECommishPerContract(shortcode_, this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  } else if (exch_source_ == kExchSourceBSE) {
    commish_dollars_per_unit_ = BaseCommish::GetBSECommishPerContract(shortcode_, this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  } else if (exch_source_ == kExchSourceBMFEQ) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBMFEquityCommishPerContract(shortcode_, this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  }

  if (is_asx_shc_) {
    numbers_to_dollars_ = GetASXContractNumbersToDollars();
  }

  if (_buysell_ == kTradeTypeBuy) {                                        // buy was filled
    if ((position_ < 0) && ((long)position_ * (long)new_position_) <= 0L)  // needed for MICEX ( lots are size 1000 )
    {  // we were short and position and new position are of differetn sign
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
        sprintf(buf, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d ", watch_.tv().tv_sec, watch_.tv().tv_usec,
                numbered_secname_, GetTradeTypeChar(_buysell_), trade1size, this_trade_price_, 0, 0,
                (int)round(realized_pnl_));
        trades_writer_ << buf << '\n';
        trades_writer_.CheckToFlushBuffer();

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          last_7_trade_lines_.push_back(buf);
          if (last_7_trade_lines_.size() > 7) {
            last_7_trade_lines_.erase(last_7_trade_lines_.begin());
          }
        }
      }

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);
      if (trade2size == 0) {
        total_pnl_ = pnl_;
        if (total_pnl_ < min_pnl_till_now_) min_pnl_till_now_ = total_pnl_;
        average_open_price_ = 0.0;  // FLAT
      }

      if (trade2size > 0) {
        pnl_ -= (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        average_open_price_ = this_trade_price_;  // open part is entirely at this price.

        total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
        if (total_pnl_ < min_pnl_till_now_) min_pnl_till_now_ = total_pnl_;
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

        {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d ", watch_.tv().tv_sec, watch_.tv().tv_usec,
                  numbered_secname_, GetTradeTypeChar(_buysell_), trade2size, this_trade_price_, new_position_,
                  (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_));

          trades_writer_ << buf << '\n';
          trades_writer_.CheckToFlushBuffer();

          if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
            last_7_trade_lines_.push_back(buf);
            if (last_7_trade_lines_.size() > 7) {
              last_7_trade_lines_.erase(last_7_trade_lines_.begin());
            }
          }
        }
      }
    } else {
      pnl_ -= (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
      if (total_pnl_ < min_pnl_till_now_) min_pnl_till_now_ = total_pnl_;
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

      average_open_price_ =
          (average_open_price_ * position_ + (new_position_ - position_) * this_trade_price_) / (new_position_);

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d", watch_.tv().tv_sec, watch_.tv().tv_usec,
                numbered_secname_, GetTradeTypeChar(_buysell_), _exec_quantity_, this_trade_price_, new_position_,
                (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_));

        trades_writer_ << buf << '\n';
        trades_writer_.CheckToFlushBuffer();

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          last_7_trade_lines_.push_back(buf);
          if (last_7_trade_lines_.size() > 7) {
            last_7_trade_lines_.erase(last_7_trade_lines_.begin());
          }
        }
      }
    }
  } else {                                                                 // _buysell_ == TradeTypeSell ... sell trade
    if ((position_ > 0) && ((long)position_ * (long)new_position_) <= 0L)  // needed for MICEX ( lots are size 1000 )
    {  // we were long and position and new position are of differetn sign
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
        sprintf(buf, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d", watch_.tv().tv_sec, watch_.tv().tv_usec,
                numbered_secname_, GetTradeTypeChar(_buysell_), trade1size, this_trade_price_, 0, 0,
                (int)round(realized_pnl_));
        trades_writer_ << buf << '\n';
        trades_writer_.CheckToFlushBuffer();

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          last_7_trade_lines_.push_back(buf);
          if (last_7_trade_lines_.size() > 7) {
            last_7_trade_lines_.erase(last_7_trade_lines_.begin());
          }
        }
      }

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);
      if (trade2size == 0) {
        total_pnl_ = pnl_;
        if (total_pnl_ < min_pnl_till_now_) min_pnl_till_now_ = total_pnl_;
        average_open_price_ = 0.0;  // FLAT
      }

      if (trade2size > 0) {
        pnl_ += (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        average_open_price_ = this_trade_price_;  // open part is entirely at this price.

        total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
        if (total_pnl_ < min_pnl_till_now_) min_pnl_till_now_ = total_pnl_;
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
        {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d", watch_.tv().tv_sec, watch_.tv().tv_usec,
                  numbered_secname_, GetTradeTypeChar(_buysell_), trade2size, this_trade_price_, new_position_,
                  (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_));

          trades_writer_ << buf << '\n';
          trades_writer_.CheckToFlushBuffer();

          if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
            last_7_trade_lines_.push_back(buf);
            if (last_7_trade_lines_.size() > 7) {
              last_7_trade_lines_.erase(last_7_trade_lines_.begin());
            }
          }
        }
      }
    } else {
      pnl_ += (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
      if (total_pnl_ < min_pnl_till_now_) min_pnl_till_now_ = total_pnl_;
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

      average_open_price_ =
          (average_open_price_ * position_ + (new_position_ - position_) * this_trade_price_) / (new_position_);

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d", watch_.tv().tv_sec, watch_.tv().tv_usec,
                numbered_secname_, GetTradeTypeChar(_buysell_), _exec_quantity_, this_trade_price_, new_position_,
                (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_));

        trades_writer_ << buf << '\n';
        trades_writer_.CheckToFlushBuffer();

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          last_7_trade_lines_.push_back(buf);
          if (last_7_trade_lines_.size() > 7) {
            last_7_trade_lines_.erase(last_7_trade_lines_.begin());
          }
        }
      }
    }
  }

  position_ = new_position_;

  max_pnl_ = std::max(max_pnl_, total_pnl_);
  drawdown_ = max_pnl_ - total_pnl_;
}
}
