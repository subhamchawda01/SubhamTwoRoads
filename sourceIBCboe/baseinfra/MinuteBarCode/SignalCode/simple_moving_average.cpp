/**
    \file MinuteBarCode/SignalCode/simple_moving_average.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "baseinfra/MinuteBar/Signal/simple_moving_average.hpp"

namespace HFSAT {

SimpleMovingAverage* SimpleMovingAverage::GetUniqueInstance(DebugLogger& dbglogger, const Watch& watch,
                                                            const std::vector<const char*>& tokens,
                                                            MinuteBarPriceType base_price_type) {
  // SIGNAL SimpleMovingAverage SHC_0 Duration PriceType
  ShortcodeMinuteBarSMVMap::StaticCheckValid(tokens[1]);
  std::string shortcode = tokens[1];
  int duration = atoi(tokens[2]);
  MinuteBarPriceType price_type = StringToMinuteBarPriceType(tokens[3]);

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << SignalName() << ' ' << shortcode << ' ' << duration << ' ' << tokens[3];
  std::string description(t_temp_oss_.str());

  static std::map<std::string, SimpleMovingAverage*> description_map;
  if (description_map.find(description) == description_map.end()) {
    description_map[description] = new SimpleMovingAverage(dbglogger, watch, shortcode, duration, price_type);
  }
  return description_map[description];
}
SimpleMovingAverage::SimpleMovingAverage(DebugLogger& dbglogger, const Watch& watch, std::string shortcode,
                                         int duration, MinuteBarPriceType price_type)
    : BaseMinuteBarSignal(dbglogger, watch),
      moving_avg_(0.0),
      num_bar_updates_(0),
      security_id_(indexer_.GetIdFromString(shortcode)),
      duration_(duration),
      price_type_(price_type),
      price_list_() {
  SubscribeSecurityID(security_id_);
}

void SimpleMovingAverage::OnBarUpdate(const unsigned int security_id, const DataBar& minute_bar) {
  double moving_sum = moving_avg_ * std::min(num_bar_updates_, (int)duration_);

  while (price_list_.size() >= duration_) {
    double price_removed = price_list_.front();
    price_list_.pop();
    moving_sum -= price_removed;
  }

  double price = minute_bar.GetPriceFromType(price_type_);

  price_list_.push(price);
  moving_sum += price;

  moving_avg_ = moving_sum / std::min(num_bar_updates_ + 1, (int)duration_);

  num_bar_updates_++;
  signal_value_ = moving_avg_;
  NotifyListeners();
}
};
