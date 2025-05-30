#include "basetrade/Tools/get_path.hpp"

using namespace std;
using namespace HFSAT;

vector<string> files;

void getFiles(string _shortcode_, int date, bool is_pf, bool is_of, bool is_fpga) {
  int tradingdate_ = date;

  if (_shortcode_.substr(0, 4) == "NSE_") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(date).LoadNSESecurityDefinitions();
  }

  /** Get Exchange **/
  ExchSource_t t_exch_source_ =
      HFSAT::SecurityDefinitions::GetUniqueInstance(date).GetContractSpecification(_shortcode_, date).exch_source_;

  string file_path = "";
  const unsigned int t_preevent_YYYYMMDD_ = date;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date);
  const char* t_exchange_symbol_;
  HFSAT::HybridSecurityManager* hybrid_security_manager_ = new HFSAT::HybridSecurityManager(tradingdate_);

  if (hybrid_security_manager_->IsHybridSecurity(_shortcode_)) {
    t_exchange_symbol_ =
        HFSAT::ExchangeSymbolManager::GetExchSymbol(hybrid_security_manager_->GetActualSecurityFromHybrid(_shortcode_));
  } else {
    t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_shortcode_);
  }

  TradingLocation_t dep_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
      HFSAT::SecurityDefinitions::GetContractExchSource(_shortcode_, date));
  TradingLocation_t trading_location_file_read_ = dep_trading_location_;

  if (HFSAT::UseEOBIData(dep_trading_location_, tradingdate_, _shortcode_)) {
    t_exch_source_ = HFSAT::kExchSourceEOBI;
  } else if (HFSAT::UseHKOMDData(dep_trading_location_, tradingdate_, t_exch_source_)) {
    t_exch_source_ = HFSAT::kExchSourceHKOMD;
  }
  string tmp;

  bool use_todays_data_ = false;

  switch (t_exch_source_) {
    case kExchSourceCME:
      if (is_pf) {
        /// Default is price-feed
        file_path = CommonLoggedMessageFileNamer::GetName(kExchSourceCME, t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                          trading_location_file_read_);
      } else if (is_of) {
        /// Order Feed file path
        file_path = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
            kExchSourceCME, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      } else if (is_fpga) {
        /// FPGA file path
        file_path = CommonLoggedMessageFileNamer::GetName(kExchSourceCME_FPGA, t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                          trading_location_file_read_);
      }
      files.push_back(file_path);
      break;
    case kExchSourceEOBI:
    case kExchSourceEUREX:
      // FESX_0 20160307
      file_path =
          EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);

      trading_location_file_read_ = HFSAT::kTLocFR2;
      file_path = EOBILoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                      trading_location_file_read_, false);
      if (is_pf) {
        file_path = EOBIPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                 trading_location_file_read_);
      } else if (is_of) {
        file_path =
            EOBILoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      } else if (is_fpga) {
        break;
      }
      files.push_back(file_path);
      break;
    case kExchSourceNTP:
    case kExchSourceBMF:
      /** Left out a case of BMF for prev dates, see why last 2 params needed, if needed add to list*/
      if (is_of || is_pf) {
        file_path = NTPLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                       trading_location_file_read_, is_of, false);
      } else if (is_fpga) {
        break;
      }
      files.push_back(file_path);
      break;
    case kExchSourceBMFEQ:
      /** Left out a case of BMF for prev dates, see why last 2 params needed, if needed add to list*/
      file_path =
          PUMALoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
      break;
    case kExchSourceTMX:
      if (is_pf) {
        file_path =
            TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      } else if (is_of) {
        file_path = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
            kExchSourceTMX, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      }
      files.push_back(file_path);
      break;
    case kExchSourceMEFF:
      break;
    case kExchSourceIDEM:
      break;
    case kExchSourceHONGKONG:
      file_path =
          HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceREUTERS:
      break;
    case kExchSourceICE:
      file_path = CommonLoggedMessageFileNamer::GetOrderFeedFilename(kExchSourceICE, t_exchange_symbol_,
                                                                     t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceLIFFE:
      file_path =
          LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceRTS:
      if (is_pf) {
        file_path =
            RTSLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      } else if (is_of) {
      } else if (is_fpga) {
      }
      files.push_back(file_path);
      break;
    case kExchSourceMICEX:
    case kExchSourceMICEX_EQ:
    case kExchSourceMICEX_CR:
      file_path =
          MICEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceLSE:
      break;
    case kExchSourceNASDAQ:
      break;
    case kExchSourceBATSCHI:
      file_path =
          CHIXL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceHYB:
      break;
    case kExchSourceJPY:
      if (dep_trading_location_ == HFSAT::kTLocJPY || dep_trading_location_ == HFSAT::kTLocCHI ||
          dep_trading_location_ == HFSAT::kTLocFR2 || dep_trading_location_ == HFSAT::kTLocSYD ||
          dep_trading_location_ == HFSAT::kTLocNSE || dep_trading_location_ == HFSAT::kTLocSPR) {
        file_path = OSEPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                                trading_location_file_read_);
        files.push_back(file_path);
      } else {
        file_path =
            OSEL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
        files.push_back(file_path);
      }
      break;
    case kExchSourceJPY_L1:
      break;
    case kExchSourceTSE:
      break;
    case kExchSourceQUINCY:
      break;
    case kExchSourceCMEMDP:
      break;
    case kExchSourceESPEED:
      break;
    case kExchSourcePUMA:
      break;
    case kExchSourceCFE:
      file_path =
          CFELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceCFNMicroware:
      break;
    case kExchSourceHKOMD:
      /** Not handled HK equity **/
      file_path = HKOMDLoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                       trading_location_file_read_, false);
      files.push_back(file_path);
      break;
    case kExchSourceHKOMDPF:
    case kExchSourceHKOMDCPF:
      file_path = HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                          trading_location_file_read_, use_todays_data_);
      files.push_back(file_path);
      break;
    case kExchSourceASX:
      // AP_0 20160303
      trading_location_file_read_ = kTLocSYD;
      if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(t_exchange_symbol_))) {
        file_path = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
            kExchSourceASX,
            (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(t_exchange_symbol_))).c_str(),
            t_preevent_YYYYMMDD_, trading_location_file_read_);
      } else {
        file_path = CommonLoggedMessageFileNamer::GetOrderFeedFilename(
            kExchSourceASX, t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      }
      files.push_back(file_path);
      break;
    case kExchSourceSGX:
      /** See the issue **/
      if (HFSAT::HybridSecurityManager::IsHybridExch(std::string(t_exchange_symbol_))) {
        file_path = CommonLoggedMessageFileNamer::GetName(
            kExchSourceSGX,
            (HFSAT::HybridSecurityManager::GetActualExchFromHybrid(std::string(t_exchange_symbol_))).c_str(),
            t_preevent_YYYYMMDD_, trading_location_file_read_);
        files.push_back(file_path);
      } else {
        file_path = CommonLoggedMessageFileNamer::GetName(kExchSourceSGX, t_exchange_symbol_, t_preevent_YYYYMMDD_,
                                                          trading_location_file_read_);
        files.push_back(file_path);
      }

      break;
    case kExchSourceAFLASH:
      break;
    case kExchSourceRETAIL:
      break;
    case kExchSourceNSE:
      file_path =
          NSELoggedMessageFileNamer::GetName(t_exchange_symbol_, t_preevent_YYYYMMDD_, trading_location_file_read_);
      files.push_back(file_path);
      break;
    case kExchSourceNSE_FO:
      break;
    case kExchSourceNSE_CD:
      break;
    case kExchSourceNSE_EQ:
      break;
    case kExchSourceICEFOD:
      break;
    case kExchSourceICEPL:
      break;
    case kExchSourceICECF:
      break;
    case kExchSourceMAX:
      break;
    case kExchSourceBSE:
      break;
    case kExchSourceInvalid:
      break;
    default:
      break;
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Usage: Product[ALL] Date [OF/PF/FPGA]" << endl;
    return 0;
  }

  string _shortcode_ = string(argv[1]);
  int date = atoi(argv[2]);

  bool is_pf = false;
  bool is_of = true;
  bool is_fpga = false;
  // Read the pf/of/fpga flags if available
  if (argc > 3) {
    if (strcmp(argv[3], "OF") == 0) {
      is_of = true;
    } else if (strcmp(argv[3], "PF") == 0) {
      is_pf = true;
      is_of = false;
    } else if (strcmp(argv[3], "FPGA") == 0) {
      is_fpga = true;
      is_of = false;
    }
  }

  if (_shortcode_ == "ALL") {
    std::ifstream file("Read.txt");
    while (std::getline(file, _shortcode_)) {
      getFiles(_shortcode_, date, is_pf, is_of, is_fpga);
    }
  } else {
    getFiles(_shortcode_, date, is_pf, is_of, is_fpga);
  }

  for (size_t i = 0; i < files.size(); i++) cout << files[i] << endl;
  return 0;
}
