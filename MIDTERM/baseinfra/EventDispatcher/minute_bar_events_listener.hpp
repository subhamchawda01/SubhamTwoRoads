// =====================================================================================
//
//       Filename:  minute_bar_events_listener.hpp
//
//    Description:  A parent listener framework
//
//        Version:  1.0
//        Created:  04/05/2016 05:27:27 PM
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

namespace hftrap {
namespace eventdispatcher {

class MinuteBarEventsListener {
 public:
  virtual void OnBarUpdate(int inst_id_, uint64_t time_, bool is_front_month_, int expiry_date_, double price_,
                           bool more_data_in_bar_) = 0;
  virtual void OnAllEventsConsumed() = 0;
  virtual ~MinuteBarEventsListener() {}
};
}
}
