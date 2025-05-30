/**
   \file ExecLogic/trade_vars.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_TRADE_VARS_H
#define BASE_EXECLOGIC_TRADE_VARS_H

#include <vector>
#include <sstream>
#include <iostream>
#include <string>

#define HIGH_THRESHOLD_VALUE 100

namespace HFSAT {

/** \brief Struct TradeVars_t represents the trading parameters
 *
 * TradeVars_t to be used at any time are a function of the curent position or current risk
 * The trading parameters are then used to see in what range the target_price_bias should be to
 *    allow placing or canceling of orders at best market
 */
struct TradeVars_t {
  double l1bid_place_;  ///< the threshold that expected profit has to exceed to place a new bid at the top level ( non
  /// self best level )
  double l1bid_keep_;  ///< the threshold that expected profit has to exceed to keep a previously placed bid at the top
  /// level
  double l1ask_place_;  ///< the threshold that expected profit has to exceed to place a new ask at the top level
  double l1ask_keep_;   ///< the threshold that expected profit has to exceed to keep a previously placed bid at the top
  /// level
  double l1bid_improve_;
  double l1bid_improve_keep_;
  double l1bid_aggressive_;
  double l1ask_improve_;
  double l1ask_improve_keep_;
  double l1ask_aggressive_;
  int l1bid_trade_size_;
  int l1ask_trade_size_;

  TradeVars_t()
      : l1bid_place_(HIGH_THRESHOLD_VALUE),
        l1bid_keep_(HIGH_THRESHOLD_VALUE),
        l1ask_place_(HIGH_THRESHOLD_VALUE),
        l1ask_keep_(HIGH_THRESHOLD_VALUE),
        l1bid_improve_(HIGH_THRESHOLD_VALUE),
        l1bid_improve_keep_(HIGH_THRESHOLD_VALUE),
        l1bid_aggressive_(HIGH_THRESHOLD_VALUE),
        l1ask_improve_(HIGH_THRESHOLD_VALUE),
        l1ask_improve_keep_(HIGH_THRESHOLD_VALUE),
        l1ask_aggressive_(HIGH_THRESHOLD_VALUE),
        l1bid_trade_size_(0),
        l1ask_trade_size_(0) {}

  TradeVars_t(double _l1bid_place_, double _l1bid_keep_, double _l1ask_place_, double _l1ask_keep_,
              double _l1bid_improve_, double _l1bid_improve_keep_, double _l1bid_aggressive_, double _l1ask_improve_,
              double _l1ask_improve_keep_, double _l1ask_aggressive_, int _l1bid_trade_size_, int _l1ask_trade_size_)
      : l1bid_place_(_l1bid_place_),
        l1bid_keep_(_l1bid_keep_),
        l1ask_place_(_l1ask_place_),
        l1ask_keep_(_l1ask_keep_),
        l1bid_improve_(_l1bid_improve_),
        l1bid_improve_keep_(_l1bid_improve_keep_),
        l1bid_aggressive_(_l1bid_aggressive_),
        l1ask_improve_(_l1ask_improve_),
        l1ask_improve_keep_(_l1ask_improve_keep_),
        l1ask_aggressive_(_l1ask_aggressive_),
        l1bid_trade_size_(_l1bid_trade_size_),
        l1ask_trade_size_(_l1ask_trade_size_) {}

  inline void Assign(double _l1bid_place_, double _l1bid_keep_, double _l1ask_place_, double _l1ask_keep_,
                     double _l1bid_improve_, double _l1bid_improve_keep_, double _l1bid_aggressive_,
                     double _l1ask_improve_, double _l1ask_improve_keep_, double _l1ask_aggressive_,
                     int _l1bid_trade_size_, int _l1ask_trade_size_) {
    l1bid_place_ = _l1bid_place_;
    l1bid_keep_ = _l1bid_keep_;
    l1ask_place_ = _l1ask_place_;
    l1ask_keep_ = _l1ask_keep_;
    l1bid_improve_ = _l1bid_improve_;
    l1bid_improve_keep_ = _l1bid_improve_keep_;
    l1bid_aggressive_ = _l1bid_aggressive_;
    l1ask_improve_ = _l1ask_improve_;
    l1ask_improve_keep_ = _l1ask_improve_keep_;
    l1ask_aggressive_ = _l1ask_aggressive_;
    l1bid_trade_size_ = _l1bid_trade_size_;
    l1ask_trade_size_ = _l1ask_trade_size_;
  }

  inline void MultiplyBy(const double& r_common_mult_factor_) {
    l1bid_place_ *= r_common_mult_factor_;
    l1bid_keep_ *= r_common_mult_factor_;
    l1ask_place_ *= r_common_mult_factor_;
    l1ask_keep_ *= r_common_mult_factor_;
    l1bid_improve_ *= r_common_mult_factor_;
    l1bid_improve_keep_ *= r_common_mult_factor_;
    l1bid_aggressive_ *= r_common_mult_factor_;
    l1ask_improve_ *= r_common_mult_factor_;
    l1ask_improve_keep_ *= r_common_mult_factor_;
    l1ask_aggressive_ *= r_common_mult_factor_;
  }

