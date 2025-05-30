/**
    \file LoggedSources/retail_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_RETAIL_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_RETAIL_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where RETAIL MDS has logged data
/// Typically it will be of the format /NAS1/data/RETAILLoggedData/2009/12/01/FESX200912_20091201
class RETAILLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/RETAILLoggedData/";
    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocBMF);
  }
};

/// Returns the name of the data file where manual incoming trades have been placed
/// Typically it will be of the format /spare/local/MDSlogs/RETAIL/FESX200912_20091201
class RETAILManualLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/spare/local/MDSlogs/RETAIL/";//directory for temporary files
    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                          rw_trading_location_, kTLocBMF);
  }
};
}

#endif  // BASE_MDSMESSAGES_RETAIL_LOGGED_MESSAGE_FILENAMER_H
