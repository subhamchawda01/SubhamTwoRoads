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
#include "dvccode/CDef/cboe_security_definition.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"
#include "dvccode/Utils/cboe_daily_token_symbol_handler.hpp"
#define WORKER_CONFIG_FILE "/home/pengine/prod/live_configs/Worker_Config_file.txt"
#define USE_UNCONVERTED_DATA_DATE_ 20240601

namespace HFSAT {

class IBKRLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    // std::cout<< _exchange_symbol_<<"\n";
    static std::string logging_dir = "/NAS1/data/CBOELoggedData/";

    /*
    if(_preevent_YYYYMMDD_>=USE_UNCONVERTED_DATA_DATE_)
        logging_dir = "/NAS10/data/NSELoggedData/";
    */

    std::ifstream worker_config_file_;
    worker_config_file_.open(WORKER_CONFIG_FILE);
    if (!worker_config_file_.is_open()) {
        std::cerr << "Cannot open the file" << WORKER_CONFIG_FILE << " \n";
    }

    std::string t_filename_ = "INVALID_FILE";
    std::string ex = _exchange_symbol_;
    std::string shc_ = CBOESecurityDefinitions::GetShortCodeFromExchangeSymbol(ex);
    // std::cout << "NSELoggedMessageFileNamer: exch, shc :: " << _exchange_symbol_ << " , " << shc_ << std::endl;

    char seg_type_ = CBOESecurityDefinitions::GetSegmentTypeFromShortCode(shc_);
    // std::cout << "Type: " << seg_type_ << std::endl;
    // std::cout << logging_dir << " "<< shc_ <<" "<< _preevent_YYYYMMDD_ <<" "<< rw_trading_location_ << " "<< seg_type_<< "\n";

    if (seg_type_ == CBOE_FO_SEGMENT_MARKING) {
      t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(logging_dir, CBOESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str(), _preevent_YYYYMMDD_,
                                                           rw_trading_location_, kTLocCBOE);
      // std::cout<<t_filename_<<"\n";
    } else {
      t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(
          logging_dir, CBOESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str(),
          _preevent_YYYYMMDD_, rw_trading_location_, kTLocCBOE);
    }
    //   std::cout << " DSNAme " << NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str()
    //              << " fname " << t_filename_ << '\n';
    // std::cout << "Filename: " << t_filename_ << std::endl;
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
