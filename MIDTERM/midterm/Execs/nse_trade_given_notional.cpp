#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader_from_file_for_sim.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager_2.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/signals.hpp"
#include "midterm/GeneralizedLogic/paths.hpp"

void termHandler(int signum) {
  // handle sigints
  exit(0);
}

void LoadNSEFOSecuritiesUnderBan(int date_t, HFSAT::DebugLogger& dbglogger_t,
                                 std::set<std::string>& list_of_securities_under_ban_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_" << date_t << ".csv";

  std::ifstream fo_banned_securities_stream;
  fo_banned_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);
  if (!fo_banned_securities_stream.is_open()) {
    dbglogger_t << t_temp_oss.str() << "  FILE DOESNOT EXIST ";
    dbglogger_t << "Couldn't load banned products for today \n"
                << "\n";
    dbglogger_t.DumpCurrentBuffer();
  }
  char line_buffer[1024];

  while (fo_banned_securities_stream.good()) {
    fo_banned_securities_stream.getline(line_buffer, 1024);
    if (std::string(line_buffer).length() < 1) continue;
    list_of_securities_under_ban_.insert(std::string(line_buffer));
  }

  fo_banned_securities_stream.close();
}

bool IsInstrumentBanned(std::string instrument, std::set<std::string> list_of_securities_under_ban_,
                        HFSAT::DebugLogger& dbglogger_t) {
  // get underlying from shortcode, DLF from NSE_DLF_FUT0
  char inst[30];
  strcpy(inst, instrument.c_str());
  char* underlying = strtok(inst, "_\n");
  underlying = strtok(NULL, "_\n");

  if (list_of_securities_under_ban_.end() != list_of_securities_under_ban_.find(std::string(underlying))) {
    dbglogger_t << "Don't trade banned products. SLACK" << instrument << " is under ban today. Not trading.\n";
    dbglogger_t.DumpCurrentBuffer();
    return true;
  }
  return false;
}

void InitTradesLogger(int tradingdate, bool livetrading, HFSAT::BulkFileWriter& trades_writer_, std::string path_) {

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << path_ << "trades." << tradingdate;
  std::string tradesfilename = t_temp_oss_.str();
  // open trades file in append mode in livetrading_
  trades_writer_.Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));
}

std::vector<NSE_SIMPLEEXEC::ParamSet*> GetMultParams(const std::string& _paramfilename_) {
  std::vector<NSE_SIMPLEEXEC::ParamSet*> mult_params;
  std::ifstream paramfile_;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  std::string line;

  while (getline(paramfile_, line)) {
    if (line.substr(0, 1) == "#" || line.empty()) {
      continue;
    }

    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(line, '\t', tokens_);

    // Now we need to get mult param for each shortcode
    std::vector<std::string> temp_shcs_ =
        HFSAT::NSESecurityDefinitions::GetAllOptionShortcodesForUnderlyingGeneric(tokens_[0]);

    std::vector<std::string> shcs_;
    std::string fut0_shc_ = "NSE_" + tokens_[0] + "_FUT0";
    std::string fut1_shc_ = "NSE_" + tokens_[0] + "_FUT1";
    shcs_.push_back( fut0_shc_ );
    shcs_.push_back( fut1_shc_ );

    // Filter
    for (auto shc_ : temp_shcs_) {

      std::vector<std::string> temp_tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(shc_, '_', temp_tokens_);

      // Filter out non near month contracts
      if (temp_tokens_[2].find("0") == std::string::npos) {
        continue;
      }

      shcs_.push_back(shc_);
    }

    for (auto shc_ : shcs_) {
      NSE_SIMPLEEXEC::ParamSet* param_ = new NSE_SIMPLEEXEC::ParamSet(line, shc_);
      // Don't subscribe to options when moneyness_ is 0
      if ( !HFSAT::NSESecurityDefinitions::IsFuture( shc_ ) ) {
        if ( param_->moneyness_ <= EPSILON ) {
    	  continue;
        }
        else {
          double last_close_ = HFSAT::NSESecurityDefinitions::GetLastClose( fut0_shc_ );
    	  double strike_ = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric( shc_ );
    	  if (std::abs(strike_/last_close_ - 1) > param_->moneyness_) {
    	    continue;
    	  }
        }
      }
      mult_params.push_back( param_ );
    }
  }
  return (mult_params);
}

typedef std::map< std::string, SyntheticLegInfo > leg_info_;

