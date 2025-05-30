/**
    \file SmartOrderRoutingCode/live_base_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "baseinfra/LivePnls/live_base_pnl.hpp"

namespace HFSAT {

LiveBasePNL::LiveBasePNL(DebugLogger& t_dbglogger_, Watch& r_watch_, SecurityMarketView& t_dep_market_view_,
                         int t_runtime_id_,
                         HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_)
    : BasePNL(t_dbglogger_, r_watch_, t_dep_market_view_, t_runtime_id_),
      log_buffer_(new HFSAT::CDef::LogBuffer()),
      client_logging_segment_initializer_(_client_logging_segment_initializer_) {
  memset((void*)log_buffer_, 0, sizeof(HFSAT::CDef::LogBuffer));
  log_buffer_->content_type_ = HFSAT::CDef::QueryTrade;
  memcpy((void*)log_buffer_->buffer_data_.query_trade_.security_name_, (void*)numbered_secname_, 40);
  watch_.subscribe_BigTimePeriod(this);
}

void LiveBasePNL::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if ((dep_market_view_.shortcode() == "SGX_CN_0" || dep_market_view_.shortcode() == "SGX_CN_1") &&
      watch_.msecs_from_midnight() >= 32400000) {
    BaseCommish::SetSgxCNCommishPerContract();
    commish_dollars_per_unit_ = BaseCommish::GetCommishPerContract(dep_market_view_.shortcode(), watch_.YYYYMMDD());
  }
}

void LiveBasePNL::OnExec(const int r_new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                         const double this_trade_price_, const int r_int_price_, const int _security_id_,
                         const int _caos_) {
  int new_position_ = r_new_position_;
  int abs_change_position_ = abs(new_position_ - position_);
  current_price_ =
      this_trade_price_;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

  double t_commish_dollars_per_unit_ = commish_dollars_per_unit_;

  if (is_shc_di1_)  // BR_DI_1 we have a generic_ticker
  {
    numbers_to_dollars_ = -GetDIContractNumbersToDollars();
  } else if (dep_market_view_.exch_source() == kExchSourceBMFEQ) {
    t_commish_dollars_per_unit_ =
        BaseCommish::GetBMFEquityCommishPerContract(dep_market_view_.shortcode(), this_trade_price_, _exec_quantity_);
    t_commish_dollars_per_unit_ = t_commish_dollars_per_unit_ / _exec_quantity_;
  } else if ((dep_market_view_.exch_source() == kExchSourceNSE) || (dep_market_view_.exch_source() == kExchSourceBSE)) {
    t_commish_dollars_per_unit_ = commish_dollars_per_unit_ * this_trade_price_;
  }

  if(watch_.tv().tv_sec == 0) {
    t_commish_dollars_per_unit_ = 0;
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
      pnl_ -= (trade1size * t_commish_dollars_per_unit_);

      last_closing_trade_pnl_ = pnl_ - realized_pnl_;
      realized_pnl_ = pnl_;

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, realized_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
        log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
        log_buffer_->buffer_data_.query_trade_.trade_size_ = trade1size;
        log_buffer_->buffer_data_.query_trade_.trade_price_ = this_trade_price_;
        log_buffer_->buffer_data_.query_trade_.new_position_ = 0;
        log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = 0;
        log_buffer_->buffer_data_.query_trade_.total_pnl_ = (int)round(realized_pnl_);
        log_buffer_->buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size();
        log_buffer_->buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price();
        log_buffer_->buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price();
        log_buffer_->buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size();
        log_buffer_->buffer_data_.query_trade_.mult_risk_ = mult_risk_;
        log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.port_risk_ = port_risk_;
        log_buffer_->buffer_data_.query_trade_.port_base_pnl_ = port_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'F';
        log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(_buysell_);

        client_logging_segment_initializer_->Log(log_buffer_);

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                  watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade1size,
                  this_trade_price_, 0, 0, (int)round(realized_pnl_), dep_market_view_.bestbid_size(),
                  dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(), dep_market_view_.bestask_size(),
                  mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);

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
        if (total_pnl_ < min_pnl_till_now_) {
          min_pnl_till_now_ = total_pnl_;
        }
        average_open_price_ = 0.0;  // FLAT
      }

      if (trade2size > 0) {
        pnl_ -= (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * t_commish_dollars_per_unit_);

        average_open_price_ = this_trade_price_;  // open part is entirely at this price.

        total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
        if (total_pnl_ < min_pnl_till_now_) {
          min_pnl_till_now_ = total_pnl_;
        }
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

        if (base_pnl_listener_ != NULL) {
          base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                          port_base_pnl_, port_risk_);
        }

        {
          log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
          log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
          log_buffer_->buffer_data_.query_trade_.trade_size_ = trade2size;
          log_buffer_->buffer_data_.query_trade_.trade_price_ = this_trade_price_;
          log_buffer_->buffer_data_.query_trade_.new_position_ = new_position_;
          log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = (int)round(opentrade_unrealized_pnl_);
          log_buffer_->buffer_data_.query_trade_.total_pnl_ = (int)round(total_pnl_);
          log_buffer_->buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size();
          log_buffer_->buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price();
          log_buffer_->buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price();
          log_buffer_->buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size();
          log_buffer_->buffer_data_.query_trade_.mult_risk_ = mult_risk_;
          log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_;
          log_buffer_->buffer_data_.query_trade_.port_risk_ = port_risk_;
          log_buffer_->buffer_data_.query_trade_.port_base_pnl_ = port_base_pnl_;
          log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'O';
          log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(_buysell_);

          client_logging_segment_initializer_->Log(log_buffer_);

          if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
            char buf[1024] = {0};
            sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                    watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade2size,
                    this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                    dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                    dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);

            last_7_trade_lines_.push_back(buf);
            if (last_7_trade_lines_.size() > 7) {
              last_7_trade_lines_.erase(last_7_trade_lines_.begin());
            }
          }
        }
      }
    } else {
      pnl_ -= (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * t_commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
      if (total_pnl_ < min_pnl_till_now_) {
        min_pnl_till_now_ = total_pnl_;
      }
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

      average_open_price_ =
          (average_open_price_ * position_ + (new_position_ - position_) * this_trade_price_) / (new_position_);

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
        log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
        log_buffer_->buffer_data_.query_trade_.trade_size_ = _exec_quantity_;
        log_buffer_->buffer_data_.query_trade_.trade_price_ = this_trade_price_;
        log_buffer_->buffer_data_.query_trade_.new_position_ = new_position_;
        log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = (int)round(opentrade_unrealized_pnl_);
        log_buffer_->buffer_data_.query_trade_.total_pnl_ = (int)round(total_pnl_);
        log_buffer_->buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size();
        log_buffer_->buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price();
        log_buffer_->buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price();
        log_buffer_->buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size();
        log_buffer_->buffer_data_.query_trade_.mult_risk_ = mult_risk_;
        log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.port_risk_ = port_risk_;
        log_buffer_->buffer_data_.query_trade_.port_base_pnl_ = port_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'O';
        log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(_buysell_);

        client_logging_segment_initializer_->Log(log_buffer_);

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                  watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_),
                  _exec_quantity_, this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_),
                  (int)round(total_pnl_), dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(),
                  dep_market_view_.bestask_price(), dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_,
                  port_risk_, port_base_pnl_);

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
      pnl_ -= (trade1size * t_commish_dollars_per_unit_);

      // since trade closed update realized pnl estimates
      last_closing_trade_pnl_ = pnl_ - realized_pnl_;
      realized_pnl_ = pnl_;

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, realized_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
        log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
        log_buffer_->buffer_data_.query_trade_.trade_size_ = trade1size;
        log_buffer_->buffer_data_.query_trade_.trade_price_ = this_trade_price_;
        log_buffer_->buffer_data_.query_trade_.new_position_ = 0;
        log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = 0;
        log_buffer_->buffer_data_.query_trade_.total_pnl_ = (int)round(realized_pnl_);
        log_buffer_->buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size();
        log_buffer_->buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price();
        log_buffer_->buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price();
        log_buffer_->buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size();
        log_buffer_->buffer_data_.query_trade_.mult_risk_ = mult_risk_;
        log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.port_risk_ = port_risk_;
        log_buffer_->buffer_data_.query_trade_.port_base_pnl_ = port_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'F';
        log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(_buysell_);

        client_logging_segment_initializer_->Log(log_buffer_);

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                  watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade1size,
                  this_trade_price_, 0, 0, (int)round(realized_pnl_), dep_market_view_.bestbid_size(),
                  dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(), dep_market_view_.bestask_size(),
                  mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);

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
        if (total_pnl_ < min_pnl_till_now_) {
          min_pnl_till_now_ = total_pnl_;
        }
        average_open_price_ = 0.0;  // FLAT
      }

      if (trade2size > 0) {
        pnl_ += (trade2size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade2size * t_commish_dollars_per_unit_);

        average_open_price_ = this_trade_price_;  // open part is entirely at this price.

        total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
        if (total_pnl_ < min_pnl_till_now_) {
          min_pnl_till_now_ = total_pnl_;
        }
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

        if (base_pnl_listener_ != NULL) {
          base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                          port_base_pnl_, port_risk_);
        }

        {
          log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
          log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
          log_buffer_->buffer_data_.query_trade_.trade_size_ = trade2size;
          log_buffer_->buffer_data_.query_trade_.trade_price_ = this_trade_price_;
          log_buffer_->buffer_data_.query_trade_.new_position_ = new_position_;
          log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = (int)round(opentrade_unrealized_pnl_);
          log_buffer_->buffer_data_.query_trade_.total_pnl_ = (int)round(total_pnl_);
          log_buffer_->buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size();
          log_buffer_->buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price();
          log_buffer_->buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price();
          log_buffer_->buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size();
          log_buffer_->buffer_data_.query_trade_.mult_risk_ = mult_risk_;
          log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_;
          log_buffer_->buffer_data_.query_trade_.port_risk_ = port_risk_;
          log_buffer_->buffer_data_.query_trade_.port_base_pnl_ = port_base_pnl_;
          log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'O';
          log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(_buysell_);

          client_logging_segment_initializer_->Log(log_buffer_);

          if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
            char buf[1024] = {0};
            sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                    watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_), trade2size,
                    this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_), (int)round(total_pnl_),
                    dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(), dep_market_view_.bestask_price(),
                    dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);

            last_7_trade_lines_.push_back(buf);
            if (last_7_trade_lines_.size() > 7) {
              last_7_trade_lines_.erase(last_7_trade_lines_.begin());
            }
          }
        }
      }
    } else {
      pnl_ += (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * t_commish_dollars_per_unit_);

      total_pnl_ = pnl_ + (new_position_ * current_price_ * numbers_to_dollars_);
      if (total_pnl_ < min_pnl_till_now_) {
        min_pnl_till_now_ = total_pnl_;
      }
      opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

      average_open_price_ =
          (average_open_price_ * position_ + (new_position_ - position_) * this_trade_price_) / (new_position_);

      if (base_pnl_listener_ != NULL) {
        base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_,
                                        port_base_pnl_, port_risk_);
      }

      {
        log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
        log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
        log_buffer_->buffer_data_.query_trade_.trade_size_ = _exec_quantity_;
        log_buffer_->buffer_data_.query_trade_.trade_price_ = this_trade_price_;
        log_buffer_->buffer_data_.query_trade_.new_position_ = new_position_;
        log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = (int)round(opentrade_unrealized_pnl_);
        log_buffer_->buffer_data_.query_trade_.total_pnl_ = (int)round(total_pnl_);
        log_buffer_->buffer_data_.query_trade_.bestbid_size_ = dep_market_view_.bestbid_size();
        log_buffer_->buffer_data_.query_trade_.bestbid_price_ = dep_market_view_.bestbid_price();
        log_buffer_->buffer_data_.query_trade_.bestask_price_ = dep_market_view_.bestask_price();
        log_buffer_->buffer_data_.query_trade_.bestask_size_ = dep_market_view_.bestask_size();
        log_buffer_->buffer_data_.query_trade_.mult_risk_ = mult_risk_;
        log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.port_risk_ = port_risk_;
        log_buffer_->buffer_data_.query_trade_.port_base_pnl_ = port_base_pnl_;
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'O';
        log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(_buysell_);

        client_logging_segment_initializer_->Log(log_buffer_);

        if (dbglogger_.CheckLoggingLevel(TRADEINIT_INFO)) {
          char buf[1024] = {0};
          sprintf(buf, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d",
                  watch_.tv().tv_sec, watch_.tv().tv_usec, numbered_secname_, GetTradeTypeChar(_buysell_),
                  _exec_quantity_, this_trade_price_, new_position_, (int)round(opentrade_unrealized_pnl_),
                  (int)round(total_pnl_), dep_market_view_.bestbid_size(), dep_market_view_.bestbid_price(),
                  dep_market_view_.bestask_price(), dep_market_view_.bestask_size(), mult_risk_, mult_base_pnl_,
                  port_risk_, port_base_pnl_);

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
