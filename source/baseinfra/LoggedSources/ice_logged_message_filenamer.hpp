/**
    \file LoggedSources/ice_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_MDSMESSAGES_ICE_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_ICE_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where ICE MDS has logged data
/// Typically it will be of the format /NAS1/data/ICELoggedData/BSL/2014/11/24/R~~~FMZ0014!
class ICELoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_todays_data_ = false) {
    static std::string logging_dir = "/NAS1/data/ICELoggedData/";
    std::string amr_code_filename_ = _exchange_symbol_;

    std::replace(amr_code_filename_.begin(), amr_code_filename_.end(), ' ', '~');

    static std::string todays_logging_dir = "/spare/local/MDSlogs/";

    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;

      t_temp_oss_ << _preevent_YYYYMMDD_;

      return ((todays_logging_dir) + "ICE/" + std::string(amr_code_filename_) + "_" + t_temp_oss_.str());
    }

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, amr_code_filename_.c_str(), _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocBSL);
  }
};
}

#endif  // BASE_MDSMESSAGES_ICE_LOGGED_MESSAGE_FILENAMER_H
