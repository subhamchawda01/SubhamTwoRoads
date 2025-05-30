/**
   \file dvccode/TradingInfo/integrated_server_config_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

// single instance class
// currently combining network_account_info and  network_account_interface

#ifndef BASE_INITCOMMON_INTEGRATED_SERVER_CONFIG_MANAGER_H
#define BASE_INITCOMMON_INTEGRATED_SERVER_CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/Utils/settings.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

#define DEF_INTEGRATED_SERVER_CONFIG_FILENAME "integrated_config.cfg"

namespace HFSAT {

class IntegratedServerConfigManager {
 public:
  // single interface files
  static IntegratedServerConfigManager& instance();
  static void RemoveInstance();
  // functions for network_account_info
  const DataInfo GetSrcDataInfo(ExchSource_t exch_source_, const std::string& src_shortcode_);

  const DataInfo GetSrcDataInfo(ExchSource_t exch_source_);

  const DataInfo GetDepDataInfo(ExchSource_t exch_source_, const std::string& dep_shortcode_);

  const DataInfo GetParamSendDataInfo();

  const DataInfo GetControlRecvDataInfo();

  const DataInfo GetCombControlDataInfo();

  const DataInfo GetMarginControlDataInfo(ExchSource_t exch_source_, std::string profile_);

  const DataInfo GetRetailDataInfoFromSourceType(const std::string& _retail_source_type_);

  // functions for network_account_interface
  const std::string GetInterface(TradingLocation_t loc, ExchSource_t exch, AppName app);
  const std::string GetInterface(ExchSource_t exch, AppName app);
  const std::string GetInterfaceForApp(AppName app);

  // functions for client_side_recovery_host_manager
  const std::string GetRecoveryHostIP();
  const int GetRecoveryHostPort();

  // functions for smart ors data logger
  const std::vector<ExchSource_t> GetORSExchangeList();
  const bool GetORSDataLoggerSetting(const std::string& logger_setting_name);

  const std::set<std::string> GetCombinedWriterExchangeList();
  const std::string GetCombinedWriterOperatingMode();

 private:
  IntegratedServerConfigManager();

  std::string AppNameString2(AppName app) {
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
      default:
        return "-X-";
    }
  }

  const std::string GetRelevantValue(const std::string& key);
  const int GetRelevantIntValue(const std::string& key);
  const DataInfo GetRelevantDataInfo(const std::string& key);
  void LogError(const std::string& key);

  static IntegratedServerConfigManager* inst;

  HFSAT::ORS::Settings* settings;

  // integrated file
  std::string integrated_server_config_filename_;

  const char* values_delimiter_ = ",";
  const std::string net_acc_info_key_prefix_ = "NET_ACC_INFO_";

  DataInfo def_data_info_;  ///< only used for returning dummy values
};
}

#endif  // BASE_INITCOMMON_INTEGRATED_SERVER_CONFIG_MANAGER_H