  inline void MultiplyBidsBy(const double& r_common_mult_factor_) {
    if (l1bid_place_ > 0) {
      l1bid_place_ *= r_common_mult_factor_;
    }
    if (l1bid_keep_ > 0) {
      l1bid_keep_ *= r_common_mult_factor_;
    }
    if (l1bid_improve_ > 0) {
      l1bid_improve_ *= r_common_mult_factor_;
    }
    if (l1bid_improve_keep_ > 0) {
      l1bid_improve_keep_ *= r_common_mult_factor_;
    }
    if (l1bid_aggressive_ > 0) {
      l1bid_aggressive_ *= r_common_mult_factor_;
    }
  }

  inline void MultiplyAsksBy(const double& r_common_mult_factor_) {
    if (l1ask_place_ > 0) {
      l1ask_place_ *= r_common_mult_factor_;
    }
    if (l1ask_keep_ > 0) {
      l1ask_keep_ *= r_common_mult_factor_;
    }
    if (l1ask_improve_ > 0) {
      l1ask_improve_ *= r_common_mult_factor_;
    }
    if (l1ask_improve_keep_ > 0) {
      l1ask_improve_keep_ *= r_common_mult_factor_;
    }
    if (l1ask_aggressive_ > 0) {
      l1ask_aggressive_ *= r_common_mult_factor_;
    }
  }

  inline void AddBy(const double& r_common_add_factor_) {
    l1bid_place_ += r_common_add_factor_;
    l1bid_keep_ += r_common_add_factor_;
    l1ask_place_ += r_common_add_factor_;
    l1ask_keep_ += r_common_add_factor_;
    l1bid_improve_ += r_common_add_factor_;
    l1bid_improve_keep_ += r_common_add_factor_;
    l1bid_aggressive_ += r_common_add_factor_;
    l1ask_improve_ += r_common_add_factor_;
    l1ask_improve_keep_ += r_common_add_factor_;
    l1ask_aggressive_ += r_common_add_factor_;
  }

  inline void AddBidsBy(const double& r_common_add_factor_) {
    l1bid_place_ += r_common_add_factor_;
    l1bid_keep_ += r_common_add_factor_;
    l1bid_improve_ += r_common_add_factor_;
    l1bid_improve_keep_ += r_common_add_factor_;
    l1bid_aggressive_ += r_common_add_factor_;
  }

  inline void AddPassiveBidsBy(const double& r_common_add_factor_) {
    l1bid_place_ += r_common_add_factor_;
    l1bid_keep_ += r_common_add_factor_;
  }

  inline void AddAggressiveBidsBy(const double& r_common_add_factor_) {
    l1bid_improve_ += r_common_add_factor_;
    l1bid_improve_keep_ += r_common_add_factor_;
    l1bid_aggressive_ += r_common_add_factor_;
  }

  inline void AddAsksBy(const double& r_common_add_factor_) {
    l1ask_place_ += r_common_add_factor_;
    l1ask_keep_ += r_common_add_factor_;
    l1ask_improve_ += r_common_add_factor_;
    l1ask_improve_keep_ += r_common_add_factor_;
    l1ask_aggressive_ += r_common_add_factor_;
  }

  inline void AddPassiveAsksBy(const double& r_common_add_factor_) {
    l1ask_place_ += r_common_add_factor_;
    l1ask_keep_ += r_common_add_factor_;
  }

  inline void AddAggressiveAsksBy(const double& r_common_add_factor_) {
    l1ask_improve_ += r_common_add_factor_;
    l1ask_improve_keep_ += r_common_add_factor_;
    l1ask_aggressive_ += r_common_add_factor_;
  }
};

inline const std::string ToString(const TradeVars_t& trade_vars_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << " BP: " << trade_vars_.l1bid_place_ << " BK: " << trade_vars_.l1bid_keep_
              << " AP: " << trade_vars_.l1ask_place_ << " AK: " << trade_vars_.l1ask_keep_
              << " BI: " << trade_vars_.l1bid_improve_ << " BIK: " << trade_vars_.l1bid_improve_keep_
              << " BA: " << trade_vars_.l1bid_aggressive_ << " AI: " << trade_vars_.l1ask_improve_
              << " AIK: " << trade_vars_.l1ask_improve_keep_ << " AA: " << trade_vars_.l1ask_aggressive_
              << " bidsz: " << trade_vars_.l1bid_trade_size_ << " asksz: " << trade_vars_.l1ask_trade_size_;
  return t_temp_oss_.str();
}

typedef std::vector<TradeVars_t> PositionTradeVarSetMap;
typedef std::vector<TradeVars_t> DeltaTradeVarSetMap;
}
#endif  // BASE_EXECLOGIC_TRADE_VARS_H
