/**
    \file LoggedSources/common_logged_message_filenamer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <string>
#include <sstream>

#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

namespace HFSAT {

/// Returns the name of the data file where CME/EUREX MDS has logged data
/// Typically it will be of the format
/// /NAS1/data/CMELoggedData/CHI/2014/12/01/ESH5_20141201
class CommonLoggedMessageFileNamer {
 public:
  /*
   * Returns the market data location for the given
   * exchange, exchange source, date and location.
   *
   * This function takes an extra argument to return the today's
   * logged data location ( t_use_todays_data_ ).
   *
   * Additionally, this function returns the filepath for
   * different (primary) location if it cannot find the file
   * in the desired location.
   */
  static std::string GetName(ExchSource_t t_exch_source_, const char* _exchange_symbol_,
                             const unsigned int _preevent_YYYYMMDD_, TradingLocation_t& rw_trading_location_,
                             bool t_use_todays_data_) {
    if (t_use_todays_data_) {
      std::string logging_dir_ = "/spare/local/MDSlogs/";
      std::string exch_str_ = "ICE";
      std::string filename_ = _exchange_symbol_;
      if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(_exchange_symbol_))) {
        filename_ = HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(_exchange_symbol_));
      }
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << _preevent_YYYYMMDD_;

      switch (t_exch_source_) {
        case kExchSourceCME: {
          exch_str_ = "CME";
        } break;

        case kExchSourceLIFFE: {
          exch_str_ = "LIFFE";
          std::replace(filename_.begin(), filename_.end(), ' ', '~');
        } break;

        case kExchSourceEUREX: {
          exch_str_ = "FPGA";
        } break;

        case kExchSourceICE: {
          exch_str_ = "ICE";
          std::replace(filename_.begin(), filename_.end(), ' ', '~');
        } break;

        case kExchSourceASX: {
          exch_str_ = "ASXPF";
        } break;

        case kExchSourceSGX: {
          exch_str_ = "SGXPF";
        } break;

        default:
          break;
      }
      return (logging_dir_ + exch_str_ + "/" + filename_ + "_" + t_temp_oss_.str());
    } else {
      return GetName(t_exch_source_, _exchange_symbol_, _preevent_YYYYMMDD_, rw_trading_location_, t_use_todays_data_);
    }
  }

  /*
   * Returns the Control message file location for the
   * given location and date
   */
  static std::string GetControlFileName(const unsigned int tradingdate_, TradingLocation_t& trading_location_) {
    std::string logging_dir_ = "/NAS1/data/CONTROLLoggedData/";
    std::string filename_ = "CONTROL";

    return HFSAT::LoggedMessageFileNamer::GetName(logging_dir_, filename_.c_str(), tradingdate_, trading_location_,
                                                  trading_location_);
  }

  /*
   * Returns the market data file location for the given
   * exchange, exchange_symbol, logging location and date
   */
  static std::string GetName(ExchSource_t t_exch_source_, const char* _exchange_symbol_,
                             const unsigned int _preevent_YYYYMMDD_, TradingLocation_t& rw_trading_location_) {
    std::string logging_dir_ = "/NAS1/data/ICELoggedData/";
    std::string filename_ = _exchange_symbol_;
    if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(_exchange_symbol_))) {
      filename_ = HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(_exchange_symbol_));
    }
    TradingLocation_t trading_location_ = TradingLocationUtils::GetTradingLocationExch(t_exch_source_);
    if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(_exchange_symbol_))) {
      filename_ = HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(_exchange_symbol_));
    }
    switch (t_exch_source_) {
      case kExchSourceASX: {
        logging_dir_ = "/NAS1/data/ASXPFLoggedData/";
        trading_location_ = kTLocSYD;
        break;
      }
      case kExchSourceBMF: {
        break;
      }
      case kExchSourceBMFEQ: {
        break;
      }
      case kExchSourceCFE: {
        logging_dir_ = "/NAS1/data/CSMLoggedData/";
        trading_location_ = kTLocNY4;
        break;
      }
      case kExchSourceCME: {
        logging_dir_ = "/NAS1/data/CMELoggedData/";
        break;
      }
      case kExchSourceEUREX: {
        if (((rw_trading_location_ == kTLocFR2) && (_preevent_YYYYMMDD_ >= 20131228)) ||
            ((rw_trading_location_ == kTLocBSL) && (_preevent_YYYYMMDD_ >= 20140108)) ||
            ((rw_trading_location_ == kTLocCHI) && (_preevent_YYYYMMDD_ >= 20140115)) ||
            ((rw_trading_location_ == kTLocHK) && (_preevent_YYYYMMDD_ >= 20140115)) ||
            ((rw_trading_location_ == kTLocJPY) && (_preevent_YYYYMMDD_ >= 20140115)) ||
            ((rw_trading_location_ == kTLocBMF) && (_preevent_YYYYMMDD_ >= 20140203)) ||
            ((rw_trading_location_ == kTLocM1) && (_preevent_YYYYMMDD_ >= 20140211))) {
          logging_dir_ = "/NAS1/data/EOBIPriceFeedLoggedData/";
        } else {
          logging_dir_ = "/NAS1/data/EUREXLoggedData/";
        }
        break;
      }
      case kExchSourceHKOMD: {
        break;
      }
      case kExchSourceHKOMDPF:
      case kExchSourceHKOMDCPF: {
        break;
      }
      case kExchSourceHONGKONG: {
        break;
      }
      case kExchSourceICE: {
        logging_dir_ = "/NAS1/data/ICELoggedData/";
        std::replace(filename_.begin(), filename_.end(), ' ', '~');
        break;
      }
      case kExchSourceJPY: {
        logging_dir_ = "/NAS1/data/OSEPFLoggedData/";
        break;
      }
      case kExchSourceTMX: {
        logging_dir_ = "/NAS1/data/TMX_OBF_PFLoggedData/";
        break;
      }
      case kExchSourceLIFFE: {
        logging_dir_ = "/NAS1/data/LIFFELoggedData/";
        std::replace(filename_.begin(), filename_.end(), ' ', '~');
        break;
      }
      case kExchSourceSGX: {
        logging_dir_ = "/NAS1/data/SGXLoggedData/";
        trading_location_ = kTLocSPR;
        if (filename_.substr(0, 4) == "SGX_") {
          filename_ = filename_.substr(4);  // Removing the initial "SGX_"
        }
        break;
      }
      case kExchSourceBSE: {
        //filename_ = BSESecurityDefinitions::GetUniqueInstance(_preevent_YYYYMMDD_).GetDataSourceSymbolFromExchSymbol(_exchange_symbol_);
        logging_dir_ = "/NAS1/data/BSELoggedData/";
        trading_location_ = kTLocBSE;
        break;
      }
      case kExchSourceCME_FPGA: {
        logging_dir_ = "/NAS1/data/CME_FPGALoggedData/";
        trading_location_ = kTLocCHI;
        break;
      }
      case kExchSourceBMF_FPGA: {
        logging_dir_ = "/NAS1/data/BMF_FPGALoggedData/";
        trading_location_ = kTLocBMF;
        break;
      }
      default: { break; }
    }

    return LoggedMessageFileNamer::GetName(logging_dir_, filename_.c_str(), _preevent_YYYYMMDD_, rw_trading_location_,
                                           trading_location_);
  }

  // Returns the orderfeed file for the given
  // exchange, symbol, location and date
  static std::string GetOrderFeedFilename(ExchSource_t t_exch_source_, const char* _exchange_symbol_,
                                          const unsigned int _preevent_YYYYMMDD_,
                                          TradingLocation_t& rw_trading_location_) {
    std::string logging_dir_ = "/NAS1/data/ICE_FODLoggedData/";
    std::string filename_ = _exchange_symbol_;

    TradingLocation_t trading_location_ = TradingLocationUtils::GetTradingLocationExch(t_exch_source_);

    switch (t_exch_source_) {
      case kExchSourceASX: {
        logging_dir_ = "/NAS1/data/ASXLoggedData/";
        break;
      }
      case kExchSourceEOBI:
      case kExchSourceEUREX: {
        logging_dir_ = "/NAS1/data/EOBINewLoggedData/";
        break;
      }
      case kExchSourceICE: {
        logging_dir_ = "/NAS1/data/ICE_FODLoggedData/";
        std::replace(filename_.begin(), filename_.end(), ' ', '~');
        break;
      }
      case kExchSourceSGX: {
        logging_dir_ = "/NAS1/data/SGXOFLoggedData/";
        if (filename_.substr(0, 4) == "SGX_") {
          filename_ = filename_.substr(4);  // Removing the initial "SGX_"
        }
        break;
      }
      case kExchSourceBSE: {
        //filename_ = BSESecurityDefinitions::GetUniqueInstance(_preevent_YYYYMMDD_).GetDataSourceSymbolFromExchSymbol(_exchange_symbol_);
        logging_dir_ = "/NAS1/data/BSEOFLoggedData/";
        break;
      }
      case kExchSourceCME: {
        logging_dir_ = "/NAS1/data/CMEOBFLoggedData/";
        break;
      }
      case kExchSourceRTS: {
        logging_dir_ = "/NAS1/data/RTS_OFv2LoggedData/";
        break;
      }
      case kExchSourceTMX: {
        logging_dir_ = "/NAS1/data/TMX_OBFLoggedData/";
        break;
      }
      case kExchSourceMICEX: {
        logging_dir_ = "/NAS1/data/MICEX_OFLoggedData/";
        break;
      }
      default: { break; }
    }

    return LoggedMessageFileNamer::GetName(logging_dir_, filename_.c_str(), _preevent_YYYYMMDD_, rw_trading_location_,
                                           trading_location_);
  }
};
}
