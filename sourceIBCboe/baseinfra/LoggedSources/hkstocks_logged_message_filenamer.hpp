
#ifndef HKSTOCKS_LOGGED_MESSAGE_FILENAMER_H
#define HKSTOCKS_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

class HKStocksLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_todays_data_ = false) {
    static std::string logging_dir = "/NAS1/data/HKStocksLoggedData/";
    static std::string todays_logging_dir = "/spare/local/MDSlogs/";

    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;

      t_temp_oss_ << _preevent_YYYYMMDD_;

      return ((todays_logging_dir) + "HKStocks/" +
              HKStocksSecurityDefinitions::GetShortCodeFromExchSymbol(_exchange_symbol_).substr(3) + "_" +
              t_temp_oss_.str());
    }

    return HFSAT::LoggedMessageFileNamer::GetName(
        logging_dir, HKStocksSecurityDefinitions::GetShortCodeFromExchSymbol(_exchange_symbol_).substr(3).c_str(),
        _preevent_YYYYMMDD_, rw_trading_location_, kTLocHK);
  }
};

class HKStocksPFLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_todays_data_ = false) {
    static std::string logging_dir_ = "/NAS1/data/HKStocksPFLoggedData/";
    static std::string todays_logging_dir_ = "/spare/local/MDSlogs/";

    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << _preevent_YYYYMMDD_;
      return ((todays_logging_dir_) + "HKStocksPF/" +
              HKStocksSecurityDefinitions::GetShortCodeFromExchSymbol(_exchange_symbol_).substr(3) + "_" +
              t_temp_oss_.str());
    }
    return HFSAT::LoggedMessageFileNamer::GetName(
        logging_dir_, HKStocksSecurityDefinitions::GetShortCodeFromExchSymbol(_exchange_symbol_).substr(3).c_str(),
        _preevent_YYYYMMDD_, rw_trading_location_, kTLocHK);
  }
};
}
#endif  // HKSTOCKS_LOGGED_MESSAGE_FILENAMER_H
