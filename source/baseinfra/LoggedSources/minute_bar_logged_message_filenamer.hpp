/**
    \file LoggedSources/minute_bar_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         India
         +91 80 4190 3551
*/
#pragma once

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"

namespace HFSAT {

class MinuteBarLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/MinuteBarLoggedData/";

    std::string t_filename_ = "INVALID_FILE";

    if (rw_trading_location_ == kTLocNSE) {
      t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(
          logging_dir, NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str(),
          _preevent_YYYYMMDD_, rw_trading_location_, rw_trading_location_, true);
      return t_filename_;
    }

    //   std::cout << " DSNAme " << NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str()
    //              << " fname " << t_filename_ << '\n';

    return (HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                   rw_trading_location_, rw_trading_location_, true));
  }
};
}
