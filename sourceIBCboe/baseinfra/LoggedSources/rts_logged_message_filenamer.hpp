/**
    \file LoggedSources/eurex_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_MDSMESSAGES_RTS_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_RTS_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

#define RTS_OF_DATE 20161226

namespace HFSAT {

/// Returns the name of the data file where RTS MDS has logged data
/// Typically it will be of the format /NAS1/data/RTSLoggedData/2009/12/01/FESX200912_20091201
class RTSLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "";
    if (_preevent_YYYYMMDD_ >= RTS_OF_DATE) {
      logging_dir = "/NAS1/data/RTS_PFLoggedData/";
    } else {
      logging_dir = "/NAS1/data/RTSLoggedData/";
    }

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocM1);
  }
};
}

#endif  // BASE_MDSMESSAGES_RTS_LOGGED_MESSAGE_FILENAMER_H
