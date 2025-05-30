#include <cstring>

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/common_files_path.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define LINE_BUFFER_SIZE 1024
#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

namespace HFSAT {
GlobalSimDataManager& GlobalSimDataManager::GetUniqueInstance(HFSAT::DebugLogger& dbglogger) {
  static GlobalSimDataManager unique_instance(dbglogger);
  return unique_instance;
}

GlobalSimDataManager::GlobalSimDataManager(HFSAT::DebugLogger& dbglogger_) {
  trading_date_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  std::string global_sim_data_config(HFSAT::FILEPATH::kGlobalSimDataFile);
  std::ifstream global_sim_data_config_stream;
  const char* date_setting_name = "Date";

  if (HFSAT::IsItSimulationServer()) {
    global_sim_data_config_stream.open(global_sim_data_config.c_str());

    if (!global_sim_data_config_stream.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "Unable to Open Global Sim Data Config File : " << global_sim_data_config
                                   << DBGLOG_ENDL_NOFLUSH;
      exit(-1);
    }

    while (global_sim_data_config_stream.good()) {
      char line_buffer[LINE_BUFFER_SIZE];
      global_sim_data_config_stream.getline(line_buffer, LINE_BUFFER_SIZE);
      if (std::string::npos != std::string(line_buffer).find("#")) continue;
      if (!global_sim_data_config_stream.good()) break;

      HFSAT::PerishableStringTokenizer pst(line_buffer, LINE_BUFFER_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      if (!strcmp(tokens[0], date_setting_name)) {
        int date = atoi(tokens[1]);
        if ((date < MIN_YYYYMMDD) || (date > MAX_YYYYMMDD)) {
          DBGLOG_CLASS_FUNC_LINE_FATAL << "Tradingdate_ " << date << " out of range [ " << MIN_YYYYMMDD << " "
                                       << MAX_YYYYMMDD << " ] " << DBGLOG_ENDL_NOFLUSH;
          exit(1);
        }
        trading_date_ = date;
      }
    }
    global_sim_data_config_stream.close();
  }
}

int GlobalSimDataManager::GetTradingDate() { return trading_date_; }
}
