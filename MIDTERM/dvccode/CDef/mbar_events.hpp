// =====================================================================================
//
//       Filename:  mbar_events.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/05/2016 04:32:59 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Qplum Inc
//
//        Address:  185 Hudson St, #1620
//                  Jersey City, NJ, 07311, USA
//          Phone:  +1 201 377 2302
//
// =====================================================================================

#pragma once

#include <sys/time.h>
#include <sstream>
#include <stdint.h>

namespace hftrap {
namespace defines {

struct MbarEvent {
  time_t event_time;
  char instrument[32];
  time_t first_trade_time;
  time_t last_trade_time;
  uint32_t expiry_date;
  double open_price;
  double close_price;
  double low_price;
  double high_price;
  int32_t total_volume;
  int32_t no_of_trades;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "=================== MINUTE BAR EVENT ===================\n";
    t_temp_oss << "Event_Time     : " << event_time << "\n";
    t_temp_oss << "Instrument     : " << instrument << "\n";
    t_temp_oss << "FirstTradeTime : " << first_trade_time << "\n";
    t_temp_oss << "LastTradeTime  : " << last_trade_time << "\n";
    t_temp_oss << "Expiry         : " << expiry_date << "\n";
    t_temp_oss << "Open           : " << open_price << "\n";
    t_temp_oss << "Close          : " << close_price << "\n";
    t_temp_oss << "Low            : " << low_price << "\n";
    t_temp_oss << "High           : " << high_price << "\n";
    t_temp_oss << "Volume         : " << total_volume << "\n";
    t_temp_oss << "Trades         : " << no_of_trades << "\n\n";

    return t_temp_oss.str();
  }
};
}
}
