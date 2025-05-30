#include "basetrade/Tools/get_exchanges.hpp"

using namespace std;
using namespace HFSAT;

vector<string> files;

vector<string> exchange_names = {
    "Invalid", "CME",          "EUREX",  "BMF",     "BMFEQ",    "NTP",    "TMX",      "MEFF",     "IDEM",   "HONGKONG",
    "REUTERS", "ICE",          "EBS",    "LIFFE",   "RTS",      "MICEX",  "MICEX_EQ", "MICEX_CR", "LSE",    "NASDAQ",
    "BATSCHI", "HYB",          "JPY",    "JPY_L1",  "TSE",      "QUINCY", "CMEMDP",   "EOBI",     "ESPEED", "PUMA",
    "CFE",     "CFNMicroware", "HKOMD",  "HKOMDPF", "HKOMDCPF", "ASX",    "SGX",      "AFLASH",   "RETAIL", "NSE",
    "NSE_FO",  "NSE_CD",       "NSE_EQ", "ICEFOD",  "ICEPL",    "ICECF",  "MAX"};

void getFiles(string _shortcode_, int date) {
  int tradingdate_ = date;
  cout << _shortcode_ << " " << date << endl;

  /** Get Exchange **/
  HFSAT::SecurityDefinitions::GetUniqueInstance(date).LoadNSESecurityDefinitions();
  ExchSource_t t_exch_source_ =
      HFSAT::SecurityDefinitions::GetUniqueInstance(date).GetContractSpecification(_shortcode_, date).exch_source_;

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date);
  TradingLocation_t dep_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
      HFSAT::SecurityDefinitions::GetContractExchSource(_shortcode_, date));
  if (HFSAT::UseEOBIData(dep_trading_location_, tradingdate_, _shortcode_)) {
    t_exch_source_ = HFSAT::kExchSourceEOBI;
  } else if (HFSAT::UseHKOMDData(dep_trading_location_, tradingdate_, t_exch_source_)) {
    t_exch_source_ = HFSAT::kExchSourceHKOMD;
  }

  cout << " Exchange: " << exchange_names[t_exch_source_] << endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Usage: Product[ALL] Date" << endl;
    return 0;
  }

  string _shortcode_ = string(argv[1]);
  int date = atoi(argv[2]);

  if (_shortcode_ == "ALL") {
    std::ifstream file("Read.txt");
    while (std::getline(file, _shortcode_)) {
      // cout << _shortcode_ << endl;
      getFiles(_shortcode_, date);
    }
  } else
    getFiles(_shortcode_, date);

  for (size_t i = 0; i < files.size(); i++) cout << files[i] << endl;
  return 0;
}
