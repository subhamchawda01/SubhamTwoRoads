/**
    \file OptionsUtilsCode/option_object.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "baseinfra/OptionsUtils/option_object.hpp"

namespace HFSAT {
std::map<std::string, OptionObject*> OptionObject::option_objects_map_;

CumulativeNormalDistribution* OptionObject::cdf_ = new CumulativeNormalDistribution();

OptionObject::OptionObject(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _option_name_)
    : dbglogger_(_dbglogger_), watch_(_watch_), option_name_(_option_name_) {
  if (OptionType_t::CALL == (OptionType_t)SecurityDefinitions::GetOptionType(option_name_)) {
    is_call_q_ = 1;
  } else {
    is_call_q_ = -1;
  }

  strike_ = SecurityDefinitions::GetStrikePriceFromShortCode(option_name_);
  interest_rate_ = SecurityDefinitions::GetInterestRate(watch_.YYYYMMDD());

  const int expiry_utc_date_ = SecurityDefinitions::GetExpiryFromShortCode(option_name_);
  const int expiry_utc_time_ = 1000;
  double num_secs_to_expiry_from_midnight_utc_ = difftime(DateTime::GetTimeUTC(expiry_utc_date_, expiry_utc_time_),
                                                          DateTime::GetTimeMidnightUTC(watch_.YYYYMMDD()));
  expiry_minus_todays_midnight_in_years_ = SEC_IN_YEARS * num_secs_to_expiry_from_midnight_utc_;
  current_time_minus_midnight_in_years_ = watch_.msecs_from_midnight() * MSEC_IN_YEARS;

  greeks_.delta_ = 0;
  greeks_.gamma_ = 0;
  greeks_.vega_ = 0;
  greeks_.theta_ = 0;
}

// unique is directly linked to option_name
OptionObject* OptionObject::GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              std::string _option_name_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << " " << _option_name_;
  std::string map_key_(t_temp_oss_.str());

  if (option_objects_map_.find(map_key_) == option_objects_map_.end()) {
    option_objects_map_[map_key_] = new OptionObject(_dbglogger_, _watch_, _option_name_);
  }
  return option_objects_map_[map_key_];
}
}
