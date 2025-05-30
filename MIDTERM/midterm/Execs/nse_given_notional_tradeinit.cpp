#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader_from_file_for_live.hpp"
#include <boost/algorithm/string.hpp>
#include "midterm/GeneralizedLogic/paths.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"
#include "nse_exec_logic_helper_for_live.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager_2.hpp"

HFSAT::ShortcodeRequestHelper *global_shc_request_helper;

#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader_from_TCP.hpp"
#include "baseinfra/LivePnls/live_base_pnl.hpp"
#include "baseinfra/LivePnls/live_pnl_writer.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "baseinfra/BaseTrader/base_live_trader.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#include "dvccode/Utils/signals.hpp"

void termHandler(int signum) {
  // handle sigints

  if (global_shc_request_helper) {
    global_shc_request_helper->RemoveAllShortcodesToListen();
    global_shc_request_helper = nullptr;
  }

  exit(0);
}

uint64_t GetTimeStamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

std::vector<NSE_SIMPLEEXEC::ParamSet *>
GetMultParams(const std::string &_paramfilename_,std::string strategy_name_) {
  std::ifstream file_;
  std::set<std::string> gen_options_all;
  std::string option_path = ALLOPTIONSGEN;
  option_path += "_" + strategy_name_;
  std::cout<<option_path<<std::endl;
  file_.open(option_path, std::ifstream::in);
  if (file_.is_open()) {
	char buf[1024];
	while (file_.good()) {
   	   	file_.getline(buf, 1024);
    	   	if (std::string(buf).length() < 1)
      	   		continue;
      		gen_options_all.insert(buf);
 	 }
  }
  else{
	std::cout<<"INFO::No ALL OPTION GEN FILE Exist!!" << std::endl;
  }
  std::cout<<"GEN ALL FOR OPTIONS"<<std::endl;
  for(auto itr = gen_options_all.begin(); itr != gen_options_all.end(); ++itr)
	  std::cout<<*itr<<std::endl;
  std::vector<NSE_SIMPLEEXEC::ParamSet *> mult_params;
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
    std::vector<std::string> temp_shcs_ = HFSAT::NSESecurityDefinitions::
        GetAllOptionShortcodesForUnderlyingGeneric(tokens_[0]);

    std::vector<std::string> shcs_;
    std::string fut0_shc_ = "NSE_" + tokens_[0] + "_FUT0";
    std::string fut1_shc_ = "NSE_" + tokens_[0] + "_FUT1";
    shcs_.push_back(fut0_shc_);
    shcs_.push_back(fut1_shc_);

    
    // Filter
    for (auto shc_ : temp_shcs_) {

      std::vector<std::string> temp_tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(shc_, '_', temp_tokens_);

      // Filter out non near month contracts
      if (temp_tokens_[2].find("0") == std::string::npos && temp_tokens_[2].find("1") == std::string::npos) {
        continue;
      }
      shcs_.push_back(shc_);
    }

    for (auto shc_ : shcs_) {
      NSE_SIMPLEEXEC::ParamSet *param_ =
          new NSE_SIMPLEEXEC::ParamSet(line, shc_);
      // Don't subscribe to options when moneyness_ is 0
      if (!HFSAT::NSESecurityDefinitions::IsFuture(shc_)) {
	if (gen_options_all.find(tokens_[0]) != gen_options_all.end()){
          // REMOVED SUPPORT FOR NXT EXPIRY FOR SHORTCODE other than BANKNIFTY,NIFTY
          std::vector<std::string> temp_tokens_;
          HFSAT::PerishableStringTokenizer::StringSplit(shc_, '_', temp_tokens_);
          int exp_num = atoi(temp_tokens_[2].substr(temp_tokens_[2].size() - 1, 1).c_str());
          if ( exp_num == 0 ) {
            mult_params.push_back(param_);
          }
          else if ( exp_num > 0 && shc_.find("NIFTY") != std::string::npos){
	    mult_params.push_back(param_);
          }
	  continue;
	}
	else if (param_->moneyness_ <= EPSILON) {
          continue;
        } else {
          double last_close_ =
              HFSAT::NSESecurityDefinitions::GetLastClose(fut0_shc_);
          double strike_ =
              HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(
                  shc_);
          std::vector<std::string> temp_tokens_;
          HFSAT::PerishableStringTokenizer::StringSplit(shc_, '_', temp_tokens_);

          int exp_num = atoi(temp_tokens_[2].substr(temp_tokens_[2].size() - 1, 1).c_str());
          if (std::abs(strike_ / last_close_ - 1) > param_->moneyness_ ||
              exp_num > param_->expiry_to_look_till) {
            continue;
          }
          std::cout<<" shc_ " << shc_ << "exp_num: " << exp_num << " PARAM " << param_->expiry_to_look_till << std::endl;
        }
      }
      mult_params.push_back(param_);
    }
  }
  return (mult_params);
}

