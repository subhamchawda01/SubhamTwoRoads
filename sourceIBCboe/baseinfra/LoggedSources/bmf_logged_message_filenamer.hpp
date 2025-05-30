/**
    \file LoggedSources/bmf_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_BMF_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_BMF_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where BMF MDS has logged data
/// Typically it will be of the format /NAS1/data/BMFLoggedData/BMF/2011/05/01/DOLM11_20110501
class BMFLoggedMessageFileNamer {
 public:
  /// Returns the name of the file with binary data of that day.
  /// Searches at the location specified, otherwise in the order specified , searches other locations and
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/BMFLoggedData/";
    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocBMF);
  }
};
}

#endif  // BASE_MDSMESSAGES_BMF_LOGGED_MESSAGE_FILENAMER_H
