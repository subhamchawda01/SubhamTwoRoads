/**
    \file SmartOrderRoutingCode/sim_base_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "baseinfra/SimPnls/sim_base_pnl.hpp"

namespace HFSAT {

SimBasePNL::SimBasePNL(DebugLogger& t_dbglogger_, Watch& r_watch_, 
                       SecurityMarketView& t_dep_market_view_, int t_runtime_id_, BulkFileWriter& t_trades_writer_)
    : BasePNL(t_dbglogger_, r_watch_, t_dep_market_view_, t_runtime_id_),
      trades_writer_(t_trades_writer_),
      max_loss_(10000),  // a very high alue so that tis is not used
      set_cons_total_pnl_(false),
      option_(nullptr),
      fut_smv_(nullptr) {
  is_option_ = NSESecurityDefinitions::IsOption(dep_market_view_.shortcode());
  if (is_option_) {
    option_ = OptionObject::GetUniqueInstance(dbglogger_, watch_, dep_market_view_.shortcode());
    fut_smv_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
        NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(dep_market_view_.shortcode()));
  }
  watch_.subscribe_BigTimePeriod(this);  // to call trades_writer_.DumpCurrentBuffer ()
}

void SimBasePNL::OnTimePeriodUpdate(const int num_pages_to_add_) {
  trades_writer_.DumpCurrentBuffer();
  if ((dep_market_view_.shortcode() == "SGX_CN_0" || dep_market_view_.shortcode() == "SGX_CN_1") &&
      watch_.msecs_from_midnight() >= 32400000) {
    BaseCommish::SetSgxCNCommishPerContract();
    commish_dollars_per_unit_ = BaseCommish::GetCommishPerContract(dep_market_view_.shortcode(), watch_.YYYYMMDD());
  }
}

void SimBasePNL::OnExec(const int r_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                        const double this_trade_price_, const int r_int_price_, const int _security_id_) {
  int new_position_ = r_new_position_;
  int abs_change_position_ = abs(new_position_ - position_);
  current_price_ =
      this_trade_price_;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

  if (is_shc_di1_)  // BR_DI_1 we have a generic_ticker
  {
    numbers_to_dollars_ = -GetDIContractNumbersToDollars();
  }

  if (dep_market_view_.this_smv_exch_source_ == kExchSourceNSE) {
    commish_dollars_per_unit_ =
        BaseCommish::GetNSECommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  } else if (dep_market_view_.this_smv_exch_source_ == kExchSourceBSE) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBSECommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  } else if (dep_market_view_.exch_source() == kExchSourceBMFEQ) {
    commish_dollars_per_unit_ =
        BaseCommish::GetBMFEquityCommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    commish_dollars_per_unit_ = commish_dollars_per_unit_ / _exec_quantity_;
  }

  if (is_asx_shc_) {
    numbers_to_dollars_ = GetASXContractNumbersToDollars();
  }

  std::string end_of_line_ = "\n";

  if (is_option_ && fut_smv_ != nullptr) {
    std::stringstream stream;
    stream << " " << std::fixed << std::setprecision(4) << option_->greeks_.delta_ << " " << option_->greeks_.gamma_
           << " " << option_->greeks_.vega_ << " " << option_->greeks_.theta_ << " " << fut_smv_->mid_price();
    end_of_line_ =
        stream.str() + " " + std::to_string(option_->MktImpliedVol(fut_smv_->mid_price(), this_trade_price_)) + "\n";
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

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, realized_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d ",
                watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade1size,
                this_trade_price_, 0, 0, (int)round(realized_pnl_), dep_market_view_.bestbid_size(),
                dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(), dep_market_view_.bestask_size(),
                mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);
        trades_writer_ << buf << end_of_line_.c_str();
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

        if (base_pnl_listener_ != NULL) {
          base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                          port_base_pnl_, port_risk_);
        }

        {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                  watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade2size,
                  this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                  dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                  dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);
          trades_writer_ << buf << end_of_line_.c_str();
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

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_),
                _exec_quantity_, this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_),
                (int)round(total_pnl_), dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(),
                dep_market_view_.bestask_price(), dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_,
                port_risk_, port_base_pnl_);
        trades_writer_ << buf << end_of_line_.c_str();
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

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, realized_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade1size,
                this_trade_price_, 0, 0, (int)round(realized_pnl_), dep_market_view_.bestbid_size(),
                dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(), dep_market_view_.bestask_size(),
                mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);
        trades_writer_ << buf << end_of_line_.c_str();
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

        if (base_pnl_listener_ != NULL) {
          base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                          port_base_pnl_, port_risk_);
        }

        {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                  watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade2size,
                  this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                  dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                  dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);
          trades_writer_ << buf << end_of_line_.c_str();
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

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        char buf[1024] = {0};
        sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_),
                _exec_quantity_, this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_),
                (int)round(total_pnl_), dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(),
                dep_market_view_.bestask_price(), dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_,
                port_risk_, port_base_pnl_);
        trades_writer_ << buf << end_of_line_.c_str();
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
