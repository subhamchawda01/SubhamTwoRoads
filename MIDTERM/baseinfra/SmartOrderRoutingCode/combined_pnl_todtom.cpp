/**
    \file SmartOrderRoutingCode/combined_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/SmartOrderRouting/combined_pnl_todtom.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

namespace HFSAT {

CombinedPnlTodTom::CombinedPnlTodTom(DebugLogger& dbglogger, const Watch& watch, const SecurityMarketView& tod_smv,
                                     const SecurityMarketView& tom_smv, const SecurityMarketView& tod_tom_smv,
                                     PnlWriter* pnl_writer)
    : dbglogger_(dbglogger),
      watch_(watch),
      tod_smv_(tod_smv),
      tom_smv_(tom_smv),
      tod_tom_smv_(tod_tom_smv),
      tod_position_(0),
      tom_position_(0),
      tod_tom_position_(0),
      spread_position_(0),
      tod_commish_(BaseCommish::GetCommishPerContract(tod_smv_.shortcode(), watch_.YYYYMMDD())),
      tom_commish_(BaseCommish::GetCommishPerContract(tom_smv_.shortcode(), watch_.YYYYMMDD())),
      tod_tom_commish_(BaseCommish::GetCommishPerContract(tod_tom_smv_.shortcode(), watch_.YYYYMMDD())),
      tod_n2d_(SecurityDefinitions::GetContractNumbersToDollars(tod_smv_.shortcode(), watch_.YYYYMMDD())),
      tom_n2d_(SecurityDefinitions::GetContractNumbersToDollars(tom_smv_.shortcode(), watch_.YYYYMMDD())),
      tod_tom_n2d_(SecurityDefinitions::GetContractNumbersToDollars(tod_tom_smv_.shortcode(), watch_.YYYYMMDD())),
      tod_mkt_price_(0.0),
      tom_mkt_price_(0.0),
      tod_tom_mkt_price_(0.0),
      tod_average_open_price_(0.0),
      tom_average_open_price_(0.0),
      tod_tom_average_open_price_(0.0),
      total_pnl_(0),
      tod_pnl_(0),
      tod_total_pnl_(0),
      tom_pnl_(0),
      tom_total_pnl_(0),
      tod_tom_pnl_(0),
      tod_tom_total_pnl_(0),
      open_or_flat_('O'),
      pnl_writer_(pnl_writer) {}

void CombinedPnlTodTom::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell,
                               const double price, const int int_price, const int _security_id_) {
  int position_ = 0;
  double pnl_ = 0.0;
  double numbers_to_dollars_ = 0.0;
  double commish_dollars_per_unit_ = 0.0;
  double t_total_pnl_ = 0.0;
  double average_open_price_ = 0.0;
  int bestbid_size_ = 0;
  double bestbid_price_ = 0.0;
  int bestask_size_ = 0;
  double bestask_price_ = 0;

  if (_security_id_ == (int32_t)tod_smv_.security_id()) {
    position_ = tod_position_;
    pnl_ = tod_pnl_;
    t_total_pnl_ = tod_total_pnl_;
    numbers_to_dollars_ = tod_n2d_;
    commish_dollars_per_unit_ = tod_commish_;
    average_open_price_ = tod_average_open_price_;
    bestbid_size_ = tod_smv_.bestbid_size();
    bestask_size_ = tod_smv_.bestask_size();
    bestbid_price_ = tod_smv_.bestbid_price();
    bestask_price_ = tod_smv_.bestask_price();
    tod_mkt_price_ = tod_smv_.mkt_size_weighted_price();
  } else if (_security_id_ == (int32_t)tom_smv_.security_id()) {
    position_ = tom_position_;
    pnl_ = tom_pnl_;
    t_total_pnl_ = tom_total_pnl_;
    numbers_to_dollars_ = tom_n2d_;
    commish_dollars_per_unit_ = tom_commish_;
    average_open_price_ = tom_average_open_price_;
    bestbid_size_ = tom_smv_.bestbid_size();
    bestask_size_ = tom_smv_.bestask_size();
    bestbid_price_ = tom_smv_.bestbid_price();
    bestask_price_ = tom_smv_.bestask_price();
    tom_mkt_price_ = tom_smv_.mkt_size_weighted_price();
  } else if (_security_id_ == (int32_t)tod_tom_smv_.security_id()) {
    position_ = tod_tom_position_;
    pnl_ = tod_tom_pnl_;
    t_total_pnl_ = tod_tom_total_pnl_;
    numbers_to_dollars_ = tod_tom_n2d_;
    commish_dollars_per_unit_ = tod_tom_commish_;
    average_open_price_ = tod_tom_average_open_price_;
    bestbid_size_ = tod_tom_smv_.bestbid_size();
    bestask_size_ = tod_tom_smv_.bestask_size();
    bestbid_price_ = tod_tom_smv_.bestbid_price();
    bestask_price_ = tod_tom_smv_.bestask_price();
    tod_tom_mkt_price_ = tod_tom_smv_.mkt_size_weighted_price();
  } else {
    return;
  }

  int abs_change_position_ = abs(new_position - position_);
  double current_price_ = price;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

  if (buysell == kTradeTypeBuy) {                                         // buy was filled
    if ((position_ < 0) && ((long)position_ * (long)new_position) <= 0L)  // needed for MICEX ( lots are size 1000 )
    {  // we were short and position and new position are of differetn sign
      // hence we are changing sign
      // break trade into two parts
      // first part to close out earlier position
      int trade1size = -1 * position_;
      pnl_ -= (trade1size * price * numbers_to_dollars_);
      pnl_ -= (trade1size * commish_dollars_per_unit_);

      total_pnl_ = GetUnrealizedPnl(_security_id_, pnl_, trade1size);

      pnl_writer_->WriteTrade(_security_id_, watch_.tv(), trade1size, price, 0, 0, pnl_, bestbid_size_, bestbid_price_,
                              bestask_price_, bestask_size_, spread_position_, total_pnl_, open_or_flat_,
                              GetTradeTypeChar(buysell));

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);
      if (trade2size == 0) {
        t_total_pnl_ = pnl_;
        average_open_price_ = 0.0;  // FLAT
      }

      if (trade2size > 0) {
        pnl_ -= (trade2size * price * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        average_open_price_ = price;  // open part is entirely at this price.

        total_pnl_ = GetUnrealizedPnl(_security_id_, pnl_, trade2size);

        t_total_pnl_ = pnl_ + (new_position * current_price_ * numbers_to_dollars_);

        pnl_writer_->WriteTrade(_security_id_, watch_.tv(), trade2size, price, new_position, 0, t_total_pnl_,
                                bestbid_size_, bestbid_price_, bestask_price_, bestask_size_, spread_position_,
                                total_pnl_, open_or_flat_, GetTradeTypeChar(buysell));
      }
    } else {
      pnl_ -= (abs_change_position_ * price * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = GetUnrealizedPnl(_security_id_, pnl_, abs_change_position_);

      t_total_pnl_ = pnl_ + (new_position * current_price_ * numbers_to_dollars_);

      average_open_price_ = (average_open_price_ * position_ + (new_position - position_) * price) / (new_position);

      pnl_writer_->WriteTrade(_security_id_, watch_.tv(), abs_change_position_, price, new_position, 0, t_total_pnl_,
                              bestbid_size_, bestbid_price_, bestask_price_, bestask_size_, spread_position_,
                              total_pnl_, open_or_flat_, GetTradeTypeChar(buysell));
    }
  } else {                                                                // _buysell_ == TradeTypeSell ... sell trade
    if ((position_ > 0) && ((long)position_ * (long)new_position) <= 0L)  // needed for MICEX ( lots are size 1000 )
    {  // we were long and position and new position are of differetn sign
      // hence we are changing sign

      // break trade into two parts
      // first part to close out earlier position
      int trade1size = position_;
      pnl_ += (trade1size * price * numbers_to_dollars_);
      pnl_ -= (trade1size * commish_dollars_per_unit_);

      total_pnl_ = GetUnrealizedPnl(_security_id_, pnl_, -trade1size);

      pnl_writer_->WriteTrade(_security_id_, watch_.tv(), trade1size, price, 0, 0, pnl_, bestbid_size_, bestbid_price_,
                              bestask_price_, bestask_size_, spread_position_, total_pnl_, open_or_flat_,
                              GetTradeTypeChar(buysell));

      // remaining part of the trade
      int trade2size = (abs_change_position_ - trade1size);
      if (trade2size == 0) {
        t_total_pnl_ = pnl_;
        average_open_price_ = 0.0;  // FLAT
      }

      if (trade2size > 0) {
        pnl_ += (trade2size * price * numbers_to_dollars_);
        pnl_ -= (trade2size * commish_dollars_per_unit_);

        average_open_price_ = price;  // open part is entirely at this price.

        total_pnl_ = GetUnrealizedPnl(_security_id_, pnl_, -trade2size);
        t_total_pnl_ = pnl_ + (new_position * current_price_ * numbers_to_dollars_);

        pnl_writer_->WriteTrade(_security_id_, watch_.tv(), trade2size, price, new_position, 0, t_total_pnl_,
                                bestbid_size_, bestbid_price_, bestask_price_, bestask_size_, spread_position_,
                                total_pnl_, open_or_flat_, GetTradeTypeChar(buysell));
      }
    } else {
      pnl_ += (abs_change_position_ * price * numbers_to_dollars_);
      pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

      total_pnl_ = GetUnrealizedPnl(_security_id_, pnl_, -abs_change_position_);
      t_total_pnl_ = pnl_ + (new_position * current_price_ * numbers_to_dollars_);

      average_open_price_ = (average_open_price_ * position_ + (new_position - position_) * price) / (new_position);

      pnl_writer_->WriteTrade(_security_id_, watch_.tv(), abs_change_position_, price, new_position, 0, t_total_pnl_,
                              bestbid_size_, bestbid_price_, bestask_price_, bestask_size_, spread_position_,
                              total_pnl_, open_or_flat_, GetTradeTypeChar(buysell));
    }
  }

  if (_security_id_ == (int32_t)tod_smv_.security_id()) {
    // tod_position_ = position_;
    tod_total_pnl_ = t_total_pnl_;
    // tod_pnl_ = pnl_;
    tod_average_open_price_ = average_open_price_;
  }

  if (_security_id_ == (int32_t)tom_smv_.security_id()) {
    // tom_position_ = position_;
    tom_total_pnl_ = t_total_pnl_;
    // tom_pnl_ = pnl_;
    tom_average_open_price_ = average_open_price_;
  }
  if (_security_id_ == (int32_t)tod_tom_smv_.security_id()) {
    // tod_tom_position_ = position_;
    tod_tom_total_pnl_ = t_total_pnl_;
    // tod_tom_pnl_ = pnl_;
    tod_tom_average_open_price_ = average_open_price_;
  }
}

double CombinedPnlTodTom::GetUnrealizedPnl() {
  int t_spread_adj_tod_pos_ = tod_position_ - 100 * tod_tom_position_;
  int t_spread_adj_tom_pos_ = tom_position_ + 100 * tod_tom_position_;
  int matched_pos_ = (t_spread_adj_tom_pos_ > 0)
                         ? std::max(0, std::min(t_spread_adj_tom_pos_, -1 * t_spread_adj_tod_pos_))
                         : std::min(0, std::max(t_spread_adj_tom_pos_, -1 * t_spread_adj_tod_pos_));
  int t_spread_pos_ = matched_pos_ / 100;

  int t_tod_position_ = t_spread_adj_tod_pos_ + 100 * t_spread_pos_;
  int t_tom_position_ = t_spread_adj_tom_pos_ - 100 * t_spread_pos_;

  int t_unrealized_pnl_ = (t_tod_position_ * tod_mkt_price_ * tod_n2d_) +
                          (t_tom_position_ * tom_mkt_price_ * tom_n2d_) +
                          (t_spread_pos_ * tod_tom_mkt_price_ * tod_tom_n2d_);

  return (tod_pnl_ + tom_pnl_ + tod_tom_pnl_ + t_unrealized_pnl_);
}

double CombinedPnlTodTom::GetUnrealizedPnl(int _security_id_, double _pnl_, int _tradesize_) {
  if (_security_id_ == (int32_t)tod_smv_.security_id()) {
    tod_pnl_ = _pnl_;
    tod_position_ = tod_position_ + _tradesize_;
  } else if (_security_id_ == (int32_t)tom_smv_.security_id()) {
    tom_pnl_ = _pnl_;
    tom_position_ = tom_position_ + _tradesize_;
  } else if (_security_id_ == (int32_t)tod_tom_smv_.security_id()) {
    tod_tom_pnl_ = _pnl_;
    tod_tom_position_ = tod_tom_position_ + _tradesize_;
  }

  int t_spread_adj_tod_pos_ = tod_position_ - 100 * tod_tom_position_;
  int t_spread_adj_tom_pos_ = tom_position_ + 100 * tod_tom_position_;
  int matched_pos_ = (t_spread_adj_tom_pos_ > 0)
                         ? std::max(0, std::min(t_spread_adj_tom_pos_, -1 * t_spread_adj_tod_pos_))
                         : std::min(0, std::max(t_spread_adj_tom_pos_, -1 * t_spread_adj_tod_pos_));
  int t_spread_pos_ = matched_pos_ / 100;

  int t_tod_position_ = t_spread_adj_tod_pos_ + 100 * t_spread_pos_;
  int t_tom_position_ = t_spread_adj_tom_pos_ - 100 * t_spread_pos_;

  int t_unrealized_pnl_ = (t_tod_position_ * tod_mkt_price_ * tod_n2d_) +
                          (t_tom_position_ * tom_mkt_price_ * tom_n2d_) +
                          (t_spread_pos_ * tod_tom_mkt_price_ * tod_tom_n2d_);

  open_or_flat_ = 'O';
  if (t_spread_adj_tod_pos_ == 0 && t_spread_adj_tom_pos_ == 0) {
    open_or_flat_ = 'F';
  }

  return (tod_pnl_ + tom_pnl_ + tod_tom_pnl_ + t_unrealized_pnl_);
}

int CombinedPnlTodTom::GetTODPosition() { return tod_position_; }

int CombinedPnlTodTom::GetTOMPosition() { return tom_position_; }

int CombinedPnlTodTom::GetTODTOMPosition() { return tod_tom_position_; }

int CombinedPnlTodTom::GetSpreadPosition() { return spread_position_; }
}
