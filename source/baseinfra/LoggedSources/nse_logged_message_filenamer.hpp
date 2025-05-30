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
#define WORKER_CONFIG_FILE "/home/pengine/prod/live_configs/Worker_Config_file.txt"
#define USE_UNCONVERTED_DATA_DATE_ 20240601

namespace HFSAT {

class NSELoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_) {
    static std::string logging_dir = "/NAS1/data/NSELoggedData/";

    /*
    if(_preevent_YYYYMMDD_>=USE_UNCONVERTED_DATA_DATE_)
        logging_dir = "/NAS10/data/NSELoggedData/";
    */

    std::ifstream worker_config_file_;
    worker_config_file_.open(WORKER_CONFIG_FILE);
    if (!worker_config_file_.is_open()) {
      //     std::cout << "Assuming Machine is Local  as File Exist" << WORKER_CONFIG_FILE << " \n";

      if (_preevent_YYYYMMDD_ < 20180201) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2017_2018_01/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20190101) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2018_02_12/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20200101) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2018_and_2019/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20200511) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2020_01_05/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20201201) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2020_05_11/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20210501) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2020_12_2021_01_04/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20210801) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2021_05_07/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20211101) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2021_08_10/data/NSELoggedData/";
      } else if (_preevent_YYYYMMDD_ < 20220101) {
        logging_dir = "/run/media/dvcinfra/NSE_MTBT_2021_11_12/data/NSELoggedData/";
      } else if ( _preevent_YYYYMMDD_ < 20220330 ) {
         logging_dir = "/run/media/dvcinfra/NSE_MTBT_2022_01_03/data/NSELoggedData/";
      }
    }

    std::string t_filename_ = "INVALID_FILE";
    std::string ex = _exchange_symbol_;
    std::string shc_ = NSESecurityDefinitions::GetShortCodeFromExchangeSymbol(ex);
    // std::cout << "NSELoggedMessageFileNamer: exch, shc :: " << _exchange_symbol_ << " , " << shc_ << std::endl;

    char seg_type_ = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shc_);
    // std::cout << "Type: " << seg_type_ << std::endl;

    if (seg_type_ == NSE_IX_SEGMENT_MARKING) {
      t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(logging_dir, shc_.c_str(), _preevent_YYYYMMDD_,
                                                           rw_trading_location_, kTLocNSE);
    } else {
      t_filename_ = HFSAT::LoggedMessageFileNamer::GetName(
          logging_dir, NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(_exchange_symbol_).c_str(),
          _preevent_YYYYMMDD_, rw_trading_location_, kTLocNSE);
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
