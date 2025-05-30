// =====================================================================================
//
//       Filename:  network_account_interface_manager.hpp
//
//    Description:
//
//
//        Version:  1.0
//        Created:  06/27/2014 01:02:10 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include <string>
#include <map>
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

#define DEF_NW_INTERFACE_CONFIG_FILENAME "network_interface_config.cfg"

#define UNDEFINED_INTERFACE_STRING "undef"

namespace HFSAT {

enum AppName {
  k_MktDataRaw,
  k_MktDataMcast,
  k_MktDataLive,
  k_ORS,
  k_ORS_LS,
  k_ORSSlow,
  k_Control,
  k_MktDataRawSideA,
  k_MktDataRawSideB,
  k_MktDataRawSideR,  // -- Recovery
  k_RetailMulticast,
  k_MktDataRawSideC,
  k_MktDataRawExanic
};

class NetworkAccountInterfaceManager {
  std::map<std::string, std::string> map_;
  static NetworkAccountInterfaceManager* inst;

  std::string AppNameString(AppName app) {
    switch (app) {
      case k_MktDataRaw:
        return "MD";
      case k_MktDataMcast:
        return "MM";
      case k_MktDataLive:
        return "LS";
      case k_ORS:
        return "ORS";
      case k_ORS_LS:
        return "ORS_LS";
      case k_ORSSlow:
        return "ORS_SLOW";
      case k_Control:
        return "CONTROL";
      case k_MktDataRawSideA:
        return "MD_A";
      case k_MktDataRawSideB:
        return "MD_B";
      case k_MktDataRawSideR:
        return "MD_R";
      case k_RetailMulticast:
        return "RETAIL_LS";
      case k_MktDataRawSideC:
        return "MD_C";
      case k_MktDataRawExanic:
        return "MD_EXANIC";
      default:
        return "-X-";
    }
  }

  NetworkAccountInterfaceManager() : map_() {
    std::ifstream network_interfaces_config_file_;

    //=============================   Trying to make things robust against incorrect filesync etc
    //============================= //

    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    std::ostringstream host_nw_interface_config_file_;
    host_nw_interface_config_file_ << PROD_CONFIGS_DIR << hostname << "_" << DEF_NW_INTERFACE_CONFIG_FILENAME;

    std::string DEF_NW_INTERFACE_CONFIG = host_nw_interface_config_file_.str();

    //std::cout << "Loaded Network Interface File : " << DEF_NW_INTERFACE_CONFIG << "\n";

    //=========================================================================================================================
    ////

    network_interfaces_config_file_.open(DEF_NW_INTERFACE_CONFIG.c_str());
    if (!network_interfaces_config_file_.is_open()) {
      std::cerr << " Could not open n/w config file, File : " << DEF_NW_INTERFACE_CONFIG << "\n";
      exit(-1);
    }

    while (network_interfaces_config_file_.good()) {
      char buffer_[1024];
      network_interfaces_config_file_.getline(buffer_, 1024);
      PerishableStringTokenizer st_(buffer_, 1024);
      std::vector<const char*> tokens = st_.GetTokens();
      if (tokens.size() < 3) continue;
      std::string key = "";
      for (unsigned int j = 0; j < tokens.size() - 1; ++j) key += tokens[j];
      map_[key] = std::string(tokens[tokens.size() - 1]);
    }
    network_interfaces_config_file_.close();
  }

 public:
  static NetworkAccountInterfaceManager& instance();

  static void RemoveInstance();

  std::string GetInterface(TradingLocation_t loc, ExchSource_t exch, AppName app) {
    std::string loc_str = TradingLocationUtils::GetTradingLocationName(loc);
    std::string exch_str = ExchSourceStringForm(exch);
    std::string app_str = AppNameString(app);

    std::string key = loc_str + exch_str + app_str;

    std::cout << "GetInterface: " << loc_str << " : " << exch_str << " : " << app_str << std::endl;
    if (map_.find(key) == map_.end()) {
      std::cerr << "could not find appropriate interface for (" << key << ") . Exiting..."
                << "\n";
      exit(-1);
    }

    std::cerr << " Location : " << TradingLocationUtils::GetTradingLocationName(loc) << " "
              << " Exchange : " << ExchSourceStringForm(exch) << " "
              << " AppName :  " << AppNameString(app) << " "
              << " Interface : " << map_[key] << "\n";
    return map_[key];
  }

  // at current location

  std::string GetInterface(ExchSource_t exch, AppName app) {
    return GetInterface(TradingLocationUtils::GetTradingLocationFromHostname(), exch, app);
  }

  // exchange string. treat BMF, BMF_EQ, NTP as same
  std::string GetInterface(const std::string& exch, AppName app) {
    std::string loc_str =
        TradingLocationUtils::GetTradingLocationName(TradingLocationUtils::GetTradingLocationFromHostname());
    std::string exch_str = (exch == "NTP" ? "BMF" : (exch == "BMF_EQ" ? "BMF" : exch));
    std::string app_str = AppNameString(app);

    std::string key = loc_str + exch_str + app_str;

    if (map_.find(key) == map_.end()) {
      std::cerr << "could not find appropriate interface for (" << key << ") . Exiting..."
                << "\n";
      exit(-1);
    }
    return map_[key];
  }

  std::string GetInterfaceForApp(AppName app) {
    std::string loc_str =
        TradingLocationUtils::GetTradingLocationName(TradingLocationUtils::GetTradingLocationFromHostname());
    std::string app_str = AppNameString(app);
    std::string key = loc_str + app_str;
    if (map_.find(key) == map_.end()) {
      std::cerr << "could not find appropriate interface for (" << key << ") . Exiting..."
                << "\n";
      exit(-1);
    }
    return map_[key];
  }
};
}
