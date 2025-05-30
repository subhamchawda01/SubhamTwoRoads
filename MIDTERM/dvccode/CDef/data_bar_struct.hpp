#ifndef _DATA_BAR_STRUCT_
#define _DATA_BAR_STRUCT_

#include <string>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"

namespace HFSAT {

struct DataBar {
  unsigned int tv_sec_;
  unsigned int dur_in_secs_;
  float open_bid_px_;
  float open_ask_px_;
  float close_bid_px_;
  float close_ask_px_;
  float high_px_;
  float low_px_;
  unsigned int vol_trd_;
  float avg_trd_px_;
  unsigned int num_events_;
  unsigned int num_trds_;

  DataBar()
      : tv_sec_(0),
        dur_in_secs_(60),
        open_bid_px_(0),
        open_ask_px_(0),
        close_bid_px_(0),
        close_ask_px_(0),
        high_px_(0.0),
        low_px_(0.0),
        vol_trd_(0),
        avg_trd_px_(0.0),
        num_events_(0),
        num_trds_(0) {}

  std::string ToString() {
    std::stringstream ss;
    ss << tv_sec_ << " ,Open: [ " << open_bid_px_ << " " << open_ask_px_ << " ] ,Close: [ " << close_bid_px_ << " "
       << close_ask_px_ << " ],High: " << high_px_ << " ,Low: " << low_px_ << " ,Vol: " << vol_trd_
       << " ,Avg_Trd_Px: " << avg_trd_px_ << " ,Events: " << num_events_ << " ,Trades: " << num_trds_ << "\n";
    return ss.str();
  }

  double GetPriceFromType(MinuteBarPriceType price_type) const {
    double price;
    switch (price_type) {
      case MinuteBarPriceType::kPriceTypeOpen: {
        price = (open_bid_px_ + open_ask_px_) / 2;
      } break;
      case MinuteBarPriceType::kPriceTypeHigh: {
        price = high_px_;
      } break;
      case MinuteBarPriceType::kPriceTypeLow: {
        price = low_px_;
      } break;
      case MinuteBarPriceType::kPriceTypeClose: {
        price = (close_bid_px_ + close_ask_px_) / 2;
      } break;
      case MinuteBarPriceType::kPriceTypeAvgTrade: {
        price = avg_trd_px_;
      } break;
      default: {
        std::cerr << "Invalid Price Type: " << (int)price_type << " . Using Close Px." << std::endl;
        price = (close_bid_px_ + close_ask_px_) / 2;
      }
    }
    return price;
  }
};
}
#endif  // _DATA_BAR_STRUCT_
