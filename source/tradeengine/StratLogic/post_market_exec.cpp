/**
   \file StratLogic/pos_exec.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include <inttypes.h>
#include <signal.h>

#include <boost/program_options.hpp>
#include "baseinfra/BaseTrader/base_live_trader.hpp"
#include "baseinfra/BaseTrader/base_sim_trader.hpp"
#include "baseinfra/LivePnls/live_base_pnl.hpp"
#include "baseinfra/SimPnls/sim_base_pnl.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/Utils/signals.hpp"
#include "dvccode/Utils/thread.hpp"
#include "tradeengine/Utils/Parser.hpp"
#include "tradeengine/CommonInitializer/common_initializer.hpp"
#include "tradeengine/Executioner/MarketOrderExecution.hpp"
#include "tradeengine/StratLogic/user_control_manager.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"

// For Offloaded Logging
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"

#if USE_UDP_DIRECT_SHM1_MERGE
#include "baseinfra/MDSMessages/combined_mds_messages_direct_processor.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "infracore/NSEMD/nse_tbt_data_processor.hpp"
#include "infracore/NSEMD/nse_tbt_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_tbt_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_tbt_data_processor.hpp"
#else
#include "baseinfra/MDSMessages/combined_mds_messages_multi_shm_processor.hpp"
#endif

#define CHANNEL_INFO_FILE "/spare/local/files/NSE/channel_info.txt"
#define CHANNEL_DETAILS_FILE "/home/pengine/prod/live_configs/channel_to_subscribe_strat.txt"
typedef std::map<std::string, std::string> KEY_VAL_MAP;

// HFSAT::OnlineDebugLogger *online_dbglogger_ = new HFSAT::OnlineDebugLogger(1024 * 4 * 1024, 1109, 256 * 1024);
// HFSAT::DebugLogger &dbglogger_ = *(online_dbglogger_);  //( 4*1024*1024, 256*1024 ); // making logging more
// efficient,
// otherwise change it backto 10240, 1

HFSAT::DebugLogger dbglogger_(1024000, 1);

HFSAT::Utils::ClientLoggingSegmentInitializer *client_logging_segment_initializer_ptr = NULL;
HFSAT::ShortcodeRequestHelper *global_shc_request_helper;

typedef std::map<std::string, int> KEY_INT_MAP;
std::vector<MarketOrderExecution *> mkt_exec_vec_;
std::map<std::string, std::string> shc_pos_map;
std::map<char, std::string> fut_stocks_channels_;
std::map<char, std::string> cash_channels_;
std::map<std::string, std::string> index_fut_channels_;
std::map<std::string,vector<std::string>> index_opt_channels_;
std::map<std::string, std::string> index_spot_channels_;
std::map<std::string, std::string> bse_index_spot_channels_;
std::map<std::string, std::string> bse_oi_channels_;
std::multimap<std::string, std::string> bse_eq_channels_;
std::multimap<std::string, std::string> bse_fo_channels_;


std::vector<std::string> shc_vec_;
int progid_;
int tradingdate_;

void termination_handler(int signum) {
  std::cout << "Exiting due to signal " << SimpleSignalString(signum) << std::endl;
  char hostname_[128];
  hostname_[127] = '\0';
  gethostname(hostname_, 127);

  std::string position_email_string_ = "";
  std::ostringstream t_oss_;

  t_oss_ << "NSE Post Market Execution Strategy: " << progid_ << " exiting signal: " << SimpleSignalString(signum)
         << " on " << hostname_ << "\n";

  std::string subject_email_string_ = t_oss_.str();
  position_email_string_ = position_email_string_ + t_oss_.str();

  for (auto mkt_exec_ : mkt_exec_vec_) {
    int pos_remaning_ = mkt_exec_->GetPosition() - stoi(shc_pos_map[mkt_exec_->GetSecondaryShc()]);
    if (pos_remaning_ != 0) {
      std::ostringstream t_oss_1;
      t_oss_1 << "SHC: " << mkt_exec_->GetSecondaryShc() << " POS_EXECUTED: " << mkt_exec_->GetPosition()
              << " POS_REMAINING: " << pos_remaning_ << "\n";
      position_email_string_ = position_email_string_ + t_oss_1.str();
    }
    std::cout << "SHC: " << mkt_exec_->GetSecondaryShc() << " POS_EXECUTED: " << mkt_exec_->GetPosition()
              << " POS_REMAINING: " << pos_remaning_ << "\n";
  }

  //  if (position_email_string_ != subject_email_string_) {
  HFSAT::Email email_;
  email_.setSubject(subject_email_string_);
  email_.addRecepient("nsehft@tworoads.co.in");
  email_.addSender("uttkarsh.sarraf@tworoads.co.in");
  email_.content_stream << position_email_string_ << "<br/>";
  email_.sendMail();
  //  }

  dbglogger_.Close();
  if (SIGINT != signum) {  // Unexpected
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
  } else {
    exit(0);
  }
}

void LoadChannelToSubscribe(){
  std::ifstream shc_to_subscribe_info_file;
  shc_to_subscribe_info_file.open(CHANNEL_DETAILS_FILE, std::ifstream::in);
  if (!shc_to_subscribe_info_file.is_open()) {
    std::cerr << "UNABLE TO OPEN CHANNEL To Subscribe FILE FOR READING :" << CHANNEL_DETAILS_FILE << std::endl;
    exit (-1);
  }
  char line_[1024];
  while (!shc_to_subscribe_info_file.eof()) {
    memset(line_, 0, sizeof(line_));
    shc_to_subscribe_info_file.getline(line_, sizeof(line_));
    HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    const std::vector<const char *> &tokens_ = t_tokenizer_.GetTokens();
    if (tokens_.size() < 5 || tokens_[0][0] == '#') {
      std::cerr << "CHANNEL INFO : IGNORING LINE : " << line_ << std::endl;
      continue;
    }
    char channel_start = tokens_[1][0];
    if (strcmp(tokens_[0],"FUT") == 0){
      cout<<"FUT " << channel_start << " CHAN " << tokens_[3] << std::endl;
      fut_stocks_channels_[channel_start] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "EQ") == 0){
      cout<<"EQ " << channel_start << " CHAN " << tokens_[3] << std::endl;
      cash_channels_[channel_start] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "FUTIDX") == 0){
      cout<<"IND_FUT " << tokens_[1] << " CHAN "<< tokens_[3] << std::endl;
      index_fut_channels_[tokens_[1]] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "OPTIDX") == 0 ){
      cout<<"IND_OP " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      index_opt_channels_[tokens_[1]].push_back(std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]));
    } else if (strcmp(tokens_[0], "IDXSPT") == 0 ){
      cout<<"IND_SPOT " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      index_spot_channels_[tokens_[1]] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    }  else if (strcmp(tokens_[0], "BSESPT") == 0 ){
      cout<<"BSE_SPOT " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      bse_index_spot_channels_[tokens_[1]] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "BSEOI") == 0 ){
      cout<<"BSE_OI " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      bse_oi_channels_[tokens_[1]] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "BSE_EQ") == 0 ){
      cout<<"BSE_EQ " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      string channel_info = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
      bse_eq_channels_.insert(std::make_pair(tokens_[1],channel_info));
    } else if (strcmp(tokens_[0], "BSE_FO") == 0 ){
      cout<<"BSE_FO " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      string channel_info = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
      bse_fo_channels_.insert(std::make_pair(tokens_[1],channel_info));
    }
  }
}

void InitDbglogger(int tradingdate, int progid, std::vector<std::string> &dbg_code_vec,
                   CommonInitializer *common_initializer_, const std::string &logs_directory, bool livetrading_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << logs_directory << "/log." << tradingdate << "." << progid;
  std::string logfilename_ = t_temp_oss.str();

  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ios::app);

  for (auto i = 0u; i < dbg_code_vec.size(); i++) {
    // TODO .. add ability to write "WATCH_INFO" instead of 110, and making it
    int dbg_code_to_be_logged = HFSAT::DebugLogger::TextToLogLevel(dbg_code_vec[i].c_str());
    if (dbg_code_to_be_logged <= 0) {
      dbglogger_.SetNoLogs();
      break;
    } else {
      dbglogger_.AddLogLevel(dbg_code_to_be_logged);
    }
  }

  // Though we do not exit here, but since it is a very very rare case and important to detect,
  // we are logging OM_ERROR and WATCH_ERROR for every default SIM run.
  dbglogger_.AddLogLevel(WATCH_ERROR);
  dbglogger_.AddLogLevel(OM_ERROR);
  dbglogger_.AddLogLevel(PLSMM_ERROR);
  dbglogger_.AddLogLevel(BOOK_ERROR);
  dbglogger_.AddLogLevel(BOOK_INFO);

  // dbglogger.AddLogLevel(TRADING_ERROR);
  // dbglogger.AddLogLevel(BOOK_ERROR);
  // dbglogger.AddLogLevel(LRDB_ERROR);
  dbglogger_.AddLogLevel(DBG_MODEL_ERROR);
  //  dbglogger.AddLogLevel(SMVSELF_ERROR);
}

void CreateShortcodeLists(std::map<std::string, std::string> &shc_pos_map,
                          std::vector<std::string> &source_shortcode_list_,
                          std::vector<std::string> &ors_shortcode_list_) {
  for (auto &shc_pos_iter : shc_pos_map) {
    HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_list_, shc_pos_iter.first);
    HFSAT::VectorUtils::UniqueVectorAdd(ors_shortcode_list_, shc_pos_iter.first);
  }
}

void LoadShortCodeChannelInfo(KEY_VAL_MAP &shc_to_channel_info) {
  std::ifstream shc_to_channel_info_file;
  shc_to_channel_info_file.open(CHANNEL_INFO_FILE, std::ifstream::in);
  if (!shc_to_channel_info_file.is_open()) {
    std::cerr << "UNABLE TO OPEN CHANNEL INFO FILE FOR READING :" << CHANNEL_INFO_FILE << std::endl;
    return;
  }
  char line_[1024];
  while (!shc_to_channel_info_file.eof()) {
    memset(line_, 0, sizeof(line_));
    shc_to_channel_info_file.getline(line_, sizeof(line_));
    HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    const std::vector<const char *> &tokens_ = t_tokenizer_.GetTokens();
    if (tokens_.size() < 4 || tokens_[0][0] == '#') {
      std::cerr << "CHANNEL INFO : IGNORING LINE : " << line_ << std::endl;
      continue;
    }

    std::stringstream tmp_shortcode_;
    std::stringstream tmp_channel_;
    tmp_shortcode_ << tokens_[0];
    // streamnum, ip, port
    tmp_channel_ << tokens_[1] << " " << tokens_[2] << " " << tokens_[3];
    shc_to_channel_info.insert(std::make_pair<std::string, std::string>(tmp_shortcode_.str(), tmp_channel_.str()));
  }
}

HFSAT::BaseTrader *GetNSETrader(std::string shortcode_, int security_id_,
                                HFSAT::NetworkAccountInfoManager &network_account_info_manager_, HFSAT::Watch &watch_,int test_flag_ = -1) {
  HFSAT::Utils::NSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);

  HFSAT::SecurityDefinitions &nse_sec_def = HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);
  char segment = nse_sec_def.GetSegmentTypeFromShortCode(shortcode_);

  if (NSE_INVALID_SEGMENT == segment) {
    dbglogger_ << "SHORTCODE : " << shortcode_ << " RETURNS INVALID NSE SEGMENT \n";
    dbglogger_.DumpCurrentBuffer();
    exit(-1);
  }

  std::string const internal_symbol =
      HFSAT::SecurityNameIndexer::GetUniqueInstance().GetSecurityNameFromId(security_id_);

  std::string const data_symbol_name = nse_sec_def.ConvertExchSymboltoDataSourceName(internal_symbol, shortcode_);
  if (std::string("INVALID") == data_symbol_name) {
    dbglogger_ << "COULDN'T FIND DATASYMBOL FROM INTERNAL SYMBOL : " << internal_symbol << " SEC ID : " << security_id_
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE EXITING..." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (-1 == nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(data_symbol_name.c_str(), segment)) {
    dbglogger_ << "COULDN'T FIND TOKEN FOR INTERNAL SYMBOL : " << data_symbol_name << " WITH SEC ID : " << security_id_
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE EXITING..." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  // Get Token
  int32_t const this_symbol_token =
      nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(data_symbol_name.c_str(), segment);

  // Get RefData Info From RefLoader
  HFSAT::Utils::NSERefDataLoader &nse_ref_data_loader = HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(tradingdate_);
  std::map<int32_t, NSE_UDP_MDS::NSERefData> &nse_ref_data = nse_ref_data_loader.GetNSERefData(segment);

  if (0 == nse_ref_data.size()) {
    dbglogger_ << "FOR SEGMENT : " << segment << " REF DATA DOESN't EXISTS" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  // This is the ORS segment which we need to connect to for this live trader/strat
  HFSAT::ExchSource_t nse_segment_source = nse_ref_data[this_symbol_token].exch_source;
  HFSAT::BaseTrader *p_base_trader_ = new HFSAT::BaseLiveTrader(
      nse_segment_source, network_account_info_manager_.GetDepTradeAccount(HFSAT::kExchSourceNSE, shortcode_),
      network_account_info_manager_.GetDepTradeHostIp(HFSAT::kExchSourceNSE, shortcode_),
      network_account_info_manager_.GetDepTradeHostPort(HFSAT::kExchSourceNSE, shortcode_), watch_, dbglogger_,test_flag_);

  return p_base_trader_;
}

HFSAT::BaseTrader *GetBSETrader(std::string shortcode_, int security_id_,
                                HFSAT::NetworkAccountInfoManager &network_account_info_manager_, HFSAT::Watch &watch_,int test_flag_ = -1) {
  HFSAT::Utils::BSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler =
      HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);

  HFSAT::SecurityDefinitions &bse_sec_def = HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);
  char segment = bse_sec_def.GetSegmentTypeFromShortCode(shortcode_);

  if (BSE_INVALID_SEGMENT == segment) {
    dbglogger_ << "SHORTCODE : " << shortcode_ << " RETURNS INVALID BSE SEGMENT \n";
    dbglogger_.DumpCurrentBuffer();
    exit(-1);
  }

  std::string const internal_symbol =
      HFSAT::SecurityNameIndexer::GetUniqueInstance().GetSecurityNameFromId(security_id_);

  std::string const data_symbol_name = bse_sec_def.ConvertExchSymboltoDataSourceName(internal_symbol, shortcode_);
  if (std::string("INVALID") == data_symbol_name) {
    dbglogger_ << "COULDN'T FIND DATASYMBOL FROM INTERNAL SYMBOL : " << internal_symbol << " SEC ID : " << security_id_
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE EXITING..." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  if (-1 == bse_daily_token_symbol_handler.GetTokenFromInternalSymbol(data_symbol_name.c_str(), segment)) {
    dbglogger_ << "COULDN'T FIND TOKEN FOR INTERNAL SYMBOL : " << data_symbol_name << " WITH SEC ID : " << security_id_
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE EXITING..." << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  // Get Token
  int32_t const this_symbol_token =
      bse_daily_token_symbol_handler.GetTokenFromInternalSymbol(data_symbol_name.c_str(), segment);

  // Get RefData Info From RefLoader
  HFSAT::Utils::BSERefDataLoader &bse_ref_data_loader = HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(tradingdate_);
  std::map<int32_t, BSE_UDP_MDS::BSERefData> &bse_ref_data = bse_ref_data_loader.GetBSERefData(segment);
  if (0 == bse_ref_data.size()) {
    dbglogger_ << "FOR SEGMENT : " << segment << " REF DATA DOESN't EXISTS" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  // This is the ORS segment which we need to connect to for this live trader/strat
  HFSAT::ExchSource_t bse_segment_source = bse_ref_data[this_symbol_token].exch_source;

  HFSAT::BaseTrader *p_base_trader_ = new HFSAT::BaseLiveTrader(
      bse_segment_source, network_account_info_manager_.GetDepTradeAccount(HFSAT::kExchSourceBSE, shortcode_),
      network_account_info_manager_.GetDepTradeHostIp(HFSAT::kExchSourceBSE, shortcode_),
      network_account_info_manager_.GetDepTradeHostPort(HFSAT::kExchSourceBSE, shortcode_), watch_, dbglogger_,test_flag_);

  return p_base_trader_;
}

#if USE_UDP_DIRECT_SHM1_MERGE
void SubscribeToORSReply(HFSAT::MDSMessages::CombinedMDSMessagesDirectProcessor *combined_mds_messages_shm_processor,
                         HFSAT::BasicOrderManager *smart_om, int security_id) {
#else
void SubscribeToORSReply(HFSAT::MDSMessages::CombinedMDSMessagesMultiShmProcessor *combined_mds_messages_shm_processor,
                         HFSAT::BasicOrderManager *smart_om, int security_id) {
#endif
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderNotFoundListener(security_id, smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderConfirmedListener(security_id, smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderConfCxlReplacedListener(security_id,
                                                                                                     smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderConfCxlReplaceRejectListener(security_id,
                                                                                                          smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderCanceledListener(security_id, smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderExecutedListener(security_id, smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderRejectedListener(security_id, smart_om);
  //  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderInternallyMatchedListener(security_id,
  //  smart_om);
}

void InsertShc(const std::string &_config_filename_, std::vector<std::string> &_shc_vec_) {
  std::ifstream ifs_(_config_filename_.c_str(), std::ifstream::in);
  if (ifs_.is_open()) {
    const int kBufferLength = 1024;
    char buffer_[kBufferLength];
    while (ifs_.good()) {
      bzero(buffer_, kBufferLength);
      ifs_.getline(buffer_, kBufferLength);
      std::string t_val_;
      HFSAT::PerishableStringTokenizer::TrimString(buffer_, t_val_);

      std::vector<char *> tokens_;
      HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(buffer_, " =", tokens_);
      if (tokens_.empty() || tokens_[0][0] == '#' || tokens_.size() < 2) {
        std::cerr << " Cant handle inappropriate line " << std::endl;
        continue;
      } else {
        // std::cout << " Adding to map " << tokens_[0] << " === " << tokens_[1] << std::endl;
        _shc_vec_.push_back(tokens_[0]);
      }
    }
    ifs_.close();
  } else {
    std::cerr << "Could not open " << _config_filename_ << " for reading " << std::endl;
  }
}

int main(int argc, char **argv) {
  // Assume we get a file source list and we have to print Market Updates
  // Arg1 : Config File
  // Arg2 : Trading date
  // Arg3 : start time
  // Arg4 : trigger time
  // Arg5 : Prog ID

  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGPIPE, SIG_IGN);

  if (argc < 6) {
    std::cerr << "expecting :\n"
              << " $postmarketexec pos_file_to_execute TRADINGDATE START_TIME TRIGGER_TIME PROGID --exchange=[NSE]/BSE" << '\n';
    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  }

  std::string _position_file = argv[1];
  tradingdate_ = atoi(argv[2]);
  int midnight_mfm_ = 0;
  int start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[3] + 4), argv[3]);
  int trigger_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[4] + 4), argv[4]);
  int trigger_utc_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, trigger_utc_hhmm_, "UTC_");

  progid_ = atoi(argv[5]);
  std::string exchange_ = "NSE";

  boost::program_options::options_description desc("Allowed Options");
  desc.add_options()("help", "produce help message.")("exchange", boost::program_options::value<std::string>()->default_value("NSE"));

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  std::string exchange_flag = vm["exchange"].as<std::string>();
  HFSAT::TradingLocation_t dep_trading_location_ = HFSAT::kTLocNSE;
  exchange_ = exchange_flag;

  //pos exec can only run for 1 exchange
  bool are_we_using_hybrid_exchange = false;
 
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGPIPE, SIG_IGN);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  //Must be called before any further exchange specific calls
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).SetExchangeType(exchange_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadSecurityDefinitions();

  dep_trading_location_ = ("BSE" == exchange_ || "BSE_NSE" == exchange_) ? HFSAT::kTLocBSE : HFSAT::kTLocNSE;


  // std::vector<std::string> dbg_code_vec = {"PLSMM_INFO","OM_INFO","SIM_ORDER_INFO","ORS_DATA_INFO"};
  std::vector<std::string> dbg_code_vec = {"OM_INFO", "FILL_TIME_INFO", "SIM_ORDER_INFO", "ORS_DATA_INFO"};

  Parser::ParseConfig(_position_file, shc_pos_map);
  InsertShc(_position_file, shc_vec_);

  std::vector<std::string> source_shortcode_list_;
  std::vector<std::string> ors_shortcode_list_;
  CreateShortcodeLists(shc_pos_map, source_shortcode_list_, ors_shortcode_list_);

  int trading_start_utc_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, start_utc_hhmm_, "UTC_");

  CommonInitializer *common_intializer =
      new CommonInitializer(source_shortcode_list_, ors_shortcode_list_, tradingdate_, dbglogger_, dep_trading_location_, true);
  std::string logs_directory = "/spare/local/logs/tradelogs";

  // Setup Logger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec, common_intializer, logs_directory, true);
  HFSAT::BulkFileWriter trades_writer_(256 * 1024);

  char offload_logging_filedir_char[512];
  char offload_logging_filename_char[256];

  memset((void *)offload_logging_filedir_char, 0, 512);
  memset((void *)offload_logging_filename_char, 0, 256);

  std::ostringstream offload_logging_filename;
  offload_logging_filename << "trades." << tradingdate_ << "." << progid_ << '\0';
  std::string offload_logging_filedir = "/spare/local/logs/tradelogs/";

  memcpy((void *)offload_logging_filedir_char, offload_logging_filedir.c_str(), offload_logging_filedir.length());
  memcpy((void *)offload_logging_filename_char, offload_logging_filename.str().c_str(),
         offload_logging_filename.str().length());

  HFSAT::Utils::ClientLoggingSegmentInitializer *client_logging_segment_initializer =
      new HFSAT::Utils::ClientLoggingSegmentInitializer(dbglogger_, progid_, offload_logging_filedir_char,
                                                        offload_logging_filename_char);
  client_logging_segment_initializer_ptr = client_logging_segment_initializer;

  HFSAT::ShortcodeRequestHelper shc_req_helper(progid_);
  global_shc_request_helper = &shc_req_helper;
  shc_req_helper.AddShortcodeListToListen(source_shortcode_list_);

  // Initialize the smv source after setting the required variables
  common_intializer->SetStartEndTime(start_utc_hhmm_, 1100);
  common_intializer->SetRuntimeID(progid_);
  common_intializer->Initialize();
  std::vector<HFSAT::SecurityMarketView *> &p_smv_vec_ = common_intializer->getSMVMap();
  HFSAT::Watch &watch_ = common_intializer->getWatch();
  watch_.ResetWatch(tradingdate_);
  midnight_mfm_ = HFSAT::DateTime::GetTimeMidnightUTC(tradingdate_);
  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  int runtime_id_ = progid_;

  HFSAT::MarketUpdateManager &market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, common_intializer->getSMVMap(), tradingdate_));

#if USE_UDP_DIRECT_SHM1_MERGE
  HFSAT::MDSMessages::CombinedMDSMessagesDirectProcessor *combined_mds_messages_shm_processor =
      common_intializer->getShmProcessor();
#else
  HFSAT::MDSMessages::CombinedMDSMessagesMultiShmProcessor *combined_mds_messages_shm_processor =
      common_intializer->getShmProcessor();
#endif
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  std::ofstream channels_file;
  std::ostringstream cfn_oss_nse;
  std::ostringstream cfn_oss_bse;


  if("NSE" == exchange_ || "BSE_NSE" == exchange_) {

    KEY_VAL_MAP shc_to_channel_info_;
    LoadShortCodeChannelInfo(shc_to_channel_info_);
    std::set<std::string> included_channels_;
    LoadChannelToSubscribe();
    bool is_fo_present_ = false;
    for (auto shc : source_shortcode_list_) {
      std::string channel = "";
      if (shc_to_channel_info_.find(shc) != shc_to_channel_info_.end()) {
        channel = shc_to_channel_info_[shc];
        included_channels_.insert(channel);
        dbglogger_ << "READ CHANNEL INFO FROM FILE : " << shc << " : " << channel << DBGLOG_ENDL_FLUSH;
      } else if (shc.find("NSE_BANKNIFTY_") != std::string::npos || shc.find("NSE_NIFTY_") != std::string::npos || shc.find("NSE_FINNIFTY_") != std::string::npos || shc.find("NSE_MIDCPNIFTY_") != std::string::npos) {
        is_fo_present_ = true;
        std::string shorthand_sc = "BANKNIFTY";
        if (shc.find("NSE_NIFTY_") != std::string::npos) shorthand_sc = "NIFTY";
        if (shc.find("NSE_FINNIFTY_") != std::string::npos) shorthand_sc = "FINNIFTY";
        if (shc.find("NSE_MIDCPNIFTY_") != std::string::npos) shorthand_sc = "MIDCPNIFTY";

        if (HFSAT::SecurityDefinitions::IsOption(shc)) {
          if ( index_opt_channels_.find(shorthand_sc) == index_opt_channels_.end()){
            std::cout << "ERROR NOT OPTION CHANNEL FOUND FOR "<< shc << " SHORTHAND: " << shorthand_sc << std::endl;
            exit(-1);
          }
          for (unsigned int index = 0; index < index_opt_channels_[shorthand_sc].size(); index++){
              channel = index_opt_channels_[shorthand_sc][index];
              included_channels_.insert(channel);
          }
        } else {
          channel = index_fut_channels_[shorthand_sc];
          included_channels_.insert(channel);
        }
      } else if (HFSAT::SecurityDefinitions::IsSpotIndex(shc) == true) {
        std::cout << "SPOT CHANNEL ADDED " << index_spot_channels_["ALL"] << std::endl;
        channel = index_spot_channels_["ALL"];
        included_channels_.insert(channel);
      } else if (HFSAT::SecurityDefinitions::IsSpotIndex(shc) == true) {
        std::cout << "SPOT CHANNEL ADDED " << index_spot_channels_["ALL"] << std::endl;
        channel = index_spot_channels_["ALL"];
        included_channels_.insert(channel);
      } else if (shc.find("_FUT") != std::string::npos ||
                      HFSAT::SecurityDefinitions::IsOption(shc)) {
        is_fo_present_ = true;
        char initial = shc.at(4);
        channel = fut_stocks_channels_[initial];
        included_channels_.insert(channel);
      } else {
        char initial = shc.at(4);
        channel = cash_channels_[initial];
        included_channels_.insert(channel);
      }
    }

    if(is_fo_present_) {
      included_channels_.insert("12 239.55.55.21 55021");
    }

    std::cout << "TOTAL CHANNELS : " << included_channels_.size() << std::endl;

    cfn_oss_nse << "/spare/local/files/NSE/nse-tbt-mcast-" << progid_ << ".txt";

    channels_file.open(cfn_oss_nse.str().c_str(), std::ofstream::out);

    if (!channels_file.is_open()) {
      std::cerr << "UNABLE TO OPEN STREAMS FILE FOR CREATING CHANNELS INFO FOR NSE : " << cfn_oss_nse.str() << std::endl;
      std::exit(-1);
    }

    for (auto &stream : included_channels_) {
      channels_file << stream << std::endl;
      std::cout << "STREAM : " << stream << std::endl;
    }

    channels_file.close();
  }
  if("BSE" == exchange_ || "BSE_NSE" == exchange_){

       KEY_VAL_MAP shc_to_channel_info_;
    LoadShortCodeChannelInfo(shc_to_channel_info_);
    std::set<std::string> included_channels_;

    LoadChannelToSubscribe();
    //std::cout << "CHANNELS SELECTION " << std::endl;
    for (auto shc : source_shortcode_list_) {

      //std::cout << "SELECTING CHANNEL FOR SHC: " << shc << std::endl;
      std::string channel = "";
      if (shc_to_channel_info_.find(shc) != shc_to_channel_info_.end()) {
        channel = shc_to_channel_info_[shc];
        included_channels_.insert(channel);
        dbglogger_ << "READ CHANNEL INFO FROM FILE : " << shc << " : " << channel << DBGLOG_ENDL_FLUSH;
      } else if (shc.find("BSE_BSX") != std::string::npos || shc.find("BSE_BKX") != std::string::npos ) {
        //std::cout << "SUBSCRIBING FO FOR SHORTCODE " << shc << std::endl;
        std::string shorthand_sc = "BSE_BKX";
        auto range = bse_fo_channels_.equal_range(shorthand_sc);
        for (auto iter = range.first; iter != range.second; ++iter) {
          channel = iter->second;
          included_channels_.insert(channel);
        }
      } 
      //else if (HFSAT::SecurityDefinitions::IsSpotIndex(shc) == true) {
      else if (shc == "BSE_SENSEX" || shc == "BSE_BANKEX") {
        //std::cout << "BSE SPOT CHANNEL ADDED " << bse_index_spot_channels_["ALL"] << std::endl;
        //std::cout << "SUBSCRIBING SPOT CHANNELS FOR SHORTCODE " << shc << std::endl;
        channel = bse_index_spot_channels_["ALL"];
        included_channels_.insert(channel);
      } else if(HFSAT::SecurityDefinitions::IsEquity(shc)){
        //std::cout << "SUBSCRIBING EQ CHANNELS FOR SHORTCODE " << shc << std::endl;
        std::string shorthand_sc = "X";
        auto range = bse_eq_channels_.equal_range(shorthand_sc);
        for (auto iter = range.first; iter != range.second; ++iter) {
          channel = iter->second;
          //std::cout << "INSERTING CHANNEL ENTRY FOR EQ " << channel << std::endl;
          included_channels_.insert(channel);
        }
      }
    }

    std::cout << "TOTAL CHANNELS : " << included_channels_.size() << std::endl;

    cfn_oss_bse << "/spare/local/files/BSE/bse-tbt-mcast-" << progid_ << ".txt";

    channels_file.open(cfn_oss_bse.str().c_str(), std::ofstream::out);

    if (!channels_file.is_open()) {
      std::cerr << "UNABLE TO OPEN STREAMS FILE FOR CREATING CHANNELS INFO FOR BSE : " << cfn_oss_bse.str() << std::endl;
      std::exit(-1);
    }
    for (auto &stream : included_channels_) {
      channels_file << stream << std::endl;
      std::cout << "STREAM : " << stream << std::endl;
    }

    channels_file.close();
  
  }

  HFSAT::Utils::UDPDirectMuxer *udp_direct_muxer_ptr = nullptr;
  HFSAT::Utils::UDPDirectMultipleZocket *udp_direct_multiple_zockets_ptr = nullptr;
  //UDP muxer only required when pulling data from multiple exchange over different interfaces
  if(true == are_we_using_hybrid_exchange){

    udp_direct_muxer_ptr = &(HFSAT::Utils::UDPDirectMuxer::GetUniqueInstance());
    udp_direct_muxer_ptr->AddEventTimeoutNotifyListener(combined_mds_messages_shm_processor);
  }else{

    udp_direct_multiple_zockets_ptr = &(HFSAT::Utils::UDPDirectMultipleZocket::GetUniqueInstance());
    udp_direct_multiple_zockets_ptr->AddEventTimeoutNotifyListener(combined_mds_messages_shm_processor);

  }

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher;
  if("NSE" == exchange_ || "BSE_NSE" == exchange_){

#if USE_EXANIC_MERGE

    HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, nullptr, HFSAT::kRaw, exchange_,
                                                        true, combined_mds_messages_shm_processor, cfn_oss.str(), true);
#else
    HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, nullptr, HFSAT::kRaw, "NSE",
                                                      true, combined_mds_messages_shm_processor, cfn_oss_nse.str());
#endif
//        HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvUDPDirectDataInfo();
//        udp_direct_multiple_zockets.CreateSocketAndAddToMuxer(control_recv_data_info_.bcast_ip_,
//        control_recv_data_info_.bcast_port_, combined_mds_messages_shm_processor, 'X', false);

    HFSAT::NSEMD::NSETBTDataProcessor &nse_tbt_data_processor = HFSAT::NSEMD::NSETBTDataProcessor::GetUniqueInstance(
      dbglogger_, HFSAT::kRaw, nullptr, combined_mds_messages_shm_processor);
    for (auto shc : source_shortcode_list_) {
      nse_tbt_data_processor.AddShortCodeForProcessing(shc);
    }

  }

  if( "BSE" == exchange_ || "BSE_NSE" == exchange_){
    HFSAT::BSEMD::BSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, nullptr, HFSAT::kRaw, exchange_,
                                                        true, combined_mds_messages_shm_processor, true,cfn_oss_bse.str());  //no recover = true
    HFSAT::BSEMD::BSETBTDataProcessor &bse_tbt_data_processor = HFSAT::BSEMD::BSETBTDataProcessor::GetUniqueInstance(
        dbglogger_, HFSAT::kRaw, nullptr, combined_mds_messages_shm_processor);

    for (auto shc : source_shortcode_list_) {
      bse_tbt_data_processor.AddShortCodeForProcessing(shc);
    }
  }

  std::map<std::string, MarketOrderExecution *> shc_exec_map;
  std::vector<HFSAT::BasicOrderManager *> basic_order_manager_vec_;

  std::map<std::string, std::string> shc_price_map;
  Parser::ParseConfig(_position_file, shc_price_map, 2);

  for (auto shc_iter_ : shc_vec_) {
    int sec_id_ = sec_name_indexer_.GetIdFromString(shc_iter_);
    int position_ = std::stoi(shc_pos_map[shc_iter_]);
    auto dep_smv_ = p_smv_vec_[sec_id_];
    assert(dep_smv_ != nullptr);

    HFSAT::BaseTrader *base_trader_; //  = GetNSETrader(shc_iter_, sec_id_, network_account_info_manager_, watch_);
    dbglogger_ << "SHC: " << dep_smv_->shortcode() << " SYM: " << dep_smv_->secname() << " ";
    if (exchange_ == "NSE") {
      base_trader_ = GetNSETrader(shc_iter_, sec_id_, network_account_info_manager_, watch_);
    }
    else if (exchange_ == "BSE") {
      base_trader_ = GetBSETrader(shc_iter_, sec_id_, network_account_info_manager_, watch_);
    } else {
	base_trader_ = GetNSETrader(shc_iter_, sec_id_, network_account_info_manager_, watch_);
    }

    std::map<std::string, std::string> pos_limits_map_;
    std::string key_prefix_ = dep_smv_->shortcode();
    pos_limits_map_[key_prefix_ + "_MAXLONGPOS"] = std::to_string(position_);
    pos_limits_map_[key_prefix_ + "_MAXSHORTPOS"] = std::to_string(-1 * position_);
    pos_limits_map_[key_prefix_ + "_MAXLONGEXPOSURE"] = std::to_string(position_ + 1);
    pos_limits_map_[key_prefix_ + "_MAXSHORTEXPOSURE"] = std::to_string(-1 * position_ + 1);

    double price_ = -1;
    if (shc_price_map.find(shc_iter_) != shc_price_map.end()) {
      price_ = std::stod(shc_price_map[shc_iter_]);
    }

    auto basic_om_ = new HFSAT::BasicOrderManager(dbglogger_, watch_, sec_name_indexer_, *base_trader_, *dep_smv_, 1,
                                                  pos_limits_map_, true, false, false);
    HFSAT::TradeType_t buysell_ =
        (position_ > 0) ? HFSAT::TradeType_t::kTradeTypeBuy : HFSAT::TradeType_t::kTradeTypeSell;
    int initial_pos_to_exec_ = position_;
    MarketOrderExecution *mkt_exec_ = new MarketOrderExecution(
        watch_, dbglogger_, dep_smv_, basic_om_, trading_start_utc_mfm_, std::abs(initial_pos_to_exec_), buysell_,
        price_, trigger_utc_mfm_, midnight_mfm_, false);
    mkt_exec_vec_.push_back(mkt_exec_);
    basic_om_->AddExecutionListener(mkt_exec_);
    int exec_id = basic_om_->AddOrderChangeListener(mkt_exec_);
    basic_om_->AddOrderRejectListener(mkt_exec_, exec_id);
    mkt_exec_->SetExecId(exec_id);

    shc_exec_map[shc_iter_] = mkt_exec_;

    HFSAT::BasePNL *base_pnl = nullptr;
    SubscribeToORSReply(combined_mds_messages_shm_processor, basic_om_, sec_id_);
    base_pnl =
        new HFSAT::LiveBasePNL(dbglogger_, watch_, *dep_smv_, runtime_id_, client_logging_segment_initializer_ptr);

    basic_om_->SetBasePNL(base_pnl);
    mkt_exec_->SetBasePNL(base_pnl);
    basic_order_manager_vec_.push_back(basic_om_);
    runtime_id_++;
  }
  mkt_exec_vec_[0]->setPolling(true);
  mkt_exec_vec_[0]->assignExecVec(&mkt_exec_vec_);
  UserControlManager *user_control_manager_ =
      new UserControlManager(dbglogger_, watch_, basic_order_manager_vec_, mkt_exec_vec_);
  combined_mds_messages_shm_processor->AddControlSourceForProcessing(HFSAT::MDS_MSG::CONTROL, progid_,
                                                                     user_control_manager_, &watch_);
  // Do not change logging format (risk monitor uses this line)
  dbglogger_ << "DUMPEDALLSACIS (logging for monitoring)\n";
  dbglogger_.DumpCurrentBuffer();
  market_update_manager_.start();

  HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("post_market_exec");

#if USE_UDP_DIRECT_SHM1_MERGE
  if(true == are_we_using_hybrid_exchange){
    udp_direct_muxer_ptr->RunMuxerLiveDispatcherWithTimeOut(100);
  }else{
    udp_direct_multiple_zockets_ptr->RunLiveDispatcherWithTimeOut(2000);
  }
#else
  combined_mds_messages_shm_processor->RunLiveShmSource();
#endif

  return 0;
}
