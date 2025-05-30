// =====================================================================================
//
//       Filename:  trade_base_px.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/10/2014 02:08:47 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================
#pragma once

#include <vector>
#include <deque>
#include <string>
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#define VOLUME_DECAY_MAX_LEN 8192

namespace HFSAT {

class TradeBasePxDetails {
 protected:
  TradePrintInfo& trade_print_info_;
  MarketUpdateInfo& market_update_info_;
  const Watch& watch_;
  double volume_threshold_;
  double norm_volume_factor_;
  double vol_decay_factor_;
  double px_decay_factor_;
  double decayed_buy_trade_volume_;
  double decayed_sell_trade_volume_;
  int last_trade_msecs_;
  int last_update_msecs;
  double decay_factor_trades_;
  std::vector<double> volume_decay_vec_;

 public:
  double alpha;
  double decayed_trade_px_;

  TradeBasePxDetails(TradePrintInfo& _trade_inf_, MarketUpdateInfo& _market_inf_, const Watch& _watch_, double* param)
      : trade_print_info_(_trade_inf_),
        market_update_info_(_market_inf_),
        watch_(_watch_),
        volume_threshold_(param[0]),
        norm_volume_factor_(param[1]),
        decayed_buy_trade_volume_(0),
        decayed_sell_trade_volume_(0),
        last_trade_msecs_(0),
        last_update_msecs(0),
        volume_decay_vec_(),
        alpha(0.0),
        decayed_trade_px_(0.0) {
    vol_decay_factor_ = MathUtils::CalcDecayFactor((int)param[2]);
    px_decay_factor_ = MathUtils::CalcDecayFactor((int)param[3]);
    decayed_trade_px_ = market_update_info_.mid_price_;
    last_trade_msecs_ = watch_.msecs_from_midnight();
    last_update_msecs = watch_.msecs_from_midnight();
    decay_factor_trades_ = std::min(0.33, std::max(0.01, (1.0 / ((double)param[4]))));
    for (unsigned i = 0; i < VOLUME_DECAY_MAX_LEN; i++) {
      volume_decay_vec_.push_back(pow(vol_decay_factor_, i));
    }
  }

  inline void OnTradeUpdate() {
    OnMarketUpdate();
    int curr_msecs = watch_.msecs_from_midnight();

    TradeType_t buysell_ = trade_print_info_.buysell_;
    if (buysell_ == kTradeTypeNoInfo) {
      if (trade_print_info_.int_trade_price_ == market_update_info_.pretrade_bestbid_int_price_)
        buysell_ = kTradeTypeSell;
      else if (trade_print_info_.int_trade_price_ == market_update_info_.pretrade_bestask_int_price_)
        buysell_ = kTradeTypeBuy;
    }
    double trade_volume = (buysell_ == kTradeTypeBuy) ? decayed_sell_trade_volume_ : decayed_buy_trade_volume_;

    if (trade_print_info_.size_traded_ + trade_volume <= 0.001) {
      decayed_trade_px_ = market_update_info_.mid_price_;
    } else {
      decayed_trade_px_ =
          trade_print_info_.trade_price_ * decay_factor_trades_ + (1 - decay_factor_trades_) * decayed_trade_px_;
    }
    market_update_info_.decayed_trade_px_ = decayed_trade_px_;

    if (buysell_ == kTradeTypeBuy)
      decayed_buy_trade_volume_ += trade_print_info_.size_traded_;
    else
      decayed_sell_trade_volume_ += trade_print_info_.size_traded_;

    last_trade_msecs_ = curr_msecs;
  }
  inline void OnMarketUpdate() {
    int curr_msecs = watch_.msecs_from_midnight();
    double volume_decay_factor = 0;
    if (curr_msecs - last_update_msecs < VOLUME_DECAY_MAX_LEN) {
      volume_decay_factor = volume_decay_vec_[curr_msecs - last_update_msecs];
    } else {
      volume_decay_factor = 0;
    }

    decayed_sell_trade_volume_ = volume_decay_factor * decayed_sell_trade_volume_;
    decayed_buy_trade_volume_ = volume_decay_factor * decayed_buy_trade_volume_;
    last_update_msecs = curr_msecs;
  }

  inline double GetAlpha1() {
    if (decayed_buy_trade_volume_ + decayed_sell_trade_volume_ <= 0.01) {
      return 0;
    }
    return (fabs(decayed_buy_trade_volume_ - decayed_sell_trade_volume_) /
            (decayed_buy_trade_volume_ + decayed_sell_trade_volume_));
  }
  inline double SigmoidFunction(double x) { return (1 / (1 + exp(-x))); }

  inline void UpdateVolumeAdjustedAlpha() {
    OnMarketUpdate();

    //          double alpha2_ = 0.0 ;
    //          if ( decayed_buy_trade_volume_+ decayed_sell_trade_volume_ - volume_threshold_ > 0 )
    //          {
    //            alpha2_ = ( 1 - 0.5*norm_volume_factor_/( decayed_buy_trade_volume_ + decayed_sell_trade_volume_ +
    //            norm_volume_factor_ - volume_threshold_ ) ) ;
    //          }
    //          else
    //          {
    //            alpha2_ = (0.5 *  norm_volume_factor_ / ( norm_volume_factor_ + volume_threshold_ -
    //            decayed_buy_trade_volume_ - decayed_sell_trade_volume_ )- 1 )  ;
    //          }
    //          alpha = GetAlpha1() * alpha2_ ;

    if (market_update_info_.bestask_int_price_ - market_update_info_.bestbid_int_price_ <= 1) {
      alpha = 0.0;
    } else {
      alpha =
          (GetAlpha1() * SigmoidFunction((decayed_buy_trade_volume_ + decayed_sell_trade_volume_ - volume_threshold_) /
                                         norm_volume_factor_));
    }
    //           std::cerr << watch_.tv() << " " << market_update_info_.bestask_int_price_ <<  " " <<
    //           market_update_info_.bestbid_int_price_ << " GetAlpha " << GetAlpha1 ( ) << " alpha " << alpha << "
    //           trd_decay_fact " << decay_factor_trades_ << " " <<decayed_buy_trade_volume_ << " " <<
    //           decayed_sell_trade_volume_ << " " << last_update_msecs << "\n" ;
  }
};
}
