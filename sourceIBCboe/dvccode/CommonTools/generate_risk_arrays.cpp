#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "dvccode/SpanMargin/span_calculator.hpp"
#include "dvccode/CDef/security_definitions.hpp"

int main(int argc, char** argv) {
  // cli arguments to the exec.
  int tradingdate_ = atoi(argv[1]);
  std::string exch = argv[2];
  std::string output_path = "";

  if (argc == 4) {
    output_path = argv[3];
  } else {
    output_path = "/spare/local/tradeinfo/" + exch + "_Files/Margin_Files/Risk_Arrays";
  }

  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).SetExchangeType(exch);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadSecurityDefinitions();

  // Just pass an empty sec_name_indexer
  // Not gonna use it anyways.
  HFSAT::SpanCalculator margin(tradingdate_, sec_name_indexer_, exch);

  // Open file stream
  std::ofstream outputFile(output_path + "/risk_arrays_" + std::to_string(tradingdate_));
  if (!outputFile.is_open()) {
      std::cerr << "Error opening file for writing!" << std::endl;
      return 1;
  }

  auto risk_array_map = margin.GetRiskMap();

  // Iterate through the map and write to the file
  for (const auto& pair : risk_array_map) {
      std::string exch_sym = HFSAT::SecurityDefinitions::ConvertDataSourceNametoExchSymbol(pair.first);
      if ("INVALID" != exch_sym) {
          outputFile << exch_sym << " ";
          for (double val : pair.second) {
              outputFile << val << " ";
          }
          outputFile << std::endl;
      }
  }
  // Close the file stream
  outputFile.close();

  return 0;
}
