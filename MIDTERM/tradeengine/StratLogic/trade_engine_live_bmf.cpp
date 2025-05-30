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

#include "tradeengine/CommonInitializer/common_initializer.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/signals.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "tradeengine/Utils/Parser.hpp"
#include "tradeengine/TheoCalc/RatioTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MomentumTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MACDTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MasterTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MispriceTheoCalculator.hpp"
#include "tradeengine/TheoCalc/HedgeTheoCalculator.hpp"
#include "tradeengine/StratLogic/portfolio_risk_manager.hpp"
#include "tradeengine/StratLogic/user_control_manager.hpp"
#include "baseinfra/LivePnls/live_base_pnl.hpp"
#include "baseinfra/BaseTrader/base_live_trader.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "baseinfra/MDSMessages/combined_mds_messages_multi_shm_processor.hpp"

// For Offloaded Logging
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"

#include "dvccode/Utils/shortcode_request_helper.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

HFSAT::OnlineDebugLogger *online_dbglogger_ = new HFSAT::OnlineDebugLogger(1024 * 4 * 1024, 1109, 256 * 1024);
HFSAT::DebugLogger &dbglogger_ = *(online_dbglogger_);  //( 4*1024*1024, 256*1024 ); // making logging more efficient,
// otherwise change it backto 10240, 1

HFSAT::Utils::ClientLoggingSegmentInitializer *client_logging_segment_initializer_ptr = NULL;
HFSAT::ShortcodeRequestHelper *global_shc_request_helper;
std::string eod_position_file_;

typedef std::vector<std::map<std::string, std::string> *> VEC_KEY_VAL_MAP;
typedef std::map<std::string, std::string> KEY_VAL_MAP;

std::vector<BaseTheoCalculator *> theo_vec_;
int progid_;
int tradingdate_;

