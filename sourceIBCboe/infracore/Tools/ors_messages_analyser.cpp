#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "infracore/Tools/positions_and_orders_calculator_all_saci.hpp"
#include "infracore/Tools/ors_message_reader.hpp"

void LoadSecurityDefinitions(const std::string short_code, const int yyyymmdd) {
  HFSAT::SecurityDefinitions& sec_def = HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd);
  if (short_code.substr(0, 4) == "NSE_") {
    if (sec_def.IsNSESecDefAvailable(yyyymmdd)) {
      sec_def.LoadNSESecurityDefinitions();
    }
  } else if (short_code.substr(0, 3) == "HK_") {
    if (sec_def.IsHKSecDefAvailable(yyyymmdd)) {
      sec_def.LoadHKStocksSecurityDefinitions();
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: <exec> <input-file> <shortcode> <yyyymmdd>" << std::endl;
    exit(-1);
  }

  std::string input_file(argv[1]);
  std::string shortcode(argv[2]);
  int yyyymmdd = std::atoi(argv[3]);

  LoadSecurityDefinitions(shortcode, yyyymmdd);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);
  const char* this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode.c_str());

  HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().AddString(this_exch_symbol_);
  int t_security_id_ = HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().GetIdFromChar16(this_exch_symbol_);
  // std::cout << t_security_id_ << " "
  //       << HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance().GetIdFromSecname(this_exch_symbol_) << std::endl;
  if (t_security_id_ < 0) {
    std::cerr << "Cannot Add Product " << shortcode << " to SSSI. Exiting." << std::endl;
    exit(1);
  }

  HFSAT::PositionAndOrderCalculatorAllSACI calculator(yyyymmdd, shortcode);
  HFSAT::ORSMessageReader reader(&calculator, input_file, t_security_id_);

  reader.PlayAll();

  return 0;
}
