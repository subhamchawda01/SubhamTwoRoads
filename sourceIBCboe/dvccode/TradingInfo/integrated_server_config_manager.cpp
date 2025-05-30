/**
   \file dvccode/TradingInfo/integrated_server_config_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/TradingInfo/integrated_server_config_manager.hpp"

namespace HFSAT {

IntegratedServerConfigManager::IntegratedServerConfigManager() {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::ostringstream host_integrated_server_config_file;
  host_integrated_server_config_file << PROD_CONFIGS_DIR << hostname << "_" << DEF_INTEGRATED_SERVER_CONFIG_FILENAME;
  integrated_server_config_filename_ = host_integrated_server_config_file.str();

  settings = new HFSAT::ORS::Settings(integrated_server_config_filename_);
}

IntegratedServerConfigManager* IntegratedServerConfigManager::inst = NULL;

IntegratedServerConfigManager& IntegratedServerConfigManager::instance() {
  if (inst == NULL) {
    inst = new IntegratedServerConfigManager();
  }
  return *inst;
}

void IntegratedServerConfigManager::RemoveInstance() {
  if (NULL != inst) {
    delete inst;
    inst = NULL;
  }
}

const DataInfo IntegratedServerConfigManager::GetSrcDataInfo(ExchSource_t exch_source_,
                                                             const std::string& src_shortcode_) {
  if (exch_source_ == kExchSourceInvalid) {
    exch_source_ =
        SecurityDefinitions::GetContractExchSource(src_shortcode_, HFSAT::DateTime::GetCurrentIsoDateLocal());
  }

  std::string key = net_acc_info_key_prefix_ + "MKTDATA_" + ExchSourceStringForm(exch_source_);
  return GetRelevantDataInfo(key);
}

const DataInfo IntegratedServerConfigManager::GetSrcDataInfo(ExchSource_t exch_source_) {
  return GetSrcDataInfo(exch_source_, "");
}

const DataInfo IntegratedServerConfigManager::GetRetailDataInfoFromSourceType(const std::string& retail_source_type_) {
  std::string key = net_acc_info_key_prefix_ + "RETAILSOURCETYPE_" + retail_source_type_;
  return GetRelevantDataInfo(key);
}

const DataInfo IntegratedServerConfigManager::GetDepDataInfo(ExchSource_t exch_source_,
                                                             const std::string& dep_shortcode_) {
  if (exch_source_ == kExchSourceMICEX_EQ || exch_source_ == kExchSourceMICEX_CR) {
    exch_source_ = kExchSourceMICEX;
  }

  if (exch_source_ == kExchSourceInvalid) {
    exch_source_ =
        SecurityDefinitions::GetContractExchSource(dep_shortcode_, HFSAT::DateTime::GetCurrentIsoDateLocal());
  }

  std::string key = net_acc_info_key_prefix_ + "ORSDATA_" + ExchSourceStringForm(exch_source_);
  return GetRelevantDataInfo(key);
}

const DataInfo IntegratedServerConfigManager::GetParamSendDataInfo() {
  std::string key = net_acc_info_key_prefix_ + "PARAMSEND";
  return GetRelevantDataInfo(key);
}

const DataInfo IntegratedServerConfigManager::GetControlRecvDataInfo() {
  std::string key = net_acc_info_key_prefix_ + "TRADECONTROLRECV";
  return GetRelevantDataInfo(key);
}

const DataInfo IntegratedServerConfigManager::GetCombControlDataInfo() {
  std::string key = net_acc_info_key_prefix_ + "COMBINEDCONTROL";
  return GetRelevantDataInfo(key);
}

const DataInfo IntegratedServerConfigManager::GetMarginControlDataInfo(ExchSource_t exch_source_,
                                                                       std::string profile_) {
  std::string key = net_acc_info_key_prefix_ + "MARGINCONTROL_" + ExchSourceStringForm(exch_source_) + "_" + profile_;
  return GetRelevantDataInfo(key);
}

const std::string IntegratedServerConfigManager::GetInterface(TradingLocation_t loc, ExchSource_t exch, AppName app) {
  std::string loc_str = TradingLocationUtils::GetTradingLocationName(loc);
  std::string exch_str = ExchSourceStringForm(exch);
  std::string app_str = AppNameString2(app);

  std::string key = "NET_IFACE_INFO_" + loc_str + "_" + exch_str + "_" + app_str;

  return GetRelevantValue(key);
}

// at current location
const std::string IntegratedServerConfigManager::GetInterface(ExchSource_t exch, AppName app) {
  return GetInterface(TradingLocationUtils::GetTradingLocationFromHostname(), exch, app);
}

const std::string IntegratedServerConfigManager::GetInterfaceForApp(AppName app) {
  std::string loc_str =
      TradingLocationUtils::GetTradingLocationName(TradingLocationUtils::GetTradingLocationFromHostname());
  std::string app_str = AppNameString2(app);

  std::string key = "NET_IFACE_INFO_" + loc_str + "_" + app_str;

  return GetRelevantValue(key);
}

const std::string IntegratedServerConfigManager::GetRecoveryHostIP() {
  std::string key = "RECOVERY_IP";
  return GetRelevantValue(key);
}

const int IntegratedServerConfigManager::GetRecoveryHostPort() {
  std::string key = "RECOVERY_PORT";
  return GetRelevantIntValue(key);
}

const std::vector<ExchSource_t> IntegratedServerConfigManager::GetORSExchangeList() {
  std::string key = "SMART_ORS_EXCHANGE_LIST";
  std::string tempvalue = GetRelevantValue(key);
  const char* value = tempvalue.c_str();
  std::vector<char*> tokens;
  HFSAT::PerishableStringTokenizer::ConstStringTokenizer(value, values_delimiter_, tokens);

  std::vector<ExchSource_t> exch_src_list;

  for (auto i = 0u; i < tokens.size(); i++) {
    exch_src_list.push_back(HFSAT::StringToExchSource(tokens[i]));
  }

  return exch_src_list;
}

const std::set<std::string> IntegratedServerConfigManager::GetCombinedWriterExchangeList() {
  std::string key = "COMBINEDWRITER_EXCHANGE_LIST";
  std::string tempvalue = GetRelevantValue(key);
  const char* value = tempvalue.c_str();
  std::vector<char*> tokens;
  HFSAT::PerishableStringTokenizer::ConstStringTokenizer(value, values_delimiter_, tokens);

  std::set<std::string> exch_set;

  for (auto i = 0u; i < tokens.size(); i++) {
    exch_set.insert(tokens[i]);
  }

  return exch_set;
}

const std::string IntegratedServerConfigManager::GetCombinedWriterOperatingMode() {
  std::string key = "COMBINEDWRITER_MODE_LIST";
  std::string tempvalue = GetRelevantValue(key);
  const char* value = tempvalue.c_str();
  std::vector<char*> tokens;
  HFSAT::PerishableStringTokenizer::ConstStringTokenizer(value, values_delimiter_, tokens);

  for (auto i = 0u; i < tokens.size(); i++) {
    if (strcmp(tokens[i], "Logger") == 0) {
      return "HYBRID";
    }
  }

  return "WRITER";
}

const bool IntegratedServerConfigManager::GetORSDataLoggerSetting(const std::string& logger_setting_name) {
  std::string key = "SMART_ORS_DATA_LOG_SETTINGS_" + logger_setting_name;
  return GetRelevantIntValue(key);
}

const std::string IntegratedServerConfigManager::GetRelevantValue(const std::string& key) {
  if (!settings->has(key)) {
    LogError(key);
  }

  return settings->getValue(key);
}

const int IntegratedServerConfigManager::GetRelevantIntValue(const std::string& key) {
  if (!settings->has(key)) {
    LogError(key);
  }

  return settings->getIntValue(key, 0);
}

const DataInfo IntegratedServerConfigManager::GetRelevantDataInfo(const std::string& key) {
  std::string tempvalue = GetRelevantValue(key);
  const char* value = tempvalue.c_str();
  std::vector<char*> tokens;
  HFSAT::PerishableStringTokenizer::ConstStringTokenizer(value, values_delimiter_, tokens);
  if (tokens.size() == 2) {
    return DataInfo(tokens[0], atoi(tokens[1]));
  }

  LogError(key);
  return def_data_info_;
}

void IntegratedServerConfigManager::LogError(const std::string& error) {
  std::cerr << "Key/Appropriate value not found : " << error << " Should not be here ! " << std::endl;
  ExitVerbose(kExitErrorCodeIntegratedServerConfigManager);
}
}
