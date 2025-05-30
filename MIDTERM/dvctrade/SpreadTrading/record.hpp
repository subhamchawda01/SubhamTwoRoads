#ifndef MT_RECORD_HPP
#define MT_RECORD_HPP

#include <stdint.h>
#include <vector>
#include <sstream>

#include <boost/archive/tmpdir.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace MT_SPRD {
struct single_spread_trade_t {
  uint64_t trade_time_;
  char trade_type_;         // 'B' for Buy, 'S' for Sell
  uint16_t num_unit_size_;  // absolute number of UTS bought/sold
  int prev_unit_size_;      // num UTS at time of trade entry
  double trade_spread_;

  // leg1
  int lotsize1_;
  int num_lots1_;
  double price1_;

  // leg2
  int lotsize2_;
  int num_lots2_;
  double price2_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int /*  file_version */) {
    ar& trade_time_& trade_type_& num_unit_size_;
    ar& prev_unit_size_& trade_spread_;
    ar& lotsize1_& num_lots1_& price1_;
    ar& lotsize2_& num_lots2_& price2_;
  }
};

// Corresponds to a round trade [ flat - flat ]
struct spread_trade_record_t {
  std::vector<single_spread_trade_t*> indv_trades_;
  double txn_cost_;
  double cash_position_;
  double mtm_pnl_;      // for closed trades this will store realized trade pnl pre txn cost adjustment
  double initial_nav_;  // used to implement stop gain/stop loss
  uint64_t trade_start_time_;
  uint64_t trade_end_time_;
  bool was_stopped_out_;
  char trade_type_;     //
  bool is_roll_trade_;  // indicates if trade entry/exit is due to roll
  // for debugging
  double cash_pos_1_;
  double cash_pos_2_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int /*  file_version */) {
    ar& indv_trades_;
    ar& txn_cost_& cash_position_& mtm_pnl_;
    ar& initial_nav_& trade_start_time_& trade_end_time_;
    ar& was_stopped_out_& trade_type_& is_roll_trade_;
    ar& cash_pos_1_& cash_pos_2_;
  }
};

// Stores state at a given point of time
struct time_record_t {
  uint64_t time_;
  int pos_;
  double px1_;
  double px2_;
  double inst_spread_;
  double curr_spread_;
  double curr_spread_diff_;
  bool ready_to_trade_;
  double nav_;
  double z_score_;
  double beta_;
  double intercept_;
  double pnlvol_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int /*  file_version */) {
    ar& time_& pos_& px1_& px2_;
    ar& inst_spread_& curr_spread_;
    ar& curr_spread_diff_;
    ar& ready_to_trade_& nav_;
    ar& z_score_;
    ar& beta_;
    ar& intercept_;
    ar& pnlvol_;
  }
};
}
#endif
