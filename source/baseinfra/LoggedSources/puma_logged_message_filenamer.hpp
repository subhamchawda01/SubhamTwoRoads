/**
   \file MDSMessage/puma_logged_message_filenamer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where PUMA MDS has logged data
/// Typically it will be of the format /NAS1/data/PUMALoggedData/NTP/2014/03/07/VALE3_20140307.gz
class PUMALoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_ntp_order_book_ = false,
                             bool use_bmf_equity_ = false) {
    static std::string logging_dir_ = "/NAS1/data/PUMALoggedData/";

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocBMF);
  }
};
}
