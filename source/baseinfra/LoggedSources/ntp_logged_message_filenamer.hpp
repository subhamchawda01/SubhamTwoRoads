/**
   \file MDSMessage/ntp_logged_message_filenamer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MDSMESSAGES_NTP_LOGGED_MESSAGE_FILENAMER_H
#define BASE_MDSMESSAGES_NTP_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where NTP MDS has logged data
/// Typically it will be of the format /NAS1/data/NTPLoggedData/NTP/2010/12/01/ESM1_20101201
class NTPLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_ntp_order_book_ = false,
                             bool use_bmf_equity_ = false) {
    static std::string logging_dir_ord = "/NAS1/data/NTP_ORDLoggedData/";
    static std::string logging_dir_price = "/NAS1/data/NTPLoggedData/";
    std::string logging_dir_equity = "/NAS1/data/BMFLoggedData/";

    if (_preevent_YYYYMMDD_ > 20140306) {
      logging_dir_equity = "/NAS1/data/PUMALoggedData/";
    }

    if (use_ntp_order_book_)
      return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_ord, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                    rw_trading_location_, kTLocBMF);
    else if (use_bmf_equity_)
      return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_equity, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                    rw_trading_location_, kTLocBMF);
    else
      return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_price, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                    rw_trading_location_, kTLocBMF);
  }
};
}

#endif  // BASE_MDSMESSAGES_NTP_LOGGED_MESSAGE_FILENAMER_H
