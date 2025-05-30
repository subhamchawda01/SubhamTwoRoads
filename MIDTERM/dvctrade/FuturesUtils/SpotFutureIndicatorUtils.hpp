/**
   \file FuturesUtils/SpotFutureIndicatorUtils.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>  //atoi
//#include <iomanip>  //setfill

#include <map>
#include <vector>

#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/defines.hpp"

#define SPF_DAY_COUNT 25200

namespace HFSAT {

class SpotFutureIndicatorUtils {
 public:
  SpotFutureIndicatorUtils(int, std::string, std::string, std::string);

  // static functions

  static inline unsigned int GetDaysToExpiry(unsigned int tradingdate_, std::string shortcode_) {
    unsigned int expiry_date_ = GetExpiry(shortcode_);
    unsigned int days_to_expiry_ = 0;
    unsigned int current_date_ = expiry_date_;
    while (current_date_ != tradingdate_) {
      current_date_ = HFSAT::DateTime::CalcPrevDay(current_date_);
      days_to_expiry_++;
    }
    return days_to_expiry_;
  }

  static inline unsigned int GetExpiry(std::string shortcode_) {
    std::string exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    if (exch_symbol_.substr(2, 2) == "Z3") {
      return 20131216;
    } else if (exch_symbol_.substr(2, 2) == "Z4") {
      return 20141215;
    } else if (exch_symbol_.substr(2, 2) == "X3") {
      return 20131115;
    } else if (exch_symbol_.substr(2, 2) == "X4") {
      return 20141117;
    } else if (exch_symbol_.substr(2, 2) == "V3") {
      return 20131015;
    } else if (exch_symbol_.substr(2, 2) == "V4") {
      return 20141015;
    } else if (exch_symbol_.substr(2, 2) == "U3") {
      return 20130916;
    } else if (exch_symbol_.substr(2, 2) == "U4") {
      return 20140915;
    } else if (exch_symbol_.substr(2, 2) == "Q3") {
      return 20130815;
    } else if (exch_symbol_.substr(2, 2) == "Q4") {
      return 20140815;
    } else if (exch_symbol_.substr(2, 2) == "N3") {
      return 20130715;
    } else if (exch_symbol_.substr(2, 2) == "N4") {
      return 20140715;
    } else if (exch_symbol_.substr(2, 2) == "M3") {
      return 20130615;
    } else if (exch_symbol_.substr(2, 2) == "M4") {
      return 20140615;
    } else if (exch_symbol_.substr(2, 2) == "K4") {
      return 20140515;
    } else if (exch_symbol_.substr(2, 2) == "J4") {
      return 20140415;
    } else if (exch_symbol_.substr(2, 2) == "H4") {
      return 20140317;
    } else if (exch_symbol_.substr(2, 2) == "G4") {
      return 20140217;
    } else if (exch_symbol_.substr(2, 2) == "F4") {
      return 20140115;
    } else {
      return 20110101;
    }
  }
};
}
