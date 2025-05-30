/**
    \file SmartOrderRoutingCode/combined_pnl_synthetic.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/SmartOrderRouting/combined_pnl_synthetic.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {

CombinedPnlSynthetic::CombinedPnlSynthetic(DebugLogger& dbglogger, const Watch& watch,
                                           const SecurityMarketView& leg1_smv, const SecurityMarketView& leg2_smv,
                                           PnlWriter* pnl_writer)
    : dbglogger_(dbglogger),
      watch_(watch),
      leg1_smv_(leg1_smv),
      leg2_smv_(leg2_smv),
      leg1_position_(0),
      leg2_position_(0),
      spread_position_(0),
      leg1_commish_(BaseCommish::GetCommishPerContract(leg1_smv_.shortcode(), watch_.YYYYMMDD())),
      leg2_commish_(BaseCommish::GetCommishPerContract(leg2_smv_.shortcode(), watch_.YYYYMMDD())),
      leg1_reserves_(SecurityDefinitions::GetDIReserves(watch_.YYYYMMDD(), leg1_smv.shortcode())),
      leg2_reserves_(SecurityDefinitions::GetDIReserves(watch_.YYYYMMDD(), leg2_smv.shortcode())),
      is_leg1_di1_(false),
      is_leg2_di1_(false),
      leg1_n2d_(SecurityDefinitions::GetContractNumbersToDollars(leg1_smv_.shortcode(), watch_.YYYYMMDD())),
      leg2_n2d_(SecurityDefinitions::GetContractNumbersToDollars(leg2_smv_.shortcode(), watch_.YYYYMMDD())),
      leg1_mkt_price_(0.0),
      leg2_mkt_price_(0.0),
      leg1_average_open_price_(0.0),
      leg2_average_open_price_(0.0),
      total_pnl_(0),
      leg1_pnl_(0),
      leg1_total_pnl_(0),
      leg2_pnl_(0),
      leg2_total_pnl_(0),
      open_or_flat_('O'),
      pnl_writer_(pnl_writer),
      tradingdate_(watch_.YYYYMMDD()) {
  leg1_smv_.ComputeMidPrice();
  shortcode_vec_.push_back(leg1_smv.shortcode());
  shortcode_vec_.push_back(leg2_smv.shortcode());
  dv01_vec_.push_back(-1.0);
  dv01_vec_.push_back(-1.0);

  if (leg1_smv_.shortcode().find("DI1") != std::string::npos) {
    is_leg1_di1_ = true;
  }
  if (leg2_smv_.shortcode().find("DI1") != std::string::npos) {
    is_leg2_di1_ = true;
  }

  leg1_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
  leg2_smv_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

void CombinedPnlSynthetic::OnExec(const int new_position, const int exec_quantity, const TradeType_t buysell,
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

  if (_security_id_ == (int32_t)leg1_smv_.security_id()) {
    position_ = leg1_position_;
    pnl_ = leg1_pnl_;
    t_total_pnl_ = leg1_total_pnl_;
    numbers_to_dollars_ = leg1_n2d_;
    if (is_leg1_di1_) {
      numbers_to_dollars_ = -GetDIContractNumbersToDollars(price, leg1_reserves_);
    }
    commish_dollars_per_unit_ = leg1_commish_;
    average_open_price_ = leg1_average_open_price_;
    bestbid_size_ = leg1_smv_.bestbid_size();
    bestask_size_ = leg1_smv_.bestask_size();
    bestbid_price_ = leg1_smv_.bestbid_price();
    bestask_price_ = leg1_smv_.bestask_price();
    leg1_mkt_price_ = price;  // Using this price as the current mkt price as it will be used in computing pnl
    leg2_mkt_price_ = leg2_smv_.mkt_size_weighted_price();
  } else if (_security_id_ == (int32_t)leg2_smv_.security_id()) {
    position_ = leg2_position_;
    pnl_ = leg2_pnl_;
    t_total_pnl_ = leg2_total_pnl_;
    numbers_to_dollars_ = leg2_n2d_;
    if (is_leg2_di1_) {
      numbers_to_dollars_ = -GetDIContractNumbersToDollars(price, leg2_reserves_);
    }
    commish_dollars_per_unit_ = leg2_commish_;
    average_open_price_ = leg2_average_open_price_;
    bestbid_size_ = leg2_smv_.bestbid_size();
    bestask_size_ = leg2_smv_.bestask_size();
    bestbid_price_ = leg2_smv_.bestbid_price();
    bestask_price_ = leg2_smv_.bestask_price();
    leg2_mkt_price_ = price;
    leg1_mkt_price_ = leg1_smv_.mkt_size_weighted_price();
  } else {
    return;
  }

  int abs_change_position_ = abs(new_position - position_);
  double current_price_ = price;  // on a trade use this trade price to compute unrealized pnl

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

  if (_security_id_ == (int32_t)leg1_smv_.security_id()) {
    // leg1_position_ = position_;
    leg1_total_pnl_ = t_total_pnl_;
    // leg1_pnl_ = pnl_;
    leg1_average_open_price_ = average_open_price_;
  }

  if (_security_id_ == (int32_t)leg2_smv_.security_id()) {
    // leg2_position_ = position_;
    leg2_total_pnl_ = t_total_pnl_;
    // leg2_pnl_ = pnl_;
    leg2_average_open_price_ = average_open_price_;
  }
}

double CombinedPnlSynthetic::GetUnrealizedPnl(int _security_id_, double _pnl_, int _tradesize_) {
  if (_security_id_ == (int32_t)leg1_smv_.security_id()) {
    leg1_pnl_ = _pnl_;
    leg1_position_ = leg1_position_ + _tradesize_;
  } else if (_security_id_ == (int32_t)leg2_smv_.security_id()) {
    leg2_pnl_ = _pnl_;
    leg2_position_ = leg2_position_ + _tradesize_;
  }

  int t_spread_pos_ =
      (leg1_position_ > 0)
          ? std::max(0, std::min(leg1_position_, -1 * (int)(dv01_vec_[1] / dv01_vec_[0] * leg2_position_)))
          : std::min(0, std::max(leg1_position_, -1 * (int)(dv01_vec_[1] / dv01_vec_[0] * leg2_position_)));

  spread_position_ = t_spread_pos_;

  int t_unrealized_pnl_ =
      (leg1_position_ * leg1_mkt_price_ * leg1_n2d_) + (leg2_position_ * leg2_mkt_price_ * leg2_n2d_);

  open_or_flat_ = 'O';
  if (leg1_position_ == 0 && leg2_position_ == 0) {
    open_or_flat_ = 'F';
  }

  return (leg1_pnl_ + leg2_pnl_ + t_unrealized_pnl_);
}

double CombinedPnlSynthetic::GetUnrealizedPnl() {
  int t_unrealized_pnl_ =
      (leg1_position_ * leg1_mkt_price_ * leg1_n2d_) + (leg2_position_ * leg2_mkt_price_ * leg2_n2d_);
  return (leg1_pnl_ + leg2_pnl_ + t_unrealized_pnl_);
}

void CombinedPnlSynthetic::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == leg1_smv_.security_id()) {
    leg1_mkt_price_ = _market_update_info_.mkt_size_weighted_price_;
    if (is_leg1_di1_) {
      leg1_n2d_ = -GetDIContractNumbersToDollars(leg1_mkt_price_, leg1_reserves_);
    }
  } else if (_security_id_ == leg2_smv_.security_id()) {
    leg2_mkt_price_ = _market_update_info_.mkt_size_weighted_price_;
    if (is_leg2_di1_) {
      leg2_n2d_ = -GetDIContractNumbersToDollars(leg2_mkt_price_, leg2_reserves_);
    }
  }
}

void CombinedPnlSynthetic::OnTimePeriodUpdate(const int num_pages_to_add_) {
  dv01_vec_[0] = HFSAT::CurveUtils::stirs_fut_dv01(shortcode_vec_[0], tradingdate_, leg1_mkt_price_);
  dv01_vec_[1] = HFSAT::CurveUtils::stirs_fut_dv01(shortcode_vec_[1], tradingdate_, leg2_mkt_price_);
}

inline double CombinedPnlSynthetic::GetDIContractNumbersToDollars(double _current_price_, int _di_reserves_) {
  double unit_price_ = 0;
  double term_ = double(_di_reserves_ / 252.0);
  if (term_ > 0.000) {
    unit_price_ = 100000 / std::pow((_current_price_ / 100 + 1), term_);
  }
  return (unit_price_ * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD) / _current_price_);
}

int CombinedPnlSynthetic::GetLeg1Position() { return leg1_position_; }

int CombinedPnlSynthetic::GetLeg2Position() { return leg2_position_; }

int CombinedPnlSynthetic::GetSpreadPosition() { return spread_position_; }
}
