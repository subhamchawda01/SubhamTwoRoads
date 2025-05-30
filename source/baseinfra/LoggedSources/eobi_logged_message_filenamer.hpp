/**
    \file LoggedSources/eobi_logged_message_filenamer.hpp

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

#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where EOBI MDS has logged data
/// Typically it will be of the format /NAS1/data/EOBILoggedData/EOBI/2013/11/01/DOLM11_20110501
class EOBILoggedMessageFileNamer {
 public:
  /// Returns the name of the file with binary data of that day.
  /// Searches at the location specified, otherwise in the order specified , searches other locations
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_todays_data_ = false) {
    static std::string logging_dir = "/NAS1/data/EOBINewLoggedData/";

    static std::string todays_logging_dir = "/spare/local/MDSlogs/";

    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;

      t_temp_oss_ << _preevent_YYYYMMDD_;

      return ((todays_logging_dir) + "EOBI/" + std::string(_exchange_symbol_) + "_" + t_temp_oss_.str());
    }

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocFR2);
  }
};
}
