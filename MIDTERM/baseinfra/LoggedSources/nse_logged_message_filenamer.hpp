/**
    \file LoggedSources/nse_logged_message_filenamer.hpp

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

class NSELoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/NSELoggedData/";

    std::string t_filename_ = "INVALID_FILE";

    t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(
        logging_dir, NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str(),
        _preevent_YYYYMMDD_, rw_trading_location_, kTLocNSE);

    //   std::cout << " DSNAme " << NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str()
    //              << " fname " << t_filename_ << '\n';

    if (rw_trading_location_ != kTLocMAX) {
      return t_filename_;
    } else {
      std::string t_str_ = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_);
      std::size_t t_index_ = t_str_.find_last_of("_");
      return (HFSAT::LoggedMessageFileNamer::GetName(logging_dir, t_str_.substr(0, t_index_).c_str(),
                                                     _preevent_YYYYMMDD_, rw_trading_location_, kTLocNSE));
    }
  }
};
}