void termination_handler(int signum) {
  std::cout << "Exiting due to signal " << SimpleSignalString(signum) << std::endl;
  char hostname_[128];
  hostname_[127] = '\0';
  gethostname(hostname_, 127);

  std::string position_email_string_ = "";
  std::ostringstream t_oss_;

  t_oss_ << "NSE Strategy: " << progid_ << " exiting signal: " << SimpleSignalString(signum) << " on " << hostname_
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
      t_oss_1 << "SHC: " << theo_calc_->GetSecondaryShc() << " SECNAME: " << theo_calc_->GetSecondarySecname()
              << " POS: " << theo_calc_->GetPosition() << "\n";
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

  //  if (position_email_string_ != subject_email_string_) {
  HFSAT::Email email_;
  email_.setSubject(subject_email_string_);
  email_.addRecepient("nsehft@tworoads.co.in");
  email_.addSender("uttkarsh.sarraf@tworoads.co.in");
  email_.content_stream << position_email_string_ << "<br/>";
  email_.sendMail();
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

bool CollectTheoSourceShortCode(KEY_VAL_MAP *key_val_map_, std::vector<std::string> &source_shortcode_list) {
  uint32_t count = 0;
  std::string primary_symbol = std::string("PRIMARY") + std::to_string(count);
  std::vector<std::string> theo_shortcode_list_;
  while (key_val_map_->find(primary_symbol) != key_val_map_->end()) {
    std::string shc_ = (*key_val_map_)[primary_symbol];
    if (HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shc_) ||
        HFSAT::NSESecurityDefinitions::IsShortcode(shc_)) {
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
    if ((Parser::GetBool(key_val_map, "STATUS", false)) &&
        (HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(
            Parser::GetString(key_val_map, "SECONDARY", "DEFAULT")))) {
      bool is_valid_shc_config_ = CollectTheoSourceShortCode(key_val_map, source_shortcode_list_);
      if (is_valid_shc_config_) {
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
      if (key_val_map->find("THEO_IDENTIFIER") == key_val_map->end()) {
        std::cerr << "THEO IDENTIFIER missing for a theo!! (exiting)" << std::endl;
        exit(-1);
      }
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
      } else if (theo_type == "MIDTERM_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc = new MidTermTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
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
      } else if (theo_type == "MASTER_THEO") {
        // std::cout << "Creating MasterTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator *theo_calc =
            new MasterTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
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
                                HFSAT::NetworkAccountInfoManager &network_account_info_manager_, HFSAT::Watch &watch_) {
  HFSAT::Utils::NSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);

  HFSAT::NSESecurityDefinitions &nse_sec_def = HFSAT::NSESecurityDefinitions::GetUniqueInstance(tradingdate_);
  char segment = nse_sec_def.GetSegmentTypeFromShortCode(shortcode_);

  if (NSE_INVALID_SEGMENT == segment) {
    dbglogger_ << "SHORTCODE : " << shortcode_ << " RETURNS INVALID NSE SEGMENT \n";
    dbglogger_.DumpCurrentBuffer();
    exit(-1);
  }

  std::string const internal_symbol =
      HFSAT::SecurityNameIndexer::GetUniqueInstance().GetSecurityNameFromId(security_id_);

  std::string const data_symbol_name = nse_sec_def.ConvertExchSymboltoDataSourceName(internal_symbol);
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
      network_account_info_manager_.GetDepTradeHostPort(HFSAT::kExchSourceNSE, shortcode_), watch_, dbglogger_);

  return p_base_trader_;
}

void SubscribeToORSReply(HFSAT::MDSMessages::CombinedMDSMessagesMultiShmProcessor *combined_mds_messages_shm_processor,
                         HFSAT::BasicOrderManager *smart_om, int security_id) {
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderNotFoundListener(security_id, smart_om);
  combined_mds_messages_shm_processor->GetProShmORSReplyProcessor()->AddOrderSequencedListener(security_id, smart_om);
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
              << " $tradeengineexec STRAT_LIVE_FILE TRADINGDATE START_TIME END_TIME PROGID" << '\n';
    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  }

  std::string _live_file = argv[1];
  tradingdate_ = atoi(argv[2]);
  int start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[3] + 4), argv[3]);
  int end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[4] + 4), argv[4]);
  progid_ = atoi(argv[5]);
  std::string exchange_ = "NSE";

  bool livetrading_ = true;
  std::string md5sum = "";

  if (argc > 6) {
    exchange_ = argv[6];
  }

  std::vector<std::string> dbg_code_vec = {"FILL_TIME_INFO"};

  std::map<std::string, std::string> _live_file_key_val_map;
  Parser::ParseConfig(_live_file, _live_file_key_val_map);

  std::string live_folder_name_ = ".";

  const size_t last_slash_idx = _live_file.rfind('/');
  if (std::string::npos != last_slash_idx) {
    live_folder_name_ = _live_file.substr(0, last_slash_idx);
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  if (exchange_ == "NSE") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }

  std::vector<std::string> source_shortcode_list_;
  std::vector<std::string> ors_shortcode_list_;
  VEC_KEY_VAL_MAP vec_key_val_map_;
  std::string logs_directory = "/spare/local/logs/tradelogs";

  int day_end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, 1530, "IST_1530");
  if (strcmp(exchange_.c_str(), "NSE") == 0) {
    if (end_utc_hhmm_ > 959) {
      end_utc_hhmm_ = 959;
    }
  } else if (strcmp(exchange_.c_str(), "BMF") == 0) {
    day_end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, 1755, "BRT_1755");
  }

  // Setup Logger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec, logs_directory, livetrading_);
  CreateShortcodeLists(_live_file_key_val_map, source_shortcode_list_, ors_shortcode_list_, vec_key_val_map_,
                       live_folder_name_);

  int trading_start_utc_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, start_utc_hhmm_, "UTC_");
  int trading_end_utc_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, end_utc_hhmm_, "UTC_");
  int aggressive_get_flat_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, day_end_utc_hhmm_, "UTC_") - 30000;

  CommonInitializer *common_intializer =
      new CommonInitializer(source_shortcode_list_, ors_shortcode_list_, tradingdate_, dbglogger_, livetrading_);

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

  /*	for (auto scode : source_shortcode_list_) {
          std::cout << " Source shortcode " << scode << std::endl;
          }

          for (auto ocode : ors_shortcode_list_) {
  std::cout << " ORS shortcode " << ocode << std::endl;
          }*/

  shc_req_helper.AddShortcodeListToListen(source_shortcode_list_);

  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  int runtime_id_ = progid_;

  HFSAT::MDSMessages::CombinedMDSMessagesMultiShmProcessor *combined_mds_messages_shm_processor =
      common_intializer->getBMFShmProcessor();
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

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

  for (auto theo_calc_ : theo_vec_) {
    theo_calc_->SetRuntimeID(runtime_id_);
    int sec_id_ = theo_calc_->GetSecondaryID();
    bool is_modify_before_confirmation_ = theo_calc_->IsOnFlyModifyAllowed();
    bool is_cancellable_before_confirmation_ = theo_calc_->IsOnFlyCancelAllowed();
    bool is_sqoff_needed_ = theo_calc_->IsSecondarySqOffNeeded();  // In case hedge is already there but we also need a
                                                                   // squareoff of secondary in case of remaining
                                                                   // positions
    SquareOffTheoCalculator *sqoff_theo_calc_ = NULL;
    theo_calc_->ConfigureHedgeDetails(theo_map_);
    if ("MASTER_THEO" == theo_calc_->GetTheoType()) {
      MasterTheoCalculator *master_theo_calc_ = dynamic_cast<MasterTheoCalculator *>(theo_calc_);
      master_theo_calc_->ConfigureMidTermDetails(theo_map_);
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
    if (strcmp(exchange_.c_str(), "NSE") == 0) {
      base_trader_ = GetNSETrader(theo_calc_->GetSecondaryShc(), sec_id_, network_account_info_manager_, watch_);
      if (use_self_book_) {
        indexed_nse_market_view_manager->AddSelfTraderId(base_trader_->GetClientId());
      }
      if (theo_calc_->IsBigTradesListener()) {
        indexed_nse_market_view_manager->AddBigTradesListener(theo_calc_, theo_calc_->GetPrimaryID());
      }
    } else {
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

  market_update_manager_.start();

  combined_mds_messages_shm_processor->RunLiveShmSourceForBMF();

  return 0;
}
