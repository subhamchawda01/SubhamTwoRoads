/**
   \file LoggedSources/ors_message_filenamer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_ORSMESSAGES_ORS_MESSAGE_FILENAMER_H
#define BASE_ORSMESSAGES_ORS_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>
#include <algorithm>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
//#include "dvccode/CDef/s3_calls.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

class ORSMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    std::string _preevent_YYYYMMDD_str_ = "";
    {
      std::stringstream ss;
      ss << _preevent_YYYYMMDD_;
      _preevent_YYYYMMDD_str_ = ss.str();
    }

    std::string _preevent_YYYY_str_ = _preevent_YYYYMMDD_str_.substr(0, 4);
    std::string _preevent_MM_str_ = _preevent_YYYYMMDD_str_.substr(4, 2);
    std::string _preevent_DD_str_ = _preevent_YYYYMMDD_str_.substr(6, 2);

    std::string retval_error = "";
    {
      std::stringstream ss;
      ss << "ORS_NO_FILE_AVAILABLE_" << _exchange_symbol_ << "_" << _preevent_YYYYMMDD_str_ << "_"
         << TradingLocationUtils::GetTradingLocationName(rw_trading_location_);  ///< error condition signal
      retval_error = ss.str();
    }

    std::string ORSDIRNAME = "/NAS1/data/ORSData/";

    std::string amr_code_filename_ = _exchange_symbol_;
    std::replace(amr_code_filename_.begin(), amr_code_filename_.end(), ' ', '~');

    return HFSAT::LoggedMessageFileNamer::GetName(ORSDIRNAME, amr_code_filename_.c_str(), _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocNY4);
  }
};
}

#endif  // BASE_ORSMESSAGES_ORS_MESSAGE_FILENAMER_H
