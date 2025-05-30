// =====================================================================================
//
//       Filename:  vol_utils.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/02/2015 01:48:05 PM
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

// Some common functions used in vol-trader/similar exec logics
//
//
#pragma once
#include <string>
#include <vector>

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"

namespace HFSAT {
namespace ExecLogicUtils {

inline void CancelOrdersInBand(SmartOrderManager* _order_manager_, const int _high_lvl_price_,
                               const int _low_lvl_price_, const int _target_size_, int& _ordered_size_,
                               const TradeType_t _side_) {
  if (_side_ == kTradeTypeBuy) {
    int cancel_requested_size_ =
        _order_manager_->SumBidSizeCancelRequestedInRange(_high_lvl_price_, _low_lvl_price_ + 1);
    if (_ordered_size_ - cancel_requested_size_ > _target_size_) {
      // Cancel orders here
      int retval_ = _order_manager_->CancelBidsInRange(_high_lvl_price_, _low_lvl_price_ + 1,
                                                       _ordered_size_ - cancel_requested_size_ - _target_size_);

      if (retval_ > 0) {
        _ordered_size_ -= retval_;
      }
    }
  } else if (_side_ == kTradeTypeSell) {
    int cancel_requested_size_ =
        _order_manager_->SumAskSizeCancelRequestedInRange(_high_lvl_price_, _low_lvl_price_ - 1);
    if (_ordered_size_ - cancel_requested_size_ > _target_size_) {
      int retval_ =
          _order_manager_->CancelAsksInRange(_high_lvl_price_, _low_lvl_price_ - 1, _ordered_size_ - _target_size_);
      if (retval_ > 0) {
        _ordered_size_ -= retval_;
      }
    }
  }
}

inline void GetLowBandPx(const SecurityMarketView* p_dep_market_view_, int& _low_band_px_, const int _base_iter_px_,
                         const TradeType_t _side_) {
  if (_side_ == kTradeTypeBuy) {
    int t_int_price_ = _base_iter_px_;
    int size_seen_ = 0, orders_seen_ = 0;
    int this_index_ = p_dep_market_view_->get_next_stable_bid_level_index(
        p_dep_market_view_->GetBaseBidMapIndex(), p_dep_market_view_->level_size_thresh_, size_seen_, orders_seen_);
    while (t_int_price_ < p_dep_market_view_->market_update_info_.bidlevels_[this_index_].limit_int_price_ &&
           this_index_ > 0) {
      this_index_ = p_dep_market_view_->get_next_stable_bid_level_index(
          this_index_ - 1, p_dep_market_view_->level_size_thresh_, size_seen_, orders_seen_);
    }
    _low_band_px_ = p_dep_market_view_->market_update_info_.bidlevels_[this_index_].limit_int_price_;
    _low_band_px_--;
  } else {
    int t_int_price_ = _base_iter_px_;
    int size_seen_ = 0, orders_seen_ = 0;
    int this_index_ = p_dep_market_view_->get_next_stable_ask_level_index(
        p_dep_market_view_->GetBaseAskMapIndex(), p_dep_market_view_->level_size_thresh_, size_seen_, orders_seen_);
    while (t_int_price_ > p_dep_market_view_->market_update_info_.asklevels_[this_index_].limit_int_price_ &&
           this_index_ > 0) {
      this_index_ = p_dep_market_view_->get_next_stable_ask_level_index(
          this_index_ - 1, p_dep_market_view_->level_size_thresh_, size_seen_, orders_seen_);
    }
    _low_band_px_ = p_dep_market_view_->market_update_info_.asklevels_[this_index_].limit_int_price_;
    _low_band_px_++;
  }
}
}
}