int main(int argc, char** argv) {
  signal(SIGINT, termHandler);
  signal(SIGPIPE, SIG_IGN);

  // check command line arguments
  if (argc != 5) {
    std::cerr << " usage : Input log_path param_file orders_file date" << std::endl;
    exit(0);
  }

  // Variable Initializations
  bool livetrading_ = false;
  std::string log_path(argv[1]);
  std::string filename_input(argv[2]);
  std::string orders_file(argv[3]);
  int tradingdate_ = atoi(argv[4]);

  HFSAT::BulkFileWriter trades_writer_(256 * 1024);  // 256KB

  std::vector< std::string > filename_tokens_;
  HFSAT::PerishableStringTokenizer::StringSplit(filename_input, '_', filename_tokens_);
  std::string strategy_type = filename_tokens_[filename_tokens_.size() -1];

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << LOG_DIR + strategy_type + EXECLOGS_PATH << tradingdate_;

  std::string logfilename_ = t_temp_oss_.str();

  // Initialize trades logger
  InitTradesLogger(tradingdate_, livetrading_, trades_writer_, log_path);

  // create debuglogger
  HFSAT::DebugLogger dbglogger_(256 * 1024, 1);
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ios_base::out);

  // Load banned products list
  std::set<std::string> list_of_securities_under_ban;
  LoadNSEFOSecuritiesUnderBan(tradingdate_, dbglogger_, list_of_securities_under_ban);

  // create watch, secname_indexer, load nse_sec definitions, smv map
  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();

  /////////NSE_EXEC_LOGIC_PARAMSET///////////
  std::vector<NSE_SIMPLEEXEC::ParamSet*> params;
  params = GetMultParams(filename_input);

  // Init secnameIndexer
  for (auto i = 0u; i < params.size(); i++) {
    NSE_SIMPLEEXEC::ParamSet* param_ = params[i];
    param_->yyyymmdd_ = tradingdate_;  // setting date here doesnt requires to change everytime in every param

    if ( !HFSAT::NSESecurityDefinitions::IsShortcode(param_->instrument_) ) continue;

    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(param_->instrument_);
    // add shortcodes to sec_name_indexer
    sec_name_indexer_.AddString(exchange_symbol_, param_->instrument_);
  }

  // create smvs
  std::cout << "CREATING SMVs..." << std::endl;
  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_ctr_);
    const char* _this_exch_symbol_ = sec_name_indexer_.GetSecurityNameFromId(t_ctr_);
    HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exch_symbol_, t_ctr_, HFSAT::kExchSourceNSE, true,
        "INVALID", "INVALID", "INVALID");
    p_smv_->SetL1OnlyFlag(false);
    std::cout << p_smv_->security_id() << '\t' << _this_shortcode_ << '\t'
    		  << HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric( _this_shortcode_ ) << '\t'
    		  << HFSAT::NSESecurityDefinitions::GetExchSymbolNSE( _this_shortcode_ ) << std::endl;
    sid_to_smv_ptr_map_.push_back(p_smv_);
    if (p_smv_->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_)
      p_smv_->InitializeSMVForIndexedBook();
  }
  std::cout << "CREATED SMVs..." << std::endl;

  // create book manager
  bool use_self_book_ = false;
  HFSAT::IndexedNSEMarketViewManager2* indexed_nse_market_view_manager_ =
      new HFSAT::IndexedNSEMarketViewManager2(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, use_self_book_);

  // create logged message filesources and add hooks
  HFSAT::HistoricalDispatcher historical_dispatcher_;
  leg_info_ info_map_;
  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {

    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(sec_name_indexer_.GetShortcodeFromId( t_ctr_ ));
    // don't trade banned products
    if (IsInstrumentBanned(sec_name_indexer_.GetShortcodeFromId(t_ctr_), list_of_securities_under_ban, dbglogger_)) {
      continue;
    }

    HFSAT::NSELoggedMessageFileSource* t_filesource_ = new HFSAT::NSELoggedMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, t_ctr_, exchange_symbol_, HFSAT::kTLocNSE);
    t_filesource_->SetExternalTimeListener(&watch_);
    t_filesource_->SetOrderGlobalListenerNSE(indexed_nse_market_view_manager_);
    historical_dispatcher_.AddExternalDataListener(t_filesource_);

    SyntheticLegInfo info_( sid_to_smv_ptr_map_[t_ctr_], params[t_ctr_], t_ctr_, t_filesource_ );
    info_map_.insert( std::make_pair( sec_name_indexer_.GetShortcodeFromId( t_ctr_ ), info_ ) );
  }

  // Pass info_map_ to order interface, since some exec logics need to be created on the fly - and this will allow us to keep these stored
  // Create file reader interface
  // In sim mode, we only use file based order reader

  NSE_SIMPLEEXEC::NseExecLogicHelper* sim_helper_ = new NSE_SIMPLEEXEC::NseExecLogicHelperForSim( watch_, dbglogger_, trades_writer_ );

  NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReader* order_reader_interface_ =
      new NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReaderFromFileForSim(watch_, dbglogger_, livetrading_, orders_file, info_map_, sim_helper_ );

  for (unsigned int i = 0; i < params.size(); i++) {
  // Create simple exec based on run type specified in param
    switch (params[i]->exec_logic_run_type_) {
      case NSE_SIMPLEEXEC::kGetOrdersFromStrategy: {
        break;
      }
      case NSE_SIMPLEEXEC::kDecideOrdersFromParamConstraints: {
        break;
      }
      default: {
        dbglogger_ << "Invalid execlogic run type. Please check config." << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }
    }
  }

  historical_dispatcher_.RunHist();
  trades_writer_.Close();
  return 0;
}
