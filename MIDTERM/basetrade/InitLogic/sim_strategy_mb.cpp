/**
   \file InitLogic/sim_strategy_mb.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "basetrade/InitLogic/sim_strategy_mb.hpp"

#define MIN_YYYYMMDD 20090920
#define SECONDS_TO_PREP 1800

HFSAT::DebugLogger* global_dbglogger_ = nullptr;

HFSAT::BulkFileWriter trades_writer_(256 * 1024);  // 256KB
// To be used in termination_handler
int global_tradingdate_ = 0;
std::string global_strategy_desc_filename_ = "";
std::string global_strategy_desc_ = "";
unsigned int global_progid_ = 0;
unsigned int global_secs_to_prep_ = SECONDS_TO_PREP;

void termination_handler(int signum) {
  // TODO check for strategies with open positions and print the same
  if (global_dbglogger_ != nullptr) {
    global_dbglogger_->Close();
  }
  trades_writer_.Close();

  if (signum == SIGSEGV || signum == SIGILL ||
      signum == SIGFPE) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);
    bool send_slack_notification = false;
    std::string newline = "\n";
    std::string job_desc = "UNKNOWN";

    // Send slack notifications to #AWS-issues channel if running on EC2, otherwise send mails
    if (strncmp(hostname_, "ip-10-0-1", 9) == 0) {
      send_slack_notification = true;
      newline = "\\n";  // slack will need extra escaping
      // Fetch parent identifier (job in scheduler), as we may need to remove it from the scheduler
      char* job_id_ptr = getenv("DVC_JOB_ID");
      if (job_id_ptr != NULL) {
        job_desc = std::string(job_id_ptr);
      }
    }

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "sim_strategy received " << SimpleSignalString(signum) << " on " << hostname_ << newline;
      t_oss_ << "Parent job identifier: " << job_desc << newline;
      t_oss_ << "tradingdate_= " << global_tradingdate_ << newline
             << "strategy_desc_filename_= " << global_strategy_desc_filename_ << newline
             << "progid_= " << global_progid_ << newline << "strategy_desc_= " << global_strategy_desc_ << newline;

      email_string_ = t_oss_.str();
    }

    if (send_slack_notification) {
      HFSAT::SlackManager slack_manager(AWSISSUES);
      slack_manager.sendNotification(email_string_);

    } else {
      HFSAT::Email email;
      email.setSubject(email_string_);

      if (!strncmp(getenv("USER"), "dvctrader", strlen("dvctrader"))) {
        email_address_ = "nseall@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "ankit", strlen("ankit"))) {
        email_address_ = "ankit@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "rkumar", strlen("rkumar"))) {
        email_address_ = "rakesh.kumar@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "ravi", strlen("ravi"))) {
        email_address_ = "ravi.parikh@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "kputta", strlen("kputta"))) {
        email_address_ = "kputta@circulumvite.com";
      } else if (!strncmp(getenv("USER"), "diwakar", strlen("diwakar"))) {
        email_address_ = "diwakar@circulumvite.com";
      } else if (!strncmp(getenv("USER"), "vedant", strlen("vedant"))) {
        email_address_ = "vedant@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "hagarwal", strlen("hagarwal"))) {
        email_address_ = "hrishav.agarwal@tworoads.co.in";
      } else {  // Not sure if others want to receive these emails.
        email_address_ = "nseall@tworoads.co.in";
      }

      email.addRecepient(email_address_);
      email.addSender(email_address_);
      // email_.content_stream << email_string_ << "<br/>";
      // email.sendMail();
    }

    abort();
  }

  exit(0);
}

void ReportResults(std::map<int, std::vector<HFSAT::MinuteBarPNL*>> client_id_to_pnl_vec) {
  for (auto pair : client_id_to_pnl_vec) {
    auto vec = pair.second;
    for (auto pnl : vec) {
      std::cout << pnl->secname() << " " << pnl->pnl() << std::endl;
    }
  }
}

void GetSecToPrep(std::string _shortcode_, unsigned int& _secs_to_prep_) {
  if ((_shortcode_.compare("NK_0") == 0)) {
    _secs_to_prep_ = 600;
  } else if (_shortcode_.compare("SXF_0") == 0) {
    _secs_to_prep_ = 900;
  }
}

void InitTradesLogger(int tradingdate, int progid, bool livetrading) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/tradelogs/trades." << tradingdate << "." << progid;
  std::string tradesfilename = t_temp_oss_.str();

  // open trades file in append mode in livetrading_
  trades_writer_.Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));
}

void InitDbglogger(int tradingdate, int progid, std::vector<std::string>& dbg_code_vec,
                   MinuteBarSMVSource* minute_bar_smv_source, HFSAT::DebugLogger& dbglogger) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/logs/tradelogs/log." << tradingdate << "." << progid;
  std::string logfilename = t_temp_oss.str();

  minute_bar_smv_source->SetDbgloggerFileName(logfilename);

  for (auto i = 0u; i < dbg_code_vec.size(); i++) {
    // TODO .. add ability to write "WATCH_INFO" instead of 110, and making it
    int dbg_code_to_be_logged = HFSAT::DebugLogger::TextToLogLevel(dbg_code_vec[i].c_str());
    if (dbg_code_to_be_logged < 0) {
      dbglogger.SetNoLogs();
      break;
    } else {
      dbglogger.AddLogLevel(dbg_code_to_be_logged);
    }
  }

  dbglogger.AddLogLevel(WATCH_ERROR);
  dbglogger.AddLogLevel(OM_ERROR);
  dbglogger.AddLogLevel(PLSMM_ERROR);
  dbglogger.AddLogLevel(TRADING_ERROR);
  dbglogger.AddLogLevel(BOOK_ERROR);
  dbglogger.AddLogLevel(LRDB_ERROR);
  dbglogger.AddLogLevel(DBG_MODEL_ERROR);
  dbglogger.AddLogLevel(SMVSELF_ERROR);
}

void GetDependentShortcodes() {}

void GetORSShortcodes() {}

void GetSourceShortcodes() {}

// returns true if given shortcode corresponds to an HK equity
bool IsHKEquity(std::string _shortcode) { return _shortcode.substr(0, 3) == "HK_"; }

void ParseCommandLineParams(int argc, char** argv, int& tradingdate_, std::string& strategy_desc,
                            unsigned int& progid_) {
  if (argc < 5) {
    std::cerr << "expecting :\n"
              << " 1. $dim_strategy_exec SIM STRAT_FILE PROGID TRADINGDATE " << '\n';

    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  }

  strategy_desc = argv[2];
  progid_ = atoi(argv[3]);
  tradingdate_ = atoi(argv[4]);
}

void CreateSignalsAndSubscribeLogic(HFSAT::DebugLogger& dbglogger, HFSAT::Watch& watch,
                                    std::vector<std::vector<std::string>> tokens_list,
                                    HFSAT::BaseMinuteBarExecLogic* exec_logic) {
  for (auto i = 0u; i < tokens_list.size(); i++) {
    auto vector = tokens_list[i];
    std::vector<const char*> tokens;
    for (auto str : vector) {
      tokens.push_back(str.c_str());
    }
    HFSAT::BaseMinuteBarSignal* signal = HFSAT::SignalHelper::GetUniqueInstanceFunc(tokens[0])(
        dbglogger, watch, tokens, HFSAT::MinuteBarPriceType::kPriceTypeClose);
    signal->AddSignalListener(exec_logic, i);
  }
}

int main(int argc, char** argv) {
  std::string do_not_run_studies_file_ = "/home/dvctrader/.DO_NOT_RUN_STUDIES";
  if (HFSAT::FileUtils::exists(do_not_run_studies_file_)) {
    std::cerr << "File: " << do_not_run_studies_file_ << " exists." << std::endl;
    std::cerr << "Either you're not supposed to run this exec on this machine or remove this file." << std::endl;
    std::cerr << "Exiting now." << std::endl;
    exit(-1);
  }

  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGILL, termination_handler);
  signal(SIGFPE, termination_handler);
  bool livetrading_ = false;
  int tradingdate_ = 0;
  unsigned int progid_ = 0;
  std::string strategy_desc_filename_ = "";

  unsigned int secs_to_prep_ = SECONDS_TO_PREP;

  HFSAT::ttime_t strat_start_time_(0, 0);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  // vector of all sources which we need data for or are trading
  std::vector<std::string> source_shortcode_vec_;

  std::vector<std::string> dependant_shortcode_vec_;

  // security_id to bool map indicating whether market data is needed or not for this source ...
  std::vector<bool> sid_to_marketdata_needed_map_;

  std::vector<std::string> dbg_code_vec_;  ///< set of codes that we want the logger to print on

  // parse command line paramters
  ParseCommandLineParams(argc, argv, tradingdate_, strategy_desc_filename_, progid_);

  // Make an object of CommonSMVSource and use it as an API
  std::vector<std::string> dummy_shc_list;
  MinuteBarSMVSource* minute_bar_smv_source = new MinuteBarSMVSource(dummy_shc_list, tradingdate_);

  // Get the dbglogger and watch after creating the source
  HFSAT::Watch& watch = minute_bar_smv_source->getWatch();
  HFSAT::DebugLogger& dbglogger = minute_bar_smv_source->getLogger();
  global_dbglogger_ = &dbglogger;

  // Setup DebugLogger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec_, minute_bar_smv_source, dbglogger);
  InitTradesLogger(tradingdate_, progid_, livetrading_);
  global_progid_ = progid_;
  global_tradingdate_ = tradingdate_;

  HFSAT::MinuteBarStrategyDesc strategy_desc(dbglogger, strategy_desc_filename_, tradingdate_);
  source_shortcode_vec_ = strategy_desc.GetAllSourceShortcodes();
  dependant_shortcode_vec_ = strategy_desc.GetAllDepShortcodes();

  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, time_t(strategy_desc.GetMinStartTime().tv_sec));
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger, watch);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  // Set all the parameters in the common_smv_source
  minute_bar_smv_source->SetSourceShortcodes(source_shortcode_vec_);
  minute_bar_smv_source->SetDepShortcodeVector(dependant_shortcode_vec_);
  minute_bar_smv_source->SetDepShortcode(dependant_shortcode_vec_[0]);
  // Initialize the smv source after setting the required variables
  minute_bar_smv_source->Initialize();

  // Commenting unused variables. Will delete them later
  // HFSAT::SecIDMinuteBarSMVMap& sid_to_smv_ptr_map_ = minute_bar_smv_source->getSMVMap();
  // HFSAT::HistoricalDispatcher& historical_dispatcher_ = minute_bar_smv_source->getHistoricalDispatcher();

  HFSAT::ShortcodeMinuteBarSMVMap& shortcode_smv_map_ = HFSAT::ShortcodeMinuteBarSMVMap::GetUniqueInstance();
  HFSAT::StrategySecurityPairToMinuteBarOrderManager& startegy_sec_id_to_om_map =
      HFSAT::sid_to_minute_bar_order_manager_map();

  std::map<int, std::vector<HFSAT::MinuteBarPNL*>> client_id_to_pnl_vec;

  // Just to initialize the signal map
  HFSAT::SignalHelper::SetSignalListMap();

  for (auto strategy_line : strategy_desc.strategy_vec_) {
    int client_id = strategy_line.runtime_id_;

    HFSAT::BaseMinuteBarExecLogic* exec_logic = new HFSAT::BaseMinuteBarExecLogic(
        dbglogger, watch, strategy_line.exec_name_, strategy_line.config_file_, client_id, startegy_sec_id_to_om_map);

    // Subscribe the exec logic with all the source shcs
    for (auto shc : strategy_line.indep_shortcode_list_) {
      shortcode_smv_map_.GetSecurityMarketView(shc)->SubscribeMinuteBars(exec_logic);
    }

    // Create the signal instances from the strat file and subscribe exec logic to that
    CreateSignalsAndSubscribeLogic(dbglogger, watch, strategy_line.signal_tokens_list_, exec_logic);

    for (auto dep_shc : strategy_line.dep_shortcode_list_) {
      int security_id = sec_name_indexer_.GetIdFromString(dep_shc);
      std::string exch_symbol = sec_name_indexer_.GetSecurityNameFromId(security_id);

      HFSAT::MinuteBarSecurityMarketView* minute_bar_smv = shortcode_smv_map_.GetSecurityMarketView(dep_shc);
      HFSAT::MinuteBarSimTrader* sim_trader = new HFSAT::MinuteBarSimTrader(client_id, security_id);

      HFSAT::MinuteBarOrderManager* order_manager = new HFSAT::MinuteBarOrderManager(
          dbglogger, watch, sec_name_indexer_, *sim_trader, *minute_bar_smv, client_id, false, 1);

      startegy_sec_id_to_om_map[make_pair(client_id, security_id)] = order_manager;

      HFSAT::MinuteBarPNL* minute_bar_pnl =
          new HFSAT::MinuteBarPNL(dbglogger, watch, *order_manager, client_id, dep_shc, exch_symbol, trades_writer_);
      (void)minute_bar_pnl;

      client_id_to_pnl_vec[client_id].push_back(minute_bar_pnl);

      if ((sim_trader != NULL) && (order_manager != NULL)) {
        sim_trader->AddOrderSequencedListener(order_manager);
        sim_trader->AddOrderConfirmedListener(order_manager);
        sim_trader->AddOrderConfCxlReplaceRejectedListener(order_manager);
        sim_trader->AddOrderConfCxlReplacedListener(order_manager);
        sim_trader->AddOrderCanceledListener(order_manager);
        sim_trader->AddOrderExecutedListener(order_manager);
        sim_trader->AddOrderRejectedListener(order_manager);
      }
    }
  }

  // To only process data starting MINUTES_TO_PREP minutes before min start time of strategies
  // In all the file sources, read events till
  // we reach the first event after the specified ttime_t

  HFSAT::ttime_t min_strat_trading_time_ = strategy_desc.GetMinStartTime();
  HFSAT::ttime_t data_seek_time_ = min_strat_trading_time_ - HFSAT::ttime_t((int)secs_to_prep_, 0);
  if (!(strat_start_time_ == HFSAT::ttime_t(0, 0)) && strat_start_time_ <= min_strat_trading_time_) {
    // give input is sane , i.e. before trading time
    data_seek_time_ = strat_start_time_;
  }

  // To avoid complications we gotta start the data on the same day which we intend to trade on.
  if (min_strat_trading_time_ >= HFSAT::ttime_t(watch.last_midnight_sec(), 0) &&
      data_seek_time_ < HFSAT::ttime_t(watch.last_midnight_sec(), 0)) {
    data_seek_time_ = HFSAT::ttime_t(watch.last_midnight_sec(), 0);
  }

  // Seek to start time
  minute_bar_smv_source->Seek(data_seek_time_);

  HFSAT::ttime_t end_time = strategy_desc.GetMaxEndTime() + HFSAT::ttime_t(3600, 0);

  // for DI1* we don't want to trade beyond BRT_1600, because it enters into pre-open/close, gap was creating problems
  // in final pnl
  for (auto i = 0u; i < dependant_shortcode_vec_.size(); i++) {
    if (strncmp(dependant_shortcode_vec_[i].c_str(), "DI1", strlen("DI1")) == 0) {
      time_t t_di_max_end_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, 1600, "BRT");
      end_time = std::min(HFSAT::ttime_t(t_di_max_end_time_, 0), end_time);
      break;
    }
  }

  // Run event loop
  minute_bar_smv_source->Run(end_time);

  ReportResults(client_id_to_pnl_vec);

  return 0;
}
