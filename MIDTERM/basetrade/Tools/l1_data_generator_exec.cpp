#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "basetrade/Tools/l1_data_generator.hpp"
#include "basetrade/Tools/l1_data_logger.hpp"

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << "USAGE : <exec> <tradingdate_yyyymmdd> <shortcode> <L1_Data_Log_Dir>\n";
    exit(1);
  }

  try {
    int trading_date = std::atoi(argv[1]);
    std::string shortcode(argv[2]);
    std::string log_dirpath(argv[3]);

    std::string symbol;
    if (shortcode.substr(0, 3) == "NSE") {
      HFSAT::NSESecurityDefinitions::GetUniqueInstance(trading_date);
      HFSAT::SecurityDefinitions::LoadNSESecurityDefinitions();
      std::string exchange_symbol = HFSAT::NSESecurityDefinitions::GetExchSymbolNSE(shortcode);
      symbol = HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
      if (symbol == "INVALID") {
        std::cerr << "Invalid Datasource Symbol.Exiting.\n";
        exit(1);
      }
    } else {
      symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
    }

    std::string log_filepath = log_dirpath + "/" + symbol + "_" + std::to_string(trading_date);

    std::vector<std::string> shortcodes;
    shortcodes.push_back(shortcode);

    HFSAT::L1DataGenerator l1_data_generator(trading_date, shortcodes);

    HFSAT::L1DataLogger l1_data_logger(log_filepath, l1_data_generator.GetWatch());
    l1_data_generator.AddListener(&l1_data_logger);

    l1_data_generator.Run();

  } catch (...) {
    std::cerr << "Invalid Arguments\n";
  }
  return 0;
}