int Get_Qid_From_Strategy(std::string strategy_name_) {

  if (HFSAT::IsItSimulationServer()) {
    return 1000000;
  }

  std::ifstream file_;
  file_.open(QID_FILE, std::ifstream::in);
  if (!file_.is_open()) {
    std::cout << "Error reading the qid file!!" << std::endl;
    return -1;
  }
  char buf[1024];
  while (file_.good()) {
    file_.getline(buf, 1024);
    if (std::string(buf).length() < 1)
      continue;
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(buf, '\t', tokens_);
    if (tokens_.size() < 2)
      std::cout << "File must contain tab delimited strategy name and qid\n";
    else if (tokens_[0] == strategy_name_)
      return atoi(tokens_[1].c_str());
  }
  std::cout << "Finished reading the qid file but strategy name not found!!\n";
  return -1;
}

int Get_ServerPort_From_Strategy(std::string strategy_name_) {

  if (HFSAT::IsItSimulationServer()) {
    return 45042;
  }

  std::ifstream file_;
  file_.open(SERVER_PORTS_FILE, std::ifstream::in);
  if (!file_.is_open()) {
    std::cout << "Error reading the port number file!!" << std::endl;
    return -1;
  }
  char buf[1024];
  while (file_.good()) {
    file_.getline(buf, 1024);
    if (std::string(buf).length() < 1)
      continue;
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(buf, '\t', tokens_);
    if (tokens_.size() != 2)
      std::cout
          << "File must contain tab delimited strategy name and port number"
          << std::endl;
    else if (tokens_[0] == strategy_name_)
      return atoi(tokens_[1].c_str());
  }
  std::cout << "Finished reading the ports file but strategy name not found!!"
            << std::endl;
  return -1;
}

void LoadNSEFOSecuritiesUnderBan(
    int date_t, HFSAT::DebugLogger &dbglogger_t,
    std::set<std::string> &list_of_securities_under_ban_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_"
             << date_t << ".csv";

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
    if (std::string(line_buffer).length() < 1)
      continue;
    list_of_securities_under_ban_.insert(std::string(line_buffer));
  }

  fo_banned_securities_stream.close();
}

bool IsInstrumentBanned(std::string instrument,
                        std::set<std::string> list_of_securities_under_ban_,
                        HFSAT::DebugLogger &dbglogger_t) {
  // get underlying from shortcode, DLF from NSE_DLF_FUT0
  char inst[30];
  strcpy(inst, instrument.c_str());
  char *underlying = strtok(inst, "_\n");
  underlying = strtok(NULL, "_\n");

  if (list_of_securities_under_ban_.end() !=
      list_of_securities_under_ban_.find(std::string(underlying))) {
    dbglogger_t << instrument << " cant be traded as it is under ban today\n";
    dbglogger_t.DumpCurrentBuffer();
    return true;
  }
  return false;
}

typedef std::map<std::string, SyntheticLegInfo> leg_info_;

