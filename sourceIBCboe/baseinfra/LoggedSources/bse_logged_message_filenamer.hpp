/**
    \file LoggedSources/bse_logged_message_filenamer.hpp

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
#include "dvccode/CDef/bse_security_definition.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"

namespace HFSAT {

class BSELoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/BSELoggedData/";

    std::string t_filename_ = "INVALID_FILE";

    //std::cout << "Exch_sym, converted_excH_sym: " << _exchange_symbol_ << " , "
//	      << BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str() << std::endl;

    t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(
        logging_dir, BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str(),
        _preevent_YYYYMMDD_, rw_trading_location_, kTLocBSE);
/*
       std::cout << " DSNAme " << BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str()
                  << " fname " << t_filename_ << '\n';
*/
    if (rw_trading_location_ != kTLocMAX) {
//      std::cout << "rw_trading_location_" << std::endl;
      return t_filename_;
    } else {
      std::string t_str_ = BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_);
      std::size_t t_index_ = t_str_.find_last_of("_");
      return (HFSAT::LoggedMessageFileNamer::GetName(logging_dir, t_str_.substr(0, t_index_).c_str(),
                                                     _preevent_YYYYMMDD_, rw_trading_location_, kTLocBSE));
    }
  }
};
}
