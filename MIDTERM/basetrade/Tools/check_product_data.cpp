/**
    \file Tools/get_volume_on_day.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

// exec to check if the indicator has data present or not
#include <sstream>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "baseinfra/LoggedSources/logged_message_filenamer.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
bool checkForData(std::string shc, int date) {
  HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shc, date);
  const char* exch_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shc);
  HFSAT::TradingLocation_t rw_trading_location_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(_this_exch_source_);  // initialize to primary location
  std::stringstream ss;
  ss << "/NAS1/data/";
  // std::cerr<< "ThisExchangeSource: " << HFSAT::ExchSourceStringForm(_this_exch_source_) << "\t\t"
  //<< "Location: " << rw_trading_location_ << std::endl;

  if (_this_exch_source_ == HFSAT::kExchSourceBMF) {
    HFSAT::NTPLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_, false, false);
  } else if (_this_exch_source_ == HFSAT::kExchSourceBMFEQ) {
    HFSAT::NTPLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_, false, true);
  } else if (_this_exch_source_ == HFSAT::kExchSourceCME) {
    HFSAT::CommonLoggedMessageFileNamer::GetName(_this_exch_source_, exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceEUREX) {
    HFSAT::EUREXLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceTMX) {
    HFSAT::TMXLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceLIFFE) {
    HFSAT::LIFFELoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceRTS) {
    HFSAT::RTSLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceMICEX || _this_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
             _this_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
    HFSAT::MICEXLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceHONGKONG) {
    HFSAT::HKEXLoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceJPY || _this_exch_source_ == HFSAT::kExchSourceJPY_L1) {
    HFSAT::OSELoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceCFE) {
    HFSAT::CFELoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  } else if (_this_exch_source_ == HFSAT::kExchSourceNSE) {
    HFSAT::NSELoggedMessageFileNamer::GetName(exch_symbol, date, rw_trading_location_);
  }
  ss << "LoggedData/";

  // std::cerr<< "ThisExchangeSource: " << HFSAT::ExchSourceStringForm(_this_exch_source_) << "\t\t"
  //<< "Location: " << rw_trading_location_ << std::endl;
  if (rw_trading_location_ == HFSAT::kTLocMAX) return false;
  return true;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " shortcode YYYYMMDD\n";
    exit(0);
  }
  std::string shc = argv[1];
  int date = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date);

  if (strncmp(shc.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(date).LoadNSESecurityDefinitions();
  }

  if (HFSAT::SecurityDefinitions::GetUniqueInstance(date).IsValidContract(shc.c_str())) {
    if (checkForData(shc, date))
      std::cout << "1\n";
    else
      std::cout << "0\n";
    return 0;

  } else {
    return 0;
  }
}
