/**
   \file MDSMessage/aflash_logged_message_filenamer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_AFLASH_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_AFLASH_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where AFLASH MDS has logged data
/// Typically it will be of the format /NAS1/data/AFLASHLoggedData/NY4/2010/12/01/ESM1_20101201
class AFLASHLoggedMessageFileNamer {
 public:
  static std::string GetName(const unsigned int _preevent_YYYYMMDD_, TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/AFLASHLoggedData/";
    static std::string exchange_symbol_ = "AFL";

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, exchange_symbol_.c_str(), _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocNY4);
  }
};
}

#endif  // BASE_MDSMESSAGES_AFLASH_LOGGED_MESSAGE_FILENAMER_H
