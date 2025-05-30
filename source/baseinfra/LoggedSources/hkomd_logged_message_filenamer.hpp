
#ifndef HKOMD_LOGGED_MESSAGE_FILENAMER_H
#define HKOMD_LOGGED_MESSAGE_FILENAMER_H

#include <string>
#include <sstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

class HKOMDLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_todays_data_ = false) {
    static std::string logging_dir = "/NAS1/data/HKOMDLoggedData/";
    static std::string todays_logging_dir = "/spare/local/MDSlogs/";

    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;

      t_temp_oss_ << _preevent_YYYYMMDD_;

      return ((todays_logging_dir) + "HKOMD/" + std::string(_exchange_symbol_) + "_" + t_temp_oss_.str());
    }

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocHK);
  }
};

class HKOMDPFLoggedMessageFileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _preevent_YYYYMMDD_,
                             TradingLocation_t& rw_trading_location_, bool use_todays_data_ = false) {
    static std::string logging_dir_ = "NAS1/data/HKOMD_PFLoggedData/";
    static std::string todays_logging_dir_ = "/spare/local/MDSlogs/";

    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << _preevent_YYYYMMDD_;
      return ((todays_logging_dir_) + "HKOMDPF/" + std::string(_exchange_symbol_) + "_" + t_temp_oss_.str());
    }
    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_, _exchange_symbol_, _preevent_YYYYMMDD_,
                                                  rw_trading_location_, kTLocHK);
  }
};

class HKOMDCPFLoggedMessagefileNamer {
 public:
  static std::string GetName(const char* _exchange_symbol_, const unsigned int _prervent_YYYYMMDD_,
                             TradingLocation_t& rw_tradeing_location_, bool use_todays_data_ = false) {
    static std::string logging_dir_ = "/NAS1/data/HKOMDCPFLoggedData/";
    static std::string todays_logging_dir_ = "/spare/local/MDSlogs/";
    if (use_todays_data_) {
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << _prervent_YYYYMMDD_;
      return ((todays_logging_dir_) + "HKOMDCPF/" + std::string(_exchange_symbol_) + "_" + t_temp_oss_.str());
    }
    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_, _exchange_symbol_, _prervent_YYYYMMDD_,
                                                  rw_tradeing_location_, kTLocHK);
  }
};
}
#endif
