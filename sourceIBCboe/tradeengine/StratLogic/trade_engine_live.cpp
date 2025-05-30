/**
   \file StratLogic/trade_engine_live.cpp

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
#include "baseinfra/LivePnls/live_base_pnl.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/Utils/signals.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/udp_direct_muxer.hpp"
#include "tradeengine/Utils/Parser.hpp"
#include "tradeengine/CommonInitializer/common_initializer.hpp"
#include "tradeengine/StratLogic/portfolio_risk_manager.hpp"
#include "tradeengine/StratLogic/user_control_manager.hpp"
#include "tradeengine/TheoCalc/HedgeTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MRTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MACDTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MasterTheoCalculator.hpp"
#include "tradeengine/TheoCalc/CorrTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MispriceTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MomentumTheoCalculator.hpp"
#include "tradeengine/TheoCalc/HighMoveTheoCalculator.hpp"
#include "tradeengine/TheoCalc/GapTheoCalculator.hpp"
#include "tradeengine/TheoCalc/GapRevertTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MATheoCalculator.hpp"
#include "tradeengine/TheoCalc/RatioTheoCalculator.hpp"
#include "tradeengine/TheoCalc/VWAPTheoCalculator.hpp"

#if USE_UDP_DIRECT_SHM1_MERGE
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/Utils/exanic_rx_buffer.hpp"
#include "infracore/NSEMD/nse_tbt_data_processor.hpp"
#include "infracore/NSEMD/nse_tbt_raw_md_handler.hpp"
#include "infracore/BSEMD/bse_tbt_data_processor.hpp"
#include "infracore/BSEMD/bse_tbt_raw_md_handler.hpp"
#include "infracore/IBKRMD/ibkr_l1_md_handler.hpp"
#else
#include "baseinfra/MDSMessages/combined_mds_messages_multi_shm_processor.hpp"
#endif

// For Offloaded Logging
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#define CHANNEL_INFO_FILE "/spare/local/files/NSE/channel_info.txt"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

#define CHANNEL_INFO_FILE "/spare/local/files/NSE/channel_info.txt"
#define CHANNEL_DETAILS_FILE "/home/pengine/prod/live_configs/channel_to_subscribe_strat.txt"
#define OI_CHANNEL_DETAILS_FILE "/home/pengine/prod/live_configs/channel_to_subscribe_strat_for_oi.txt"

HFSAT::OnlineDebugLogger *online_dbglogger_ = new HFSAT::OnlineDebugLogger(1024 * 4 * 1024, 1109, 256 * 1024);
HFSAT::DebugLogger &dbglogger_ = *(online_dbglogger_);  //( 4*1024*1024, 256*1024 ); // making logging more efficient,
// otherwise change it backto 10240, 1

HFSAT::Utils::ClientLoggingSegmentInitializer *client_logging_segment_initializer_ptr = NULL;
HFSAT::ShortcodeRequestHelper *global_shc_request_helper;
std::string eod_position_file_;

typedef std::vector<std::map<std::string, std::string> *> VEC_KEY_VAL_MAP;
typedef std::map<std::string, std::string> KEY_VAL_MAP;

std::map<char, std::string> fut_stocks_channels_;
std::map<char, std::string> fut_oi_stocks_channels_;
std::map<char, std::string> cash_channels_;
std::map<std::string, std::string> index_fut_channels_;
std::map<std::string, std::string> index_oi_fut_channels_;
std::map<std::string,vector<std::string>> index_opt_channels_;
std::map<std::string,vector<std::string>> index_oi_opt_channels_;
std::map<std::string, std::string> index_spot_channels_;
std::map<std::string, std::string> bse_index_spot_channels_;
std::map<std::string, std::string> bse_oi_channels_;
std::multimap<std::string, std::string> bse_eq_channels_;
std::multimap<std::string, std::string> bse_fo_channels_;

std::vector<BaseTheoCalculator *> theo_vec_;
int progid_;
int tradingdate_;
int end_strat_timestamp;
std::string exchange_global_ = "NSE";

void termination_handler(int signum) {
  std::cout << "Exiting due to signal " << SimpleSignalString(signum) << std::endl;
  char hostname_[128];
  hostname_[127] = '\0';
  gethostname(hostname_, 127);

  std::string position_email_string_ = "";
  std::ostringstream t_oss_;

  t_oss_ << exchange_global_ << " Strategy: " << progid_ << " exiting signal: " << SimpleSignalString(signum) << " on " << hostname_
         << "\n";
  std::string subject_email_string_ = t_oss_.str();
  position_email_string_ = position_email_string_ + t_oss_.str();

  int sum_pnl_ = 0;
  double sum_net_exp_ = 0;
  double sum_gross_exp_ = 0;
  double sum_ttv_ = 0;
  int num_unhedged_theo_ = 0;
  int total_orders_ = 0;
  for (auto theo_calc_ : theo_vec_) {
    if (theo_calc_->GetPosition() != 0) {
      std::ostringstream t_oss_1;
      t_oss_1 << "<br/>" << theo_calc_->GetSecondaryShc() << "  "
        << theo_calc_->GetSecondarySecname() << "  " << theo_calc_->GetPosition() ;
      position_email_string_ = position_email_string_ + t_oss_1.str();
    }
    theo_calc_->PNLStats();
    sum_pnl_ += theo_calc_->GetPNL();
    sum_net_exp_ += theo_calc_->GetExposure();
    sum_gross_exp_ += std::abs(theo_calc_->GetExposure());
    sum_ttv_ += theo_calc_->GetTTV();
    total_orders_ += theo_calc_->GetTotalOrderCount();
    if ((theo_calc_->IsNeedToHedge() == false) && (theo_calc_->GetTotalPosition() != 0)) {
      num_unhedged_theo_++;
    }
  }
  dbglogger_ << "PORTFOLIO PNL: " << sum_pnl_ << " NETEXP: " << sum_net_exp_ << " GROSSEXP: " << sum_gross_exp_
             << " TTV: " << sum_ttv_ << " NUMOPEN: " << num_unhedged_theo_ << " NUM_ORDERS: " << total_orders_ << "\n"
             << DBGLOG_ENDL_FLUSH;
  dbglogger_.DumpCurrentBuffer();
  struct timeval disconnection_time;
  gettimeofday(&disconnection_time,NULL);
  if (position_email_string_ != subject_email_string_ || disconnection_time.tv_sec < end_strat_timestamp) {
      std::cout << "Sending Mail " << std::endl;
      HFSAT::Email email_;
      email_.setSubject(subject_email_string_);
      email_.addRecepient("nsehft@tworoads.co.in");
      email_.addSender("uttkarsh.sarraf@tworoads.co.in");
      email_.content_stream << position_email_string_ << "<br/>";
      email_.sendMail();
  } 
  //  This is causing a delay of around 2 mins if the route is not available or incorrect route is mentioned in
  //  the file used in this function.
  //    HFSAT::SendAlert::sendAlert(position_email_string_);
  //  }

  //  if (global_shc_request_helper) {
  //    global_shc_request_helper->RemoveAllShortcodesToListen();
  //    global_shc_request_helper = nullptr;
  //  }
  //
  dbglogger_.Close();
  //  HFSAT::RuntimeProfiler::ResetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT);
  //
  //  if (NULL != client_logging_segment_initializer_ptr) {
  //    client_logging_segment_initializer_ptr->CleanUp();
  //  }
  //
  //  std::ofstream out(eod_position_file_);
  //  out << position_email_string_;
  //  out.close();
  //
  if (SIGINT != signum) {  // Unexpected
                           //
                           //    std::ostringstream t_temp_oss;
    //    t_temp_oss << "RECEIVED SIGSEGV, CAN'T CONTINUE, CHECK FOR LOGS, GENERATING CORE FILE AS..... : core." <<
    //    getpid();
    //
    //    // also send an alert
    //    char hostname[128];
    //    hostname[127] = '\0';
    //    gethostname(hostname, 127);
    //    HFSAT::Email e;
    //
    //    e.setSubject("FATAL : TRADEINIT RECEIVED SIGSEGV - IF THIS IS EOD THAN FINE");
    //    e.addRecepient("DVCTech@circulumvite.com");
    //
    //    e.addSender("DVCTech@circulumvite.com");
    //    e.content_stream << "host_machine: " << hostname << "<br/>";
    //    e.content_stream << t_temp_oss.str() << "<br/>";
    //    e.sendMail();
    //    HFSAT::SendAlert::sendAlert(t_temp_oss.str());
    //
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
    //
  } else {
    exit(0);
  }
}

void InitDbglogger(int tradingdate, int progid, std::vector<std::string> &dbg_code_vec,
                   const std::string &logs_directory, bool livetrading_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << logs_directory << "/log." << tradingdate << "." << progid;
  std::string logfilename_ = t_temp_oss.str();

  dbglogger_.OpenLogFile(logfilename_.c_str(), (livetrading_ ? std::ios::app : std::ios::out));

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

  //  if (dbg_code_vec.size() <= 0) {
  //    dbglogger.SetNoLogs();
  //  }

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
    } else if (strcmp(tokens_[0], "BSESPT") == 0 ){
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

void LoadOIChannelToSubscribe(){
  std::ifstream shc_to_subscribe_info_file_oi;
  shc_to_subscribe_info_file_oi.open(OI_CHANNEL_DETAILS_FILE, std::ifstream::in);
  if (!shc_to_subscribe_info_file_oi.is_open()) {
    std::cerr << "UNABLE TO OPEN CHANNEL To Subscribe FILE FOR READING :" << OI_CHANNEL_DETAILS_FILE << std::endl;
    exit (-1);
  }
  char line_[1024];
  while (!shc_to_subscribe_info_file_oi.eof()) {
    memset(line_, 0, sizeof(line_));
    shc_to_subscribe_info_file_oi.getline(line_, sizeof(line_));
    HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    const std::vector<const char *> &tokens_ = t_tokenizer_.GetTokens();
    if (tokens_.size() < 5 || tokens_[0][0] == '#') {
      std::cerr << "CHANNEL INFO : IGNORING LINE : " << line_ << std::endl;
      continue;
    }
    char channel_start = tokens_[1][0];
    if (strcmp(tokens_[0],"FUT") == 0){
      cout<<"OI FUT " << channel_start << " CHAN " << tokens_[3] << std::endl;
      fut_oi_stocks_channels_[channel_start] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "FUTIDX") == 0){
      cout<<"OI IND_FUT " << tokens_[1] << " CHAN "<< tokens_[3] << std::endl;
      index_oi_fut_channels_[tokens_[1]] = std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]);
    } else if (strcmp(tokens_[0], "OPTIDX") == 0 ){
      cout<<"OI IND_OP " << tokens_[1] << " CHAN " << tokens_[3] << std::endl;
      index_oi_opt_channels_[tokens_[1]].push_back(std::string(tokens_[2]) + " " + std::string(tokens_[3]) + " " + std::string(tokens_[4]));
    } 
  }
}

bool CollectTheoSourceShortCode(KEY_VAL_MAP *key_val_map_, std::vector<std::string> &source_shortcode_list) {

  uint32_t count = 0;
  std::string primary_symbol = std::string("PRIMARY") + std::to_string(count);
  std::vector<std::string> theo_shortcode_list_;
  while (key_val_map_->find(primary_symbol) != key_val_map_->end()) {
    std::string shc_ = (*key_val_map_)[primary_symbol];

    if (HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shc_) ||
        HFSAT::SecurityDefinitions::IsShortcode(shc_)){
      theo_shortcode_list_.push_back(shc_);
      count++;
      primary_symbol = std::string("PRIMARY") + std::to_string(count);
    } else {
      dbglogger_ << "WRONG SHORTCODE: " << shc_ << DBGLOG_ENDL_FLUSH;
      dbglogger_.DumpCurrentBuffer();
      return false;
    }
  }
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_list, theo_shortcode_list_);
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_list, (*key_val_map_)["SECONDARY"]);
  return true;
}

void CollectTheoORSShortCode(KEY_VAL_MAP *key_val_map_, std::vector<std::string> &ors_shortcode_list) {
  HFSAT::VectorUtils::UniqueVectorAdd(ors_shortcode_list, (*key_val_map_)["SECONDARY"]);
  // Will need to add Hedge symbols at a later stage
}

bool CheckIfTokenExistsBSE(std::string shortcode_){

  HFSAT::Utils::BSEDailyTokenSymbolHandler &bse_daily_token_symbol_handler =
      HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);

  HFSAT::SecurityDefinitions &bse_sec_def = HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);
  char segment = bse_sec_def.GetSegmentTypeFromShortCode(shortcode_);
  std::cout <<"Segment " << segment << std::endl;

  if (BSE_INVALID_SEGMENT == segment) {
    dbglogger_ << "SHORTCODE : " << shortcode_ << " RETURNS INVALID BSE SEGMENT \n";
    dbglogger_.DumpCurrentBuffer();
    return false;
  }

  std::string const internal_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
  std::string const data_symbol_name = bse_sec_def.ConvertExchSymboltoDataSourceName(internal_symbol, shortcode_);

  if (std::string("INVALID") == data_symbol_name) {
    dbglogger_ << "COULDN'T FIND DATASYMBOL FROM INTERNAL SYMBOL : " << internal_symbol
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE, SKIPPING THIS SHORTCODE..." << shortcode_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return false;
  }

  if (-1 == bse_daily_token_symbol_handler.GetTokenFromInternalSymbol(data_symbol_name.c_str(), segment)) {
    dbglogger_ << "COULDN'T FIND TOKEN FOR INTERNAL SYMBOL : " << data_symbol_name << " WITH SEC ID : "
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE, SKIPPING THIS SHORTCODE..." << shortcode_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return false;
  }

  // Get RefData Info From RefLoader
  HFSAT::Utils::BSERefDataLoader &bse_ref_data_loader = HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(tradingdate_);
  std::map<int32_t, BSE_UDP_MDS::BSERefData> &bse_ref_data = bse_ref_data_loader.GetBSERefData(segment);

  if (0 == bse_ref_data.size()) {
    dbglogger_ << "FOR SEGMENT : " << segment << " REF DATA DOESN't EXISTS" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return false;
  }

  return true;
}

bool CheckIfTokenExistsNSE(std::string shortcode_){

  HFSAT::Utils::NSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);

  HFSAT::SecurityDefinitions &nse_sec_def = HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_);
  char segment = nse_sec_def.GetSegmentTypeFromShortCode(shortcode_);
  std::cout <<"Segment " << segment << std::endl;

  if (NSE_INVALID_SEGMENT == segment) {
    dbglogger_ << "SHORTCODE : " << shortcode_ << " RETURNS INVALID NSE SEGMENT \n";
    dbglogger_.DumpCurrentBuffer();
    return false;
  }

  std::string const internal_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
  std::string const data_symbol_name = nse_sec_def.ConvertExchSymboltoDataSourceName(internal_symbol, shortcode_);

  if (std::string("INVALID") == data_symbol_name) {
    dbglogger_ << "COULDN'T FIND DATASYMBOL FROM INTERNAL SYMBOL : " << internal_symbol
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE, SKIPPING THIS SHORTCODE..." << shortcode_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return false;
  }

  if (-1 == nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(data_symbol_name.c_str(), segment)) {
    dbglogger_ << "COULDN'T FIND TOKEN FOR INTERNAL SYMBOL : " << data_symbol_name << " WITH SEC ID : "
               << " WON'T BE ABLE TO DECIDE WHICH ORS TO CONNECT TO HENCE, SKIPPING THIS SHORTCODE..." << shortcode_ << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return false;
  }

  // Get RefData Info From RefLoader
  HFSAT::Utils::NSERefDataLoader &nse_ref_data_loader = HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(tradingdate_);
  std::map<int32_t, NSE_UDP_MDS::NSERefData> &nse_ref_data = nse_ref_data_loader.GetNSERefData(segment);

  if (0 == nse_ref_data.size()) {
    dbglogger_ << "FOR SEGMENT : " << segment << " REF DATA DOESN't EXISTS" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return false;
  }

  return true;

}

bool CheckIfTokenExists(std::string shortcode_){
  return ("NSE" == HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).GetExchangeStringFromShortcode(shortcode_)) ? CheckIfTokenExistsNSE(shortcode_) : CheckIfTokenExistsBSE(shortcode_);
}

void CreateShortcodeLists(KEY_VAL_MAP &live_file_key_val_map_, std::vector<std::string> &source_shortcode_list_,
                          std::vector<std::string> &ors_shortcode_list_, VEC_KEY_VAL_MAP &vec_key_val_map_,
                          std::string &live_folder_name) {
  uint64_t i = 1;
  std::string theo_number = std::string("THEO") + std::to_string(i);
  while (live_file_key_val_map_.find(theo_number) != live_file_key_val_map_.end()) {
    std::string theo_folder_name = live_folder_name + std::string("/") + live_file_key_val_map_[theo_number] + "/";
    std::string theo_cal = theo_folder_name + std::string("MainConfig.cfg");
    // std::cout << " Trying THEOCAL" << i << " " << theo_cal << std::endl;
    std::map<std::string, std::string> *key_val_map = new std::map<std::string, std::string>();
    Parser::ParseConfig(theo_cal, *key_val_map);
    (*key_val_map)["THEO_FOLDER"] = theo_folder_name;

    std::cout << " Shortcode : " << Parser::GetString(key_val_map, "SECONDARY", "DEFAULT") << " Check If Token Exists : " << (CheckIfTokenExists(Parser::GetString(key_val_map, "SECONDARY", "DEFAULT"))) << std::endl; 

    if ((Parser::GetBool(key_val_map, "STATUS", false)) &&
        (HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(
            Parser::GetString(key_val_map, "SECONDARY", "DEFAULT")))) {
      bool is_valid_shc_config_ = CollectTheoSourceShortCode(key_val_map, source_shortcode_list_);
      if (is_valid_shc_config_) {
        if (key_val_map->find("THEO_IDENTIFIER") == key_val_map->end()) {
          std::cerr << "THEO IDENTIFIER missing for a theo!! (exiting)" << std::endl;
          exit(-1);
        }
        std::string theo_identifier_ = (*key_val_map)["THEO_IDENTIFIER"];
        for (unsigned int i = 0; i < vec_key_val_map_.size(); i++) {
          if (theo_identifier_ == (*vec_key_val_map_[i])["THEO_IDENTIFIER"]) {
            std::cerr << "SAME THEO IDENTIFIER for multiple theos " << theo_identifier_ << " !! (exiting)" << std::endl;
            exit(-1);
          }
        }
        vec_key_val_map_.push_back(key_val_map);
        CollectTheoORSShortCode(key_val_map, ors_shortcode_list_);
      }
    } else if ("MASTER_THEO" == (*key_val_map)["THEO_TYPE"]) {
      vec_key_val_map_.push_back(key_val_map);
    }

    i++;
    theo_number = std::string("THEO") + std::to_string(i);
  }
}

void CreateTheoCalculators(VEC_KEY_VAL_MAP &vec_key_val_map_, std::vector<BaseTheoCalculator *> &theo_vec_,
                           std::vector<BaseTheoCalculator *> &sqoff_theo_vec_,
                           std::map<std::string, BaseTheoCalculator *> &theo_map_,
                           std::map<int, std::deque<SquareOffTheoCalculator *> > &sqoff_theo_map_, HFSAT::Watch &watch_,
                           HFSAT::DebugLogger &dbglogger_, int _trading_start_utc_mfm_, int start_window_msec,
                           int _trading_end_utc_mfm_, int end_window_msec, int aggressive_get_flat_mfm_,
                           int eff_squareoff_start_utc_mfm_, double bid_multiplier_, double ask_multiplier_) {
  int num_theos = vec_key_val_map_.size();
  int start_theo_diff = (num_theos > 0) ? start_window_msec / num_theos : 0;
  int end_theo_diff = (num_theos > 0) ? end_window_msec / num_theos : 0;
  int count = 0;
  for (auto key_val_map : vec_key_val_map_) {
    if (!Parser::GetBool(key_val_map, "STATUS", false)) return;
    if (key_val_map->find("THEO_TYPE") != key_val_map->end()) {
      std::string theo_type = (*key_val_map)["THEO_TYPE"];
      // if (key_val_map->find("THEO_IDENTIFIER") == key_val_map->end()) {
      // 	std::cerr << "THEO IDENTIFIER missing for a theo!! (exiting)" << std::endl;
      // 	exit(-1);
      // }
      if (theo_type == "RATIO_THEO") {
        // std::cout << "Creating RatioTheo starttime " << _trading_start_utc_mfm_+ (count*start_theo_diff) << " endtime
        // " << _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new RatioTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_ + (count * start_theo_diff),
            _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_,
            bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "MISPRICE_THEO") {
        // std::cout << "Creating MispriceTheo starttime " << _trading_start_utc_mfm_<< " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new MispriceTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_ + (count * start_theo_diff),
            _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_,
            bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "SQUAREOFF_THEO") {
        // std::cout << "Creating SquareoffTheo starttime " << _trading_start_utc_mfm_<< " endtime " <<
        // _trading_end_utc_mfm_ + end_window_msec << std::endl;
        SquareOffTheoCalculator *theo_calc =
            new SquareOffTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
                                        _trading_end_utc_mfm_ + end_window_msec, aggressive_get_flat_mfm_);
        int sec_id = theo_calc->GetSecondaryID();
        sqoff_theo_vec_.push_back(theo_calc);
        sqoff_theo_map_[sec_id].push_back(theo_calc);
        // Create Delta Theo
      } else if (theo_type == "HEDGE_THEO") {
        // std::cout << "Creating HedgeTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new HedgeTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "VWAP_THEO") {
        // std::cout << "Creating HedgeTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new VWAPTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "MIDTERM_THEO") {
        // Create MidTerm Theo
        // std::cout << "Creating MidTermTheo starttime " << (*key_val_map)["THEO_IDENTIFIER"] << std::endl;
        // std::string theo_type = (*key_val_map)["THEO_TYPE"];
        BaseTheoCalculator* theo_calc = new MidTermTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        MidTermTheoCalculator* midterm_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(theo_calc);
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        if (key_val_map->find("MOMENTUM1") != key_val_map->end()) {
          std::string mom_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MOMENTUM1"];
          std::map<std::string, std::string>* mom_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(mom_param_file_, *mom_key_val_map_);
          BaseTheoCalculator* mom_theo_calc = new MomentumTheoCalculator(
            mom_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(mom_theo_calc);
          MidTermTheoCalculator* mom_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(mom_theo_calc);
          mom_theo_calc_->setStratParamFile(mom_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(mom_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*mom_key_val_map_)["THEO_IDENTIFIER"]] = mom_theo_calc;
        }

        if (key_val_map->find("MACD1") != key_val_map->end()) {
          std::string macd_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MACD1"];
          std::map<std::string, std::string>* macd_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(macd_param_file_, *macd_key_val_map_);
          BaseTheoCalculator* macd_theo_calc = new MACDTheoCalculator(
            macd_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(macd_theo_calc);
          MidTermTheoCalculator* macd_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(macd_theo_calc);
          macd_theo_calc_->setStratParamFile(macd_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(macd_theo_calc));
          // macd_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*macd_key_val_map_)["THEO_IDENTIFIER"]] = macd_theo_calc;
        }

        if (key_val_map->find("MR1") != key_val_map->end()) {
          std::string mr_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MR1"];
          std::map<std::string, std::string>* mr_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(mr_param_file_, *mr_key_val_map_);
          BaseTheoCalculator* mr_theo_calc = new MRTheoCalculator(
            mr_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(mr_theo_calc);
          MidTermTheoCalculator* mr_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(mr_theo_calc);
          mr_theo_calc_->setStratParamFile(mr_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(mr_theo_calc));
          // mr_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*mr_key_val_map_)["THEO_IDENTIFIER"]] = mr_theo_calc;
        }

        if (key_val_map->find("GAP1") != key_val_map->end()) {
          std::string gap_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["GAP1"];
          std::map<std::string, std::string>* gap_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(gap_param_file_, *gap_key_val_map_);
          BaseTheoCalculator* gap_theo_calc = new GapTheoCalculator(
            gap_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(gap_theo_calc);
          MidTermTheoCalculator* gap_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(gap_theo_calc);
          gap_theo_calc_->setStratParamFile(gap_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(gap_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*gap_key_val_map_)["THEO_IDENTIFIER"]] = gap_theo_calc;
        }
        if (key_val_map->find("GAPREVERT1") != key_val_map->end()) {
          std::string gaprevert_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["GAPREVERT1"];
          std::map<std::string, std::string>* gaprevert_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(gaprevert_param_file_, *gaprevert_key_val_map_);
          BaseTheoCalculator* gaprevert_theo_calc = new GapRevertTheoCalculator(
            gaprevert_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(gaprevert_theo_calc);
          MidTermTheoCalculator* gaprevert_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(gaprevert_theo_calc);
          gaprevert_theo_calc_->setStratParamFile(gaprevert_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(gaprevert_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*gaprevert_key_val_map_)["THEO_IDENTIFIER"]] = gaprevert_theo_calc;
        }

        if (key_val_map->find("MA1") != key_val_map->end()) {
          std::string ma_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MA1"];
          std::map<std::string, std::string>* ma_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(ma_param_file_, *ma_key_val_map_);
          BaseTheoCalculator* ma_theo_calc = new MATheoCalculator(
            ma_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(ma_theo_calc);
          MidTermTheoCalculator* ma_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(ma_theo_calc);
          ma_theo_calc_->setStratParamFile(ma_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(ma_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*ma_key_val_map_)["THEO_IDENTIFIER"]] = ma_theo_calc;
        }
        if (key_val_map->find("HMT1") != key_val_map->end()) {
          std::string hmt_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["HMT1"];
          std::map<std::string, std::string>* hmt_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(hmt_param_file_, *hmt_key_val_map_);
          BaseTheoCalculator* hmt_theo_calc = new HighMoveTheoCalculator(
            hmt_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(hmt_theo_calc);
          MidTermTheoCalculator* hmt_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(hmt_theo_calc);
          hmt_theo_calc_->setStratParamFile(hmt_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(hmt_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*hmt_key_val_map_)["THEO_IDENTIFIER"]] = hmt_theo_calc;
        }

      } else if (theo_type == "MOMENTUM_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new MomentumTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MACD_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new MACDTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MR_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new MRTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "GAP_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new GapTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MA_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new MATheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "HMT_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new HighMoveTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MASTER_THEO") {
        // std::cout << "Creating MasterTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc =
            new MasterTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
                                     _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Master Theo
      } else if (theo_type == "CORR_THEO") {
        // std::cout << "Creating MasterTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc =
            new CorrTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
                                     _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Master Theo
      } else {
        std::cerr << __func__ << " THEO_TYPE not supported " << theo_type << std::endl;
        exit(-1);
      }
    } else {
      std::cerr << "THEO_TYPE missing in MainConfig! cant construct theo" << std::endl;
      exit(-1);
    }
    ++count;
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
  // combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderSequencedListener(security_id,
  // smart_om);
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

int main(int argc, char **argv) {
  // Assume we get a file source list and we have to print Market Updates
  // Arg1 : Strat File
  // Arg2 : Trading date
  // Arg3 : start time
  // Arg4 : end  time
  // Arg5 : Prog ID

  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGPIPE, SIG_IGN);
  if (argc < 6) {  // 5 is min of live and sim
    std::cerr << "expecting :\n"
              << " $tradeengineexec STRAT_LIVE_FILE TRADINGDATE START_TIME END_TIME PROGID ADD_DBG_CODE DBG_CODE.... --tbt_recovery=[false]/true [by default false is used as not to go for HFT/tbt data recovery] --use_oi=[false]/true --exchange=[NSE]/BSE/BSE_NSE/CBOE" << '\n';
    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  }

  std::string _live_file = argv[1];
  tradingdate_ = atoi(argv[2]);
  int start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[3] + 4), argv[3]);
  int end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[4] + 4), argv[4]);
  progid_ = atoi(argv[5]);
  std::string exchange_ = "NSE";

  bool livetrading_ = true;
  bool subsribe_to_oi_data = false;
  std::string md5sum = "";
  int test_flag = -1;
  bool tbt_recovery_allowed = false;


  std::vector<std::string> dbg_code_vec = {"FILL_TIME_INFO"};

  //For future reference - please do not change the core arguments of the program, newer arguments should always be used with optional boost args
  if(argc > 6){
    if (strcmp(argv[6], "ADD_DBG_CODE") == 0) {
      for (int i = 7; i < argc; i++) {
        if(std::string(argv[i]).find("--") == std::string::npos){
          dbg_code_vec.push_back(std::string(argv[i]));
        }
      }
    }
  }

  boost::program_options::options_description desc("Allowed Options");
  desc.add_options()("help", "produce help message.")("tbt_recovery", boost::program_options::value<std::string>()->default_value("false"))("use_oi", boost::program_options::value<std::string>()->default_value("false"))("exchange", boost::program_options::value<std::string>()->default_value("NSE"))("no_ors", boost::program_options::value<std::string>()->default_value("false"))("use_exanic", boost::program_options::value<std::string>()->default_value("false"));

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  std::string tbt_recovery_flag = vm["tbt_recovery"].as<std::string>();
  std::string use_oi_flag = vm["use_oi"].as<std::string>();
  std::string exchange_flag = vm["exchange"].as<std::string>();
  std::string no_ors_flag = vm["no_ors"].as<std::string>();
  std::string use_exanic_flag = vm["use_exanic"].as<std::string>();

  tbt_recovery_allowed = ("true" == tbt_recovery_flag) ? true : false;
  subsribe_to_oi_data = ("true" == use_oi_flag) ? true : false ;
  bool are_we_using_hybrid_exchange = ("BSE_NSE" == exchange_flag) ? true : false;
  bool are_we_using_ibkr_interface = ("CBOE" == exchange_flag) ? true : false;
  test_flag = ("true" == no_ors_flag) ? 1 : -1 ;
  bool are_we_using_exanic = ("true" == use_exanic_flag) ? true : false;

  exchange_ = exchange_flag;
  exchange_global_ = exchange_;

  std::map<std::string, std::string> _live_file_key_val_map;
  Parser::ParseConfig(_live_file, _live_file_key_val_map);

  std::string live_folder_name_ = ".";

  const size_t last_slash_idx = _live_file.rfind('/');
  if (std::string::npos != last_slash_idx) {
    live_folder_name_ = _live_file.substr(0, last_slash_idx);
  }

  //default location
  HFSAT::TradingLocation_t dep_trading_location_ = HFSAT::kTLocNSE;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  //Must be called before any further exchange specific calls
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).SetExchangeType(exchange_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadSecurityDefinitions();

  dep_trading_location_ = ("BSE" == exchange_ || "BSE_NSE" == exchange_) ? HFSAT::kTLocBSE : HFSAT::kTLocNSE;
  if("CBOE" == exchange_) dep_trading_location_ = HFSAT::kTLocCBOE;

  std::vector<std::string> source_shortcode_list_;
  std::vector<std::string> ors_shortcode_list_;
  VEC_KEY_VAL_MAP vec_key_val_map_;
  std::string logs_directory = "/spare/local/logs/tradelogs";

  //TODO - CBOE // define trading hours

  int day_end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, 1530, "IST_1530");
  int end_utc_hhmm_minus_1_ = 959;
  if (_live_file_key_val_map.find("DAY_END_IST_TIME") != _live_file_key_val_map.end()) {
    char *day_end_ist_time_ = new char[strlen(_live_file_key_val_map["DAY_END_IST_TIME"].c_str()) + 1];
    std::strcpy(day_end_ist_time_, _live_file_key_val_map["DAY_END_IST_TIME"].c_str());
    day_end_utc_hhmm_ =
        HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(day_end_ist_time_ + 4), day_end_ist_time_);
    int day_end_utc_mins_minus_1_ =
        HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(tradingdate_, day_end_utc_hhmm_, "UTC_") / 60 - 1;
    end_utc_hhmm_minus_1_ = ((day_end_utc_mins_minus_1_ % 60) + (day_end_utc_mins_minus_1_/60) % 24 * 100);
  }
  if (strcmp(exchange_.c_str(), "NSE") == 0 || strcmp(exchange_.c_str(), "BSE") == 0 || strcmp(exchange_.c_str(), "BSE_NSE") == 0) {
    if (end_utc_hhmm_ > end_utc_hhmm_minus_1_) {
      end_utc_hhmm_ = end_utc_hhmm_minus_1_;
    }
  }  
  
  dbglogger_ << "START_UTC_HHMM: " << start_utc_hhmm_ << " END_UTC_HHMM: " << end_utc_hhmm_ << DBGLOG_ENDL_FLUSH;
 
  // Setup Logger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec, logs_directory, livetrading_);
  CreateShortcodeLists(_live_file_key_val_map, source_shortcode_list_, ors_shortcode_list_, vec_key_val_map_,
                       live_folder_name_);

  int trading_start_utc_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, start_utc_hhmm_, "UTC_");
  int trading_end_utc_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, end_utc_hhmm_, "UTC_");
  int aggressive_get_flat_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, day_end_utc_hhmm_, "UTC_") - 30000;

  end_strat_timestamp = HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, day_end_utc_hhmm_, "UTC_");
  CommonInitializer *common_intializer =
      new CommonInitializer(source_shortcode_list_, ors_shortcode_list_, tradingdate_, dbglogger_, dep_trading_location_, livetrading_);

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

  HFSAT::Utils::ClientLoggingSegmentInitializer client_logging_segment_initializer(
      dbglogger_, progid_, offload_logging_filedir_char, offload_logging_filename_char);
  client_logging_segment_initializer_ptr = &client_logging_segment_initializer;

  HFSAT::ShortcodeRequestHelper shc_req_helper(progid_);
  global_shc_request_helper = &shc_req_helper;

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/tradelogs/position." << tradingdate_ << "." << progid_;
  eod_position_file_ = t_temp_oss_.str();

  // Initialize the smv source after setting the required variables
  common_intializer->SetStartEndTime(start_utc_hhmm_, end_utc_hhmm_);
  common_intializer->SetRuntimeID(progid_);

  bool use_self_book_ = Parser::GetBool(&_live_file_key_val_map, "USE_SELF_BOOK", false);
  common_intializer->SetSelfBook(use_self_book_);

  common_intializer->Initialize();
  std::vector<HFSAT::SecurityMarketView *> &p_smv_vec_ = common_intializer->getSMVMap();
  HFSAT::Watch &watch_ = common_intializer->getWatch();

  shc_req_helper.AddShortcodeListToListen(source_shortcode_list_);

  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  int runtime_id_ = progid_;

#if USE_UDP_DIRECT_SHM1_MERGE
  HFSAT::MDSMessages::CombinedMDSMessagesDirectProcessor *combined_mds_messages_shm_processor =
      common_intializer->getShmProcessor();
  if (false == tbt_recovery_allowed){
    combined_mds_messages_shm_processor->ResetRecoveryIndex();
  }
  //This is done to speedup the control msg read for BSE where there is lot of data
  if( "BSE" == exchange_ || "BSE_NSE" == exchange_){
    combined_mds_messages_shm_processor->SetControlMsgTriggerCount(100);
  }
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
    LoadOIChannelToSubscribe();
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
          if ( subsribe_to_oi_data ) {
                  for (unsigned int index = 0; index < index_oi_opt_channels_[shorthand_sc].size(); index++){
                          channel = index_oi_opt_channels_[shorthand_sc][index];
                          included_channels_.insert(channel);
                  }
          }
        } else {
          channel = index_fut_channels_[shorthand_sc];
          included_channels_.insert(channel);
          if ( subsribe_to_oi_data){
            channel = index_oi_fut_channels_[shc];
            included_channels_.insert(channel);
        }
        }
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
        if ( subsribe_to_oi_data){
          channel = fut_oi_stocks_channels_[initial];
          included_channels_.insert(channel);
        }
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

      if (shc.find("NSE_") != std::string::npos) continue;

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

    if ( subsribe_to_oi_data ) {
      included_channels_.insert(bse_oi_channels_["ALL"]);
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

  int timeout = 0;
  if(true == are_we_using_ibkr_interface) timeout = 1;
  //for CBOE declare timeout to return to process ORS and control msg
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher(timeout);


  //CBOE exchange
  if(true == are_we_using_ibkr_interface){

  }


  HFSAT::Utils::UDPDirectMuxer *udp_direct_muxer_ptr = nullptr;
  HFSAT::Utils::UDPDirectMultipleZocket *udp_direct_multiple_zockets_ptr = nullptr;
  HFSAT::Utils::ExanicMultipleRx *exanic_multiple_zockets_ptr = nullptr;

  //UDP muxer only required when pulling data from multiple exchange over different interfaces
  if(true == are_we_using_hybrid_exchange){

    udp_direct_muxer_ptr = &(HFSAT::Utils::UDPDirectMuxer::GetUniqueInstance());
    udp_direct_muxer_ptr->AddEventTimeoutNotifyListener(combined_mds_messages_shm_processor);

  }else if(true == are_we_using_exanic){

    exanic_multiple_zockets_ptr = &(HFSAT::Utils::ExanicMultipleRx::GetUniqueInstance());
    exanic_multiple_zockets_ptr->AddEventTimeoutNotifyListener(combined_mds_messages_shm_processor);

  }else{

    udp_direct_multiple_zockets_ptr = &(HFSAT::Utils::UDPDirectMultipleZocket::GetUniqueInstance());
    udp_direct_multiple_zockets_ptr->AddEventTimeoutNotifyListener(combined_mds_messages_shm_processor);

  }


  if("NSE" == exchange_ || "BSE_NSE" == exchange_){

#if USE_EXANIC_MERGE

    HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, nullptr, HFSAT::kRaw, exchange_,
                                                        true, combined_mds_messages_shm_processor, cfn_oss.str(), true);
#else
    HFSAT::NSEMD::NSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, nullptr, HFSAT::kRaw, exchange_,
                                                        true, combined_mds_messages_shm_processor, cfn_oss_nse.str());
  //        HFSAT::DataInfo control_recv_data_info_ = network_account_info_manager_.GetControlRecvUDPDirectDataInfo();
  //        udp_direct_multiple_zockets.CreateSocketAndAddToMuxer(control_recv_data_info_.bcast_ip_,
  //        control_recv_data_info_.bcast_port_, combined_mds_messages_shm_processor, 'X', false);

#endif  

    HFSAT::NSEMD::NSETBTDataProcessor &nse_tbt_data_processor = HFSAT::NSEMD::NSETBTDataProcessor::GetUniqueInstance(
        dbglogger_, HFSAT::kRaw, nullptr, combined_mds_messages_shm_processor);
    for (auto shc : source_shortcode_list_) {
      nse_tbt_data_processor.AddShortCodeForProcessing(shc);
    }

  }

  if( "BSE" == exchange_ || "BSE_NSE" == exchange_){
    HFSAT::BSEMD::BSETBTRawMDHandler::GetUniqueInstance(dbglogger_, simple_live_dispatcher, nullptr, HFSAT::kRaw, exchange_,
                                                        true, combined_mds_messages_shm_processor, tbt_recovery_allowed,cfn_oss_bse.str());
    HFSAT::BSEMD::BSETBTDataProcessor &bse_tbt_data_processor = HFSAT::BSEMD::BSETBTDataProcessor::GetUniqueInstance(
        dbglogger_, HFSAT::kRaw, nullptr, combined_mds_messages_shm_processor);

    for (auto shc : source_shortcode_list_) {
      bse_tbt_data_processor.AddShortCodeForProcessing(shc);
    }
  }

  if("CBOE" == exchange_){
    HFSAT::IBKRMD::IBKRL1MDHandler::GetUniqueInstance(dbglogger_,simple_live_dispatcher,combined_mds_messages_shm_processor,"DEFAULT");
  }

  HFSAT::MarketUpdateManager &market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, common_intializer->getSMVMap(), tradingdate_));

  std::vector<BaseTheoCalculator *> sqoff_theo_vec_;
  std::map<std::string, BaseTheoCalculator *> theo_map_;
  std::map<int, std::deque<SquareOffTheoCalculator *> > sqoff_theo_map_;
  int start_window_msec = 0;
  int end_window_msec = 0;
  int eff_squareoff_start_utc_mfm_ = 0;
  double bid_multiplier_ = 1;
  double ask_multiplier_ = 1;
  if (_live_file_key_val_map.find("START_TRADING_WINDOW") != _live_file_key_val_map.end()) {
    start_window_msec = std::atoi(_live_file_key_val_map["START_TRADING_WINDOW"].c_str());
  }
  if (_live_file_key_val_map.find("SQUAREOFF_START_WINDOW") != _live_file_key_val_map.end()) {
    end_window_msec = std::atoi(_live_file_key_val_map["SQUAREOFF_START_WINDOW"].c_str());
  }
  if (_live_file_key_val_map.find("EFF_SQUAREOFF_START_TIME") != _live_file_key_val_map.end()) {
    char *eff_squareoff_time_ = new char[strlen(_live_file_key_val_map["EFF_SQUAREOFF_START_TIME"].c_str()) + 1];
    std::strcpy(eff_squareoff_time_, _live_file_key_val_map["EFF_SQUAREOFF_START_TIME"].c_str());
    int eff_squareoff_start_utc_hhmm_ =
        HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(eff_squareoff_time_ + 4), eff_squareoff_time_);
    eff_squareoff_start_utc_mfm_ =
        HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, eff_squareoff_start_utc_hhmm_, "UTC_");
  }

  if (_live_file_key_val_map.find("BID_MULTIPLIER") != _live_file_key_val_map.end()) {
    bid_multiplier_ = std::atof(_live_file_key_val_map["BID_MULTIPLIER"].c_str());
  }

  if (_live_file_key_val_map.find("ASK_MULTIPLIER") != _live_file_key_val_map.end()) {
    ask_multiplier_ = std::atof(_live_file_key_val_map["ASK_MULTIPLIER"].c_str());
  }

  CreateTheoCalculators(vec_key_val_map_, theo_vec_, sqoff_theo_vec_, theo_map_, sqoff_theo_map_, watch_, dbglogger_,
                        trading_start_utc_mfm_, start_window_msec, trading_end_utc_mfm_, end_window_msec,
                        aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);

  std::string pos_file_tag_ = _live_file_key_val_map["POSLIMIT"];
  std::string position_file_ = live_folder_name_ + "/" + pos_file_tag_;
  std::map<std::string, std::string> pos_key_val_map_;
  Parser::ParseConfig(position_file_, pos_key_val_map_);

   HFSAT::IndexedNSEMarketViewManager2 *indexed_nse_market_view_manager =
      common_intializer->indexed_nse_market_view_manager();
  HFSAT::IndexedBSEMarketViewManager2 *indexed_bse_market_view_manager =
      common_intializer->indexed_bse_market_view_manager();

  for (auto theo_calc_ : theo_vec_) {
    theo_calc_->SetRuntimeID(runtime_id_);
    int sec_id_ = theo_calc_->GetSecondaryID();
    bool is_modify_before_confirmation_ = theo_calc_->IsOnFlyModifyAllowed();
    bool is_cancellable_before_confirmation_ = theo_calc_->IsOnFlyCancelAllowed();
    bool is_sqoff_needed_ =
        theo_calc_->IsSecondarySqOffNeeded();  // In case hedge is already there but we also need a squareoff of
                                               // secondary in case of remaining positions
    SquareOffTheoCalculator *sqoff_theo_calc_ = NULL;
    theo_calc_->ConfigureHedgeDetails(theo_map_);
    if ("MASTER_THEO" == theo_calc_->GetTheoType()) {
      MasterTheoCalculator *master_theo_calc_ = dynamic_cast<MasterTheoCalculator *>(theo_calc_);
      master_theo_calc_->ConfigureMidTermDetails(theo_map_);
    }
     else if ("CORR_THEO" == theo_calc_->GetTheoType()) {
      CorrTheoCalculator* corr_theo_calc_ = dynamic_cast<CorrTheoCalculator*>(theo_calc_);
      corr_theo_calc_->ConfigureMidTermDetails(theo_map_);
    }
    if ((!theo_calc_->IsNeedToHedge()) || (is_sqoff_needed_)) {
      if ((sqoff_theo_map_.find(sec_id_) != sqoff_theo_map_.end() && (sqoff_theo_map_[sec_id_].size() > 0))) {
        sqoff_theo_calc_ = sqoff_theo_map_[sec_id_].front();
        sqoff_theo_map_[sec_id_].pop_front();
        theo_calc_->SetSquareOffTheo(sqoff_theo_calc_);
      }
      if (!sqoff_theo_calc_) {
        std::cerr << "SQUAREOFF THEO not give for secID " << sec_id_ << std::endl;
        exit(-1);
      }
    }
    auto dep_smv_ = p_smv_vec_[sec_id_];
    assert(dep_smv_ != nullptr);
    dbglogger_ << "SHC: " << theo_calc_->GetSecondaryShc() << " SYM: " << theo_calc_->GetSecondarySecname() << " ";

    HFSAT::BaseTrader *base_trader_;

    if (exchange_ == "BSE" || exchange_ == "BSE_NSE") {
      base_trader_ = GetBSETrader(theo_calc_->GetSecondaryShc(), sec_id_, network_account_info_manager_, watch_,test_flag);
      if (use_self_book_) {
        indexed_bse_market_view_manager->AddSelfTraderId(base_trader_->GetClientId());
      }
      if (theo_calc_->IsBigTradesListener()) {
        indexed_bse_market_view_manager->AddBigTradesListener(theo_calc_, theo_calc_->GetPrimaryID());
      }
    } else if("NSE" == exchange_){
      base_trader_ = GetNSETrader(theo_calc_->GetSecondaryShc(), sec_id_, network_account_info_manager_, watch_,test_flag);
      if (use_self_book_) {
        indexed_nse_market_view_manager->AddSelfTraderId(base_trader_->GetClientId());
      }
      if (theo_calc_->IsBigTradesListener()) {
        indexed_nse_market_view_manager->AddBigTradesListener(theo_calc_, theo_calc_->GetPrimaryID());
      }
    }else{
      base_trader_ = new HFSAT::BaseLiveTrader(
          HFSAT::kExchSourceBMF,
          network_account_info_manager_.GetDepTradeAccount(HFSAT::kExchSourceBMF, theo_calc_->GetSecondaryShc()),
          network_account_info_manager_.GetDepTradeHostIp(HFSAT::kExchSourceBMF, theo_calc_->GetSecondaryShc()),
          network_account_info_manager_.GetDepTradeHostPort(HFSAT::kExchSourceBMF, theo_calc_->GetSecondaryShc()),
          watch_, dbglogger_);
    }

    auto basic_om_ = new HFSAT::BasicOrderManager(dbglogger_, watch_, sec_name_indexer_, *base_trader_, *dep_smv_, 1,
                                                  pos_key_val_map_, livetrading_, is_modify_before_confirmation_,
                                                  is_cancellable_before_confirmation_);
    basic_om_->AddExecutionListener(theo_calc_);
    HFSAT::BasePNL *live_base_pnl = nullptr;
    live_base_pnl = new HFSAT::LiveBasePNL(dbglogger_, watch_, *dep_smv_, theo_calc_->GetRuntimeID(),
                                           client_logging_segment_initializer_ptr);

    basic_om_->SetBasePNL(live_base_pnl);
    // live_base_pnl->AddListener(0, theo_calc_);

    SubscribeToORSReply(combined_mds_messages_shm_processor, basic_om_, sec_id_);

    theo_calc_->SetBasePNL(live_base_pnl);
    theo_calc_->SetupPNLHooks();
    theo_calc_->CreateAllExecutioners(basic_om_, livetrading_);
    theo_calc_->SetOMSubscriptions();
    // TODO Create new om for Square off theo
    if (sqoff_theo_calc_) {
      sqoff_theo_calc_->CreateAllExecutioners(basic_om_, livetrading_);
    }

    runtime_id_++;
  }

  PortfolioRiskManager *risk_manager_ =
      new PortfolioRiskManager(dbglogger_, watch_, theo_vec_, sqoff_theo_vec_, pos_key_val_map_);

  UserControlManager *user_control_manager_ =
      new UserControlManager(dbglogger_, watch_, theo_vec_, sqoff_theo_vec_, position_file_, risk_manager_);
  combined_mds_messages_shm_processor->AddControlSourceForProcessing(HFSAT::MDS_MSG::CONTROL, progid_,
                                                                     user_control_manager_, &watch_);

  // Do not change logging format (risk monitor uses this line)
  dbglogger_ << "DUMPEDALLSACIS (logging for monitoring)\n";
  dbglogger_.DumpCurrentBuffer();

  if (exchange_ == "BMF"){
    market_update_manager_.start();
  }

#if USE_UDP_DIRECT_SHM1_MERGE
  std::cout << "RUN LIVE DIS" << std::endl ;

  if(true == are_we_using_hybrid_exchange){
    udp_direct_muxer_ptr->RunMuxerLiveDispatcherWithTimeOut(100);
  }else if(true == are_we_using_exanic){
    exanic_multiple_zockets_ptr->RunLiveDispatcherWithNotify();
  }else{
    if(true == are_we_using_ibkr_interface){
      simple_live_dispatcher.RunLive();
    }else{
      udp_direct_multiple_zockets_ptr->RunLiveDispatcherWithTimeOut(2000);
    }
  }
#else
  combined_mds_messages_shm_processor->RunLiveShmSource();
#endif

  return 0;
}