int main(int argc, char **argv) {
  signal(SIGINT, termHandler);

  // check command line arguments
  if (argc != 3) {
    std::cerr << " usage : Input param_file i/o(TCP or FILE)" << std::endl;
    exit(0);
  }

  // Variable Initializations
  bool livetrading_ = true;
  std::string filename_input(argv[1]);
  std::string io_mode(argv[2]);
  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::Utils::ClientLoggingSegmentInitializer
      *client_logging_segment_initializer_ptr = NULL;

  // Get Straetegy type from param filename
  std::vector<std::string> filename_tokens_;
  HFSAT::PerishableStringTokenizer::StringSplit(filename_input, '_',
                                                filename_tokens_);
  std::string strategy_type = filename_tokens_[filename_tokens_.size() - 1];
  int qid = Get_Qid_From_Strategy(strategy_type);
  if (qid == -1)
    return 0;

  int32_t server_port_ = Get_ServerPort_From_Strategy(strategy_type);
  if (server_port_ == -1)
    return 0;

  // create debuglogger
  HFSAT::DebugLogger dbglogger_(256 * 1024, 1);
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << LOG_DIR + strategy_type + EXECLOGS_PATH << tradingdate_;
  std::string logfilename_ = t_temp_oss_.str();
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ios_base::app);

  // Load banned products list
  std::set<std::string> list_of_securities_under_ban;
  LoadNSEFOSecuritiesUnderBan(tradingdate_, dbglogger_,
                              list_of_securities_under_ban);

  // Initialize client_logging_segment_initializer_ptr
  char offload_logging_filedir_char[512];
  char offload_logging_filename_char[256];

  memset((void *)offload_logging_filedir_char, 0, 512);
  memset((void *)offload_logging_filename_char, 0, 256);

  std::ostringstream offload_logging_filename;
  offload_logging_filename << "nse_complex_exec_trades." << tradingdate_ << "."
                           << GetTimeStamp() << '\0';
  std::string offload_logging_filedir = "/spare/local/logs/tradelogs/";

  memcpy((void *)offload_logging_filedir_char, offload_logging_filedir.c_str(),
         offload_logging_filedir.length());
  memcpy((void *)offload_logging_filename_char,
         offload_logging_filename.str().c_str(),
         offload_logging_filename.str().length());

  HFSAT::Utils::ClientLoggingSegmentInitializer
      client_logging_segment_initializer(dbglogger_, 1000,
                                         offload_logging_filedir_char,
                                         offload_logging_filename_char);
  client_logging_segment_initializer_ptr = &client_logging_segment_initializer;

  // create watch, secname_indexer, load nse_sec definitions, smv
  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::SecurityNameIndexer &sec_name_indexer_ =
      HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_)
      .LoadNSESecurityDefinitions();
  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map_ =
      HFSAT::sid_to_security_market_view_map();
  // NSE_EXEC_LOGIC_PARAMSET
  std::vector<NSE_SIMPLEEXEC::ParamSet *> params;
  params = GetMultParams(filename_input,strategy_type);
  for(unsigned int i=0;i<params.size();i++)std::cout<<params[i]->instrument_<<std::endl;
  // Create file reader interface incase we need orders from strategy
  std::string ordersfile_path_ = HFSAT::IsItSimulationServer()
                                     ? TEST_SERVER_ORDERSFILE_PATH
                                     : ORDERSFILE_PATH;
  std::string orders_file = ordersfile_path_ + strategy_type; // Todo: as input

  // Create backup file path -- TODO move dated dat file to standard format
  std::string backup_file = std::string(LOG_DIR) + std::string(BKP_TRADEFILE_PATH) + strategy_type + ".dat";
  //TODO -- remove logging after testing
  dbglogger_ << "Backup file set to " << backup_file << '\n'; 
  

  NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReader *order_reader_interface_;

  // if order is to be received via tcp or read from file
  if (io_mode == "TCP" || io_mode == "tcp") {
    // Do nothing
    std::cout << "Running in TCP mode" << std::endl;
  } else if (io_mode == "FILE" || io_mode == "file") {
    // Do Nothing
    std::cout << "Running in FILE mode" << std::endl;
  } else {
    std::cerr << "Incorrect io_mode. Expected TCP or FILE " << std::endl;
    exit(-1);
  }

  std::vector<std::string> shortcodes_to_listen;

  for (unsigned int i = 0; i < params.size(); i++) {
    NSE_SIMPLEEXEC::ParamSet *param_ = params[i];
    param_->yyyymmdd_ = tradingdate_;

    if (!HFSAT::NSESecurityDefinitions::IsShortcode(param_->instrument_))
      continue;
    shortcodes_to_listen.push_back(param_->instrument_);
    const char *exchange_symbol_ =
        HFSAT::ExchangeSymbolManager::GetExchSymbol(param_->instrument_);
    // add shortcodes to sec_name_indexer
    sec_name_indexer_.AddString(exchange_symbol_, param_->instrument_);
  }

  HFSAT::ShortcodeRequestHelper shc_req_helper(qid);
  global_shc_request_helper = &shc_req_helper;
  shc_req_helper.AddShortcodeListToListen(shortcodes_to_listen);

  // create smvs
  std::cout << "CREATING SMVs..." << std::endl;
  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId();
       t_ctr_++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_ctr_);
    const char *_this_exch_symbol_ =
        sec_name_indexer_.GetSecurityNameFromId(t_ctr_);
    HFSAT::SecurityMarketView *p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, _this_shortcode_,
        _this_exch_symbol_, t_ctr_, HFSAT::kExchSourceNSE, true,
        "INVALID", "INVALID", "INVALID");
    p_smv_->SetL1OnlyFlag(false);
    std::cout
        << p_smv_->security_id() << '\t' << _this_shortcode_ << '\t'
        << HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(
               _this_shortcode_)
        << '\t'
        << HFSAT::NSESecurityDefinitions::GetExchSymbolNSE(_this_shortcode_)
        << std::endl;
    sid_to_smv_ptr_map_.push_back(p_smv_);
    if (p_smv_->market_update_info_
            .temporary_bool_checking_if_this_is_an_indexed_book_)
      p_smv_->InitializeSMVForIndexedBook();
  }
  std::cout << "CREATED SMVs..." << std::endl;

  // create book manager
  std::cout << "BOOK Mangagers..." << std::endl;
  bool use_self_book_ = false;
  HFSAT::IndexedNSEMarketViewManager2 *indexed_nse_market_view_manager_ =
      new HFSAT::IndexedNSEMarketViewManager2(
          dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_,use_self_book_);
  // combined mds processor to manage shm messages and pass on to nse book
  // manager
  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor
      combined_mds_messages_shm_processor_(dbglogger_, sec_name_indexer_,
                                           HFSAT::kComShmConsumer);

  // we only process ORS messages and nse market updates from shm
  combined_mds_messages_shm_processor_.AddORSreplySourceForProcessing(
      HFSAT::MDS_MSG::ORS_REPLY, &watch_, sid_to_smv_ptr_map_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::NSE,
      (void *)((
          HFSAT::OrderGlobalListenerNSE *)(indexed_nse_market_view_manager_)),
      &watch_);
  leg_info_ info_map_;
  // create live trader, order manager and dependent objects
  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId();
       t_ctr_++) {
    // don't trade if this product is banned
    if (IsInstrumentBanned(sec_name_indexer_.GetShortcodeFromId(t_ctr_),
                           list_of_securities_under_ban, dbglogger_)) {
      continue;
    }
    SyntheticLegInfo info_(sid_to_smv_ptr_map_[t_ctr_], params[t_ctr_], t_ctr_,
                           nullptr);
    info_map_.insert(
        std::make_pair(sec_name_indexer_.GetShortcodeFromId(t_ctr_), info_));
  }
  NSE_SIMPLEEXEC::NseExecLogicHelper *live_helper_ =
      new NSE_SIMPLEEXEC::NseExecLogicHelperForLive(
          watch_, dbglogger_, client_logging_segment_initializer_ptr,
          combined_mds_messages_shm_processor_, strategy_type);
  // if order is to be received via tcp or read from file
  if (io_mode == "TCP" || io_mode == "tcp") {
    order_reader_interface_ =
        new NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReaderFromTCP(
            watch_, dbglogger_, livetrading_, info_map_, live_helper_,
            server_port_);
  } else if (io_mode == "FILE" || io_mode == "file") {
    order_reader_interface_ =
        new NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReaderFromFileForLive(
            watch_, dbglogger_, livetrading_, orders_file, backup_file, info_map_,
            live_helper_);
  } else {
    std::cerr << "Incorrect io_mode. Expected TCP or FILE " << std::endl;
    exit(-1);
  }
  watch_.subscribe_BigTimePeriod(order_reader_interface_);
  for (unsigned int i = 0; i < params.size(); i++) {
    // Create simple exec based on run type specified in param
    switch (params[i]->exec_logic_run_type_) {
    case NSE_SIMPLEEXEC::kGetOrdersFromStrategy: {
      break;
    }
    default: {
      dbglogger_ << "Invalid execlogic run type. Please check config."
                 << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
    }
  }
  // Do not change logging format (risk monitor uses this line): Abhishek
  dbglogger_ << "DUMPEDALLSACIS (logging for risk monitor)\n";
  dbglogger_.DumpCurrentBuffer();
  combined_mds_messages_shm_processor_.RunLiveShmSource();
  std::cout << "Ready to receive generalized orders..." << std::endl;
  return 0;
}
