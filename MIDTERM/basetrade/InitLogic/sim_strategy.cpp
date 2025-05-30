/**
   \file InitLogic/sim_strategy.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "basetrade/InitLogic/sim_strategy.hpp"

//#define CCPROFILING 0

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225
#define SECONDS_TO_PREP 1800

HFSAT::DebugLogger* global_dbglogger_ = nullptr;

HFSAT::BulkFileWriter trades_writer_(256 * 1024);  // 256KB
// To be used in termination_handler
bool global_livetrading_ = false;
int global_tradingdate_ = 0;
std::string global_strategy_desc_filename_ = "";
std::string global_strategy_desc_ = "";
unsigned int global_progid_ = 0;
unsigned int global_market_model_index_ = 0;
unsigned int global_secs_to_prep_ = SECONDS_TO_PREP;
std::vector<HFSAT::SmartOrderManager*> all_om_vec;
void termination_handler(int signum) {
  // TODO check for strategies with open positions and print the same
  for (unsigned i = 0; i < all_om_vec.size(); i++) {
    all_om_vec[i]->PrintCancelSeqdExecTimes();
  }
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
      if (job_id_ptr != nullptr) {
        job_desc = std::string(job_id_ptr);
      }
    }

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "sim_strategy received " << SimpleSignalString(signum) << " on " << hostname_ << newline;
      t_oss_ << "Parent job identifier: " << job_desc << newline;
      t_oss_ << "livetrading_= " << global_livetrading_ << newline << "tradingdate_= " << global_tradingdate_ << newline
             << "strategy_desc_filename_= " << global_strategy_desc_filename_ << newline
             << "progid_= " << global_progid_ << newline << "market_model_index_= " << global_market_model_index_
             << newline << newline << "strategy_desc_= " << global_strategy_desc_ << newline;

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
      } else if (!strncmp(getenv("USER"), "mayank", strlen("mayank"))) {
        email_address_ = "mayank@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "hardik", strlen("hardik"))) {
        email_address_ = "hardik@circulumvite.com";
      } else if (!strncmp(getenv("USER"), "rkumar", strlen("rkumar"))) {
        email_address_ = "rakesh.kumar@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "ravi", strlen("ravi"))) {
        email_address_ = "ravi.parikh@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "kputta", strlen("kputta"))) {
        email_address_ = "kputta@circulumvite.com";
      } else if (!strncmp(getenv("USER"), "anshul", strlen("anshul"))) {
        email_address_ = "anshul@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "diwakar", strlen("diwakar"))) {
        email_address_ = "diwakar@circulumvite.com";
      } else if (!strncmp(getenv("USER"), "archit", strlen("archit"))) {
        email_address_ = "archit@circulumvite.com";
      } else if (!strncmp(getenv("USER"), "sushant", strlen("sushant"))) {
        email_address_ = "sushant.garg@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "vedant", strlen("vedant"))) {
        email_address_ = "vedant@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "hagarwal", strlen("hagarwal"))) {
        email_address_ = "hrishav.agarwal@tworoads.co.in";
      } else if (!strncmp(getenv("USER"), "ashwin", strlen("ashwin"))) {
        email_address_ = "ashwin.kumar@tworoads.co.in";
      } else {  // Not sure if others want to receive these emails.
        email_address_ = "nseall@tworoads.co.in";
      }

      email.addRecepient(email_address_);
      email.addSender(email_address_);
      // email_.content_stream << email_string_ << "<br/>";
      email.sendMail();
    }
    abort();
  }

  exit(0);
}

void GetSecToPrep(std::string _shortcode_, unsigned int& _secs_to_prep_) {
  if ((_shortcode_.compare("NK_0") == 0))
  //        ( _shortcode_.compare ( "NKM_0" ) == 0  ) ||
  //      ( _shortcode_.compare ( "NKMF_0" ) == 0  ) ||
  //        ( _shortcode_.compare ( "TOPIX_0" ) == 0 ) ||
  //        ( _shortcode_.compare ( "JGBL_0" ) == 0 ) )
  {
    _secs_to_prep_ = 600;
  } else if (_shortcode_.compare("SXF_0") == 0) {
    _secs_to_prep_ = 900;
  }
}
/// expect :
/// 1. $tradeinitexec "SIM" STRATEGYDESCFILENAME PROGID TRADINGDATE
///    $tradeinitexec "SIM" STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX
void ParseCommandLineParams(int argc, char** argv, bool& livetrading_, int& tradingdate_,
                            std::string& strategy_desc_filename_, unsigned int& progid_,
                            std::string& network_account_info_filename_, unsigned int& market_model_index_,
                            bool& ignore_user_msg_, HFSAT::ttime_t& t_strat_start_time_, bool& t_use_fake_faster_data_,
                            std::vector<std::string>& dbg_code_vec_, std::vector<int>& real_saci_vec_,
                            bool& use_l1_data_, std::string& logs_directory) {
  if (argc < 4) {  // 4 is min of live and sim
    std::cerr << "expecting :\n"
              << " 1. $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 USE_CONTROL_MSG=0 "
                 "DATA_START_TIME=0.0 USE_FAKE_FAST_DATA=1 NETWORKINFOFILENAME=SAME_NETWORK_INFO " << '\n';

    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  } else {
    if (strcmp(argv[1], "LIVE") == 0) {
      std::cerr << "expecting :\n"
                << " 1. $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 "
                   "USE_CONTROL_MSG=0 DATA_START_TIME=0.0 USE_FAKE_FAST_DATA=1 NETWORKINFOFILENAME=SAME_NETWORK_INFO "
                << '\n';

      HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
    } else {
      livetrading_ = false;
      if (argc < 5) {
        std::cerr << "expecting :\n"
                  << " 1. $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 "
                     "USE_CONTROL_MSG=0 DATA_START_TIME=0.0 USE_FAKE_FAST_DATA=1 NETWORKINFOFILENAME=SAME_NETWORK_INFO "
                  << '\n';

        HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
      }

      strategy_desc_filename_ = argv[2];

      progid_ = std::max(0, atoi(argv[3]));

      tradingdate_ = atoi(argv[4]);
      if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
        std::cerr << "tradingdate_ " << tradingdate_ << " out of range [ " << MIN_YYYYMMDD << " " << MAX_YYYYMMDD
                  << " ] " << std::endl;
        exit(1);
      }

      if (argc >= 6) {
        if (!strcmp(argv[5], "USEL1DATA")) {
          use_l1_data_ = true;
          if (argc >= 7) {
            if (strcmp(argv[6], "ADD_DBG_CODE") == 0) {
              for (int i = 7; i < argc; i++) {
                dbg_code_vec_.push_back(std::string(argv[i]));
              }
            } else {
              // expecting MARKETMODELINDEX
              market_model_index_ = std::max(0, atoi(argv[6]));

              if (argc >= 8) {
                if (strcmp(argv[7], "ADD_DBG_CODE") == 0) {
                  for (int i = 8; i < argc; i++) {
                    dbg_code_vec_.push_back(std::string(argv[i]));
                  }
                } else {
                  // expecting minutes to prep
                  ignore_user_msg_ = (strcmp(argv[7], "0") == 0);
                  // saci vec as csv - SACI1,SACI2,....
                  char* t_saci_ = strtok(argv[7], ",");
                  real_saci_vec_.push_back(atoi(t_saci_));
                  while ((t_saci_ = strtok(nullptr, ",")) != nullptr) {
                    real_saci_vec_.push_back(atoi(t_saci_));
                  }

                  if (argc >= 9) {
                    if (strcmp(argv[8], "ADD_DBG_CODE") == 0) {
                      for (int i = 9; i < argc; i++) {
                        dbg_code_vec_.push_back(std::string(argv[i]));
                      }
                    } else {
                      std::string start_time_ = argv[8];  // 1398858612.294373
                      int t_len_ = strlen(argv[8]);

                      if (t_len_ >= 10) {
                        t_strat_start_time_.tv_sec = atoi(start_time_.substr(0, 10).c_str());
                        t_strat_start_time_.tv_usec = 0;

                        if (t_len_ > 11 && argv[8][10] == '.') {
                          t_strat_start_time_.tv_usec = atoi(start_time_.substr(11, std::min(6, t_len_ - 11)).c_str());
                        }
                      }

                      if (argc >= 10) {
                        if (strcmp(argv[9], "ADD_DBG_CODE") == 0) {
                          for (int i = 10; i < argc; i++) {
                            dbg_code_vec_.push_back(std::string(argv[i]));
                          }
                        }

                        else {
                          t_use_fake_faster_data_ = atoi(argv[9]) > 0;

                          if (argc >= 11) {
                            if (strcmp(argv[10], "ADD_DBG_CODE") == 0) {
                              for (int i = 11; i < argc; i++) {
                                dbg_code_vec_.push_back(std::string(argv[i]));
                              }
                            } else {
                              if (strcmp(argv[10], "SAME_NETWORK_INFO") != 0) {
                                network_account_info_filename_ = argv[10];
                              }

                              if (argc >= 12) {
                                if (strcmp(argv[11], "ADD_DBG_CODE") == 0) {
                                  for (int i = 12; i < argc; i++) {
                                    dbg_code_vec_.push_back(std::string(argv[i]));
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        } else {
          if (strcmp(argv[5], "ADD_DBG_CODE") == 0) {
            for (int i = 6; i < argc; i++) {
              dbg_code_vec_.push_back(std::string(argv[i]));
            }
          } else {
            // expecting MARKETMODELINDEX
            market_model_index_ = std::max(0, atoi(argv[5]));

            if (argc >= 7) {
              if (strcmp(argv[6], "ADD_DBG_CODE") == 0) {
                for (int i = 7; i < argc; i++) {
                  dbg_code_vec_.push_back(std::string(argv[i]));
                }
              } else {
                // expecting minutes to prep
                ignore_user_msg_ = (strcmp(argv[6], "0") == 0);
                // saci vec as csv - SACI1,SACI2,....
                char* t_saci_ = strtok(argv[6], ",");
                real_saci_vec_.push_back(atoi(t_saci_));
                while ((t_saci_ = strtok(nullptr, ",")) != nullptr) {
                  real_saci_vec_.push_back(atoi(t_saci_));
                }

                if (argc >= 8) {
                  if (strcmp(argv[7], "ADD_DBG_CODE") == 0) {
                    for (int i = 8; i < argc; i++) {
                      dbg_code_vec_.push_back(std::string(argv[i]));
                    }
                  } else {
                    std::string start_time_ = argv[7];  // 1398858612.294373
                    int t_len_ = strlen(argv[7]);

                    if (t_len_ >= 10) {
                      t_strat_start_time_.tv_sec = atoi(start_time_.substr(0, 10).c_str());
                      t_strat_start_time_.tv_usec = 0;

                      if (t_len_ > 11 && argv[7][10] == '.') {
                        t_strat_start_time_.tv_usec = atoi(start_time_.substr(11, std::min(6, t_len_ - 11)).c_str());
                      }
                    }

                    if (argc >= 9) {
                      if (strcmp(argv[8], "ADD_DBG_CODE") == 0) {
                        for (int i = 9; i < argc; i++) {
                          dbg_code_vec_.push_back(std::string(argv[i]));
                        }
                      }

                      else {
                        t_use_fake_faster_data_ = atoi(argv[8]) > 0;

                        if (argc >= 10) {
                          if (strcmp(argv[9], "ADD_DBG_CODE") == 0) {
                            for (int i = 10; i < argc; i++) {
                              dbg_code_vec_.push_back(std::string(argv[i]));
                            }
                          } else {
                            if (strcmp(argv[9], "SAME_NETWORK_INFO") != 0) {
                              network_account_info_filename_ = argv[9];
                            }

                            if (argc >= 11) {
                              if (strcmp(argv[10], "ADD_DBG_CODE") == 0) {
                                for (int i = 11; i < argc; i++) {
                                  dbg_code_vec_.push_back(std::string(argv[i]));
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  global_livetrading_ = livetrading_;
  global_tradingdate_ = tradingdate_;
  global_strategy_desc_filename_ = strategy_desc_filename_;
  std::ifstream t(strategy_desc_filename_.c_str());
  global_strategy_desc_ = std::string((std::istreambuf_iterator<char>(t)),
                                      std::istreambuf_iterator<char>());  // to get teh contents of the strategy
  t.close();
  global_progid_ = progid_;
  global_market_model_index_ = market_model_index_;

  // TODO change command line parameters so that we can read all system paths from a file.
}

void InitTradesLogger(int tradingdate, int progid, bool livetrading, const std::string& logs_directory) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << logs_directory << "/trades." << tradingdate << "." << progid;
  std::string tradesfilename = t_temp_oss_.str();

  // open trades file in append mode in livetrading_
  trades_writer_.Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));
}

void InitDbglogger(int tradingdate, int progid, std::vector<std::string>& dbg_code_vec,
                   CommonSMVSource* common_smv_source, HFSAT::DebugLogger& dbglogger,
                   const std::string& logs_directory) {
  std::ostringstream t_temp_oss;
  t_temp_oss << logs_directory << "/log." << tradingdate << "." << progid;
  std::string logfilename = t_temp_oss.str();

  common_smv_source->SetDbgloggerFileName(logfilename);

  for (auto i = 0u; i < dbg_code_vec.size(); i++) {
    // TODO .. add ability to write "WATCH_INFO" instead of 110, and making it
    int dbg_code_to_be_logged = HFSAT::DebugLogger::TextToLogLevel(dbg_code_vec[i].c_str());
    if (dbg_code_to_be_logged <= 0) {
      dbglogger.SetNoLogs();
      break;
    } else {
      dbglogger.AddLogLevel(dbg_code_to_be_logged);
    }
  }

  if (dbg_code_vec.size() <= 0) {
    dbglogger.SetNoLogs();
  }

  // Though we do not exit here, but since it is a very very rare case and important to detect,
  // we are logging OM_ERROR and WATCH_ERROR for every default SIM run.
  dbglogger.AddLogLevel(WATCH_ERROR);
  dbglogger.AddLogLevel(OM_ERROR);
  dbglogger.AddLogLevel(PLSMM_ERROR);

  // dbglogger.AddLogLevel(TRADING_ERROR);
  // dbglogger.AddLogLevel(BOOK_ERROR);
  // dbglogger.AddLogLevel(LRDB_ERROR);
  dbglogger.AddLogLevel(DBG_MODEL_ERROR);
  //  dbglogger.AddLogLevel(SMVSELF_ERROR);
}

void GetDependentShortcodes() {}

void GetORSShortcodes() {}

void GetSourceShortcodes() {}

// Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
void CheckAndAddNSEBSEDefinitions(std::vector<std::string>& t_shortcode_vec_) {
  bool is_nse_present_ = false;
  bool is_bse_present_ = false;
  for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
    if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      is_nse_present_ = true;
    } else if (strncmp(t_shortcode_vec_[i].c_str(), "BSE_", 4) == 0) {
      is_bse_present_ = true;
    }
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadNSESecurityDefinitions();
  }
  if (is_bse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadBSESecurityDefinitions();
  }
}

/**
 * @brief Main Initialization
 *
 * parse command line paramters
 * process strategy_desc_file
 * build the list of source_shortcode_vec_ to initialize and dependant_shortcode_vec_ ( to make simtraders etc )
 * initialize sec_name_indexer
 * depending on historical study or real trading, initialize the Adapter ( with right input argument ) for each source
 * initialize the marketdata processing market_view_managers
 * if in live then setup the control_screen
 * looking at the strategy_desc_file, get the list of models and initialize them
 * for each model make the modelmath aggregator subscribe to all the market books of interest i.e for the securities
 of
 interest as a OnReadyListener ..
 * An OnReadyListener is updated after all the variables/simmarketmaker/ordermanager/execlogic... have been updated
 * An Onreadylistener is not updated with market data .. but with the notification that now every market data listener
 has the right picture of data.
 * Only on OnReadyListener does the aggregator send signal ahead

 * for each product traded make a simmarketmaker ( if sim ) or live order_routing_daemon ( if live )
 * for each line in the strategy_desc_file initialize :
 *     internal_order_routing ( bound to the appropriate simmarketmaker )
 *     strategy_code
 * setup the result reporting screen
 * start event loop
 */
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

  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");
  unsigned int market_model_index_ = 0u;
  unsigned int secs_to_prep_ = SECONDS_TO_PREP;
  bool ignore_user_msg_ = true;
  HFSAT::ttime_t strat_start_time_(0, 0);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  // HFSAT::SyntheticSecurityManager& synthetic_security_manager_ =
  // HFSAT::SyntheticSecurityManager::GetUniqueInstance();
  // HFSAT::HybridSecurityManager *hybrid_security_manager_ = new HFAST::HybridSecurityManager ( );

  std::vector<std::string> source_shortcode_vec_;  ///< vector of all sources which we need data for or are trading
  std::vector<std::string>
      ors_needed_by_indicators_vec_;  ///< vector of all sources which we need ORS messages for, to build indicators
  std::vector<std::string>
      dependant_shortcode_vec_;  ///< separate vector for the sources which we have to made simtraders etc of
  // bool using_non_self_market_view_ = false; ///< if using_non_self_market_view_ then ORS messages are listened to
  // by
  // SimMarketMaker, PromOrderMmanager ...

  std::vector<bool> sid_to_marketdata_needed_map_;  ///< security_id to bool map indicating whether market data is
  /// needed or not for this source ...

  std::vector<std::string> dbg_code_vec_;  ///< set of codes that we want the logger to print on
  std::vector<HFSAT::SyntheticMarketView*> synth_market_view_vec_;

#ifdef CCPROFILING
  // Cpu Cycle Profiling
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(5);
  cpucycle_profiler_.SetTag(0, "ProcessAllEvents to PLMVM ");
  cpucycle_profiler_.SetTag(1, " RF OnIndicatorUpdate 1 ");
  cpucycle_profiler_.SetTag(2, " RF OnIndicatorUpdate 2 ");
  cpucycle_profiler_.SetTag(3, " RF OnIndicatorUpdate 3 ");
#endif  // CCPROFILING

  bool use_fake_faster_data_ = true;
  std::vector<int> real_saci_vec_;
  bool use_l1_data_ = false;

  // This directory is one of the absolute paths that this program
  // assumes that it can write to.
  std::string logs_directory = "/spare/local/logs/tradelogs";

  // parse command line paramters
  ParseCommandLineParams(argc, argv, livetrading_, tradingdate_, strategy_desc_filename_, progid_,
                         network_account_info_filename_, market_model_index_, ignore_user_msg_, strat_start_time_,
                         use_fake_faster_data_, dbg_code_vec_, real_saci_vec_, use_l1_data_, logs_directory);

  // Make an object of CommonSMVSource and use it as an API
  std::vector<std::string> dummy_shc_list;
  CommonSMVSource* common_smv_source = new CommonSMVSource(dummy_shc_list, tradingdate_);

  // Get the dbglogger and watch after creating the source
  HFSAT::Watch& watch_ = common_smv_source->getWatch();
  HFSAT::DebugLogger& dbglogger_ = common_smv_source->getLogger();
  global_dbglogger_ = &dbglogger_;

  // Setup DebugLogger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec_, common_smv_source, dbglogger_, logs_directory);
  InitTradesLogger(tradingdate_, progid_, livetrading_, logs_directory);

  // Setup StrategyDesc
  HFSAT::StrategyDesc strategy_desc_(dbglogger_, strategy_desc_filename_, tradingdate_);

  // Get Traded Ezone
  std::string t_traded_ezone_ = "";
  if (!strategy_desc_.strategy_vec_.empty()) {
    t_traded_ezone_ = strategy_desc_.strategy_vec_[0].traded_ezone_;
  } else if (!strategy_desc_.structured_strategy_vec_.empty()) {
    t_traded_ezone_ = strategy_desc_.structured_strategy_vec_[0].traded_ezone_;
  }

  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, time_t(strategy_desc_.GetMinStartTime().tv_sec));
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_, t_traded_ezone_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::PcaWeightsManager::SetUniqueInstance(tradingdate_);

  // map from modelfilename to source shortcode vec to pass to setup ModelMath as listener to market data events
  std::map<std::string, std::vector<std::string> > modelfilename_source_shortcode_vec_map_;

  // map from modelfile name to source shorcode vec to pass to setup ModelMath as listener to ORS message events
  std::map<std::string, std::vector<std::string> > modelfilename_ors_needed_by_indicators_vec_map_;

  std::string offline_mix_mms_wts_filename_ = "INVALIDFILE";
  std::string online_mix_price_consts_filename_ = "INVALIDFILE";
  std::string online_beta_kalman_consts_filename_ = "INVALIDFILE";

  SimStrategyHelper* sim_strategy_helper_ = new SimStrategyHelper();

  std::map<int, vector<std::string> > struct_strat_source_shortcode_vec_map_;
  std::map<int, vector<std::string> > strat_source_shortcode_vec_map_;

  // build list of source shortcode vec and dependant shortcode vec to be needed by common smv source and SMM
  // for structured strategies
  if (strategy_desc_.structured_strategy_vec_.size() > 0) {
    sim_strategy_helper_->SetShortcodeVectorsForStructuredStrategies(
        strategy_desc_, dbglogger_, watch_, source_shortcode_vec_, ors_needed_by_indicators_vec_,
        dependant_shortcode_vec_, offline_mix_mms_wts_filename_, online_mix_price_consts_filename_,
        online_beta_kalman_consts_filename_, modelfilename_ors_needed_by_indicators_vec_map_,
        modelfilename_source_shortcode_vec_map_, tradingdate_, struct_strat_source_shortcode_vec_map_);
  }

  // build list of source shortcode vec and dependant shortcode vec to be needed by common smv source and SMM
  // for normal strategies
  if (strategy_desc_.strategy_vec_.size() > 0) {
    sim_strategy_helper_->SetShortcodeVectorsForStrategies(
        strategy_desc_, dbglogger_, watch_, source_shortcode_vec_, ors_needed_by_indicators_vec_,
        dependant_shortcode_vec_, offline_mix_mms_wts_filename_, online_mix_price_consts_filename_,
        online_beta_kalman_consts_filename_, modelfilename_ors_needed_by_indicators_vec_map_,
        modelfilename_source_shortcode_vec_map_, tradingdate_, strat_source_shortcode_vec_map_);
  }

  std::vector<std::string> portfolio_source_shortcode_vec_;
  std::vector<std::string> dummy_list_;
  if (strategy_desc_.is_portfolio_trading_strategy_) {
    for (auto i = 0u; i < strategy_desc_.portfolio_strategy_vec_.size(); i++) {
      if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "MeanRevertingTrading") {
	HFSAT::MeanRevertingTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
							  source_shortcode_vec_, ors_needed_by_indicators_vec_);
	HFSAT::MeanRevertingTrading::CollectTradingShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
							      dependant_shortcode_vec_);
	HFSAT::MeanRevertingTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
							  portfolio_source_shortcode_vec_, dummy_list_);
      } else if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "IndexFuturesMeanRevertingTrading") {
	HFSAT::IndexFuturesMeanRevertingTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
								      source_shortcode_vec_, ors_needed_by_indicators_vec_);
	HFSAT::IndexFuturesMeanRevertingTrading::CollectTradingShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
									  dependant_shortcode_vec_);
	HFSAT::IndexFuturesMeanRevertingTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
								      portfolio_source_shortcode_vec_, dummy_list_);
      } else if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "OptionsMeanRevertingTrading") {
	// this is needed for dynamic option_shortcodes logic
	HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadNSESecurityDefinitions();
	// here we are making sure to add futures and options
	// we need options data when we are placing orders and for fair_impliedvol signal
	// we need futures data to compute futures_fair_price
	HFSAT::OptionsMeanRevertingTrading::CollectSourceShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
								    source_shortcode_vec_, ors_needed_by_indicators_vec_);
	// we are not trading futures, hence depedanats only has options
	HFSAT::OptionsMeanRevertingTrading::CollectTradingShortCodes(dbglogger_, strategy_desc_.portfolio_strategy_vec_[i].prod_filename_,
								     dependant_shortcode_vec_);
      } else if (strategy_desc_.portfolio_strategy_vec_[i].strategy_type_ == "BasePortTrading") {
	std::vector<HFSAT::PortExecParamSet*> t_paramset_vec_;
      // just so I clear confusion ( about naming )
	HFSAT::PortfolioTradingInterface::CollectPortExecParamSetVec(t_paramset_vec_, strategy_desc_.portfolio_strategy_vec_[i].global_paramfilename_);
	for (auto t_paramset_ : t_paramset_vec_) {
	  // ridiculous all we need was three vectors ( source / ors-needed sources / trading )
	  // but I am not sure what are other two for !! going by the bad flow
	  source_shortcode_vec_.push_back(t_paramset_->instrument_);
	  ors_needed_by_indicators_vec_.push_back(t_paramset_->instrument_);
	  dependant_shortcode_vec_.push_back(t_paramset_->instrument_);
	  portfolio_source_shortcode_vec_.push_back(t_paramset_->instrument_);
	  dummy_list_.push_back(t_paramset_->instrument_);
	}
      }
    }
  }

  // By this time, source_shortcode_vec, ors_needed etc. should have been populated

  // Set all the parameters in the common_smv_source
  common_smv_source->SetSourceShortcodes(source_shortcode_vec_);
  common_smv_source->SetSourcesNeedingOrs(ors_needed_by_indicators_vec_);
  common_smv_source->SetDepShortcodeVector(dependant_shortcode_vec_);
  common_smv_source->SetDepShortcode(dependant_shortcode_vec_[0]);
  common_smv_source->SetOfflineMixMMSFilename(offline_mix_mms_wts_filename_);
  common_smv_source->SetOnlineMixPriceFilename(online_mix_price_consts_filename_);
  common_smv_source->SetOnlineBetaKalmanFileName(online_beta_kalman_consts_filename_);
  common_smv_source->SetFakeFasterData(use_fake_faster_data_);
  common_smv_source->SetIgnoreUserMsg(ignore_user_msg_);
  common_smv_source->SetSimSmvRequired(true);
  common_smv_source->SetStrategyDesc(strategy_desc_.simconfig_filename_);
  common_smv_source->SetNSEL1Mode(use_l1_data_);
  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_ = common_smv_source->getMOVMap();
  sid_to_mov_ptr_map_.resize(sec_name_indexer_.NumSecurityId(), nullptr);
  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = common_smv_source->getSMVMap();
  HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_ = common_smv_source->getSimSMVMap();
  HFSAT::SimTimeSeriesInfo& sim_time_series_info_ = common_smv_source->getSimTimeSeriesInfo();
  HFSAT::NetworkAccountInfoManager& network_account_info_manager_ = common_smv_source->getNetworkAccountInfoManager();
  HFSAT::TradingLocation_t dep_trading_location_ = common_smv_source->getDepTradingLocation();
  HFSAT::HistoricalDispatcher& historical_dispatcher_ = common_smv_source->getHistoricalDispatcher();
  HFSAT::PromOrderManagerPtrVec& sid_to_prom_order_manager_map_ = HFSAT::sid_to_prom_order_manager_map();
  std::vector<bool>& sid_to_ors_needed_map_ = common_smv_source->getSidORSNeededMap();

  // get shorcode smv map
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  // get shortcode ors data filesource map
  HFSAT::ShortcodeORSMessageFilesourceMap& shortcode_ors_data_filesource_map_ =
      HFSAT::ShortcodeORSMessageFilesourceMap::GetUniqueInstance();

  // get alphaflash data filesource
  HFSAT::AFLASHLoggedMessageFileSource* aflash_data_filesource_ = nullptr;

  // setup shortcode to SMM map
  std::map<std::string, HFSAT::BaseSimMarketMaker*> shortcode_to_smm_map;

  // Note that since market update manager is being initialized before the indicators
  // the PCAPortPrice::OnMarketDataResumed will be called before the OnMarketUpdate and
  // hence the security_id_last_price_map_ will not be uptodate when OnPortfolioPriceReset is called
  HFSAT::MarketUpdateManager& market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, tradingdate_));

  HFSAT::MulticastSenderSocket* p_strategy_param_sender_socket_ = nullptr;

  HFSAT::RiskManager* p_risk_manager_ = nullptr;

  // These 3 variables will be needed later is a RiskManagewr::GetUniqueInstance needs to be called.
  const std::string& first_dep_shortcode_ =
      dependant_shortcode_vec_[0];  // should be same as strategy_desc_.strategy_vec_[i].dep_shortcode_ ;
  const int first_dep_security_id_ = sec_name_indexer_.GetIdFromString(first_dep_shortcode_);

  // if in live then setup the control_screen
  // looking at the strategy_desc_file, get the list of models and initialize them
  // for each product traded make a simmarketmaker ( if sim ) or live order_routing_daemon ( if live )
  // for each line in the strategy_desc_file initialize :
  //     internal_order_routing ( bound to the appropriate SimMarketMaker )
  //     strategy_code
  std::map<std::string, HFSAT::RETAILLoggedMessageFileSource*> retail_logged_message_filesource_map_;
  std::map<std::string, HFSAT::RETAILLoggedMessageFileSource*> retail_manual_logged_message_filesource_map_;

  HFSAT::SpreadTradingManager* spread_trading_manager_ = nullptr;

  if (strategy_desc_.structured_strategy_vec_.size() > 0) {
    sim_strategy_helper_->SetDepComponentsStructuredStrategies(
        dbglogger_, watch_, strategy_desc_, network_account_info_manager_, historical_dispatcher_, shortcode_to_smm_map,
        sid_to_mov_ptr_map_, common_smv_source, economic_events_manager_, livetrading_, market_model_index_,
        sim_time_series_info_, p_strategy_param_sender_socket_, market_update_manager_, sid_to_smv_ptr_map_,
        sid_to_sim_smv_ptr_map_, shortcode_smv_map_, shortcode_ors_data_filesource_map_,
        retail_logged_message_filesource_map_, retail_manual_logged_message_filesource_map_, dep_trading_location_,
        ignore_user_msg_, tradingdate_, modelfilename_source_shortcode_vec_map_, real_saci_vec_,
        struct_strat_source_shortcode_vec_map_, sec_name_indexer_, trades_writer_);
  }

  if (strategy_desc_.portfolio_strategy_vec_.size() > 0) {
    sim_strategy_helper_->SetDepComponentsPortfolioStrategies(
        dbglogger_, watch_, strategy_desc_, network_account_info_manager_, historical_dispatcher_, shortcode_to_smm_map,
        sid_to_mov_ptr_map_, common_smv_source, livetrading_, market_model_index_, sim_time_series_info_,
        sid_to_sim_smv_ptr_map_, shortcode_smv_map_, shortcode_ors_data_filesource_map_, dep_trading_location_,
        ignore_user_msg_, tradingdate_, sid_to_prom_order_manager_map_, real_saci_vec_, dependant_shortcode_vec_,
        sec_name_indexer_, trades_writer_, portfolio_source_shortcode_vec_, economic_events_manager_);
  }

  if (strategy_desc_.strategy_vec_.size() > 0) {
    sim_strategy_helper_->SetDepComponentsStrategies(
        dbglogger_, watch_, strategy_desc_, network_account_info_manager_, historical_dispatcher_, shortcode_to_smm_map,
        sid_to_mov_ptr_map_, common_smv_source, economic_events_manager_, livetrading_, market_model_index_,
        sim_time_series_info_, p_strategy_param_sender_socket_, market_update_manager_, sid_to_smv_ptr_map_,
        sid_to_sim_smv_ptr_map_, shortcode_smv_map_, shortcode_ors_data_filesource_map_,
        retail_logged_message_filesource_map_, retail_manual_logged_message_filesource_map_, dep_trading_location_,
        ignore_user_msg_, tradingdate_, sid_to_prom_order_manager_map_, p_risk_manager_,
        modelfilename_source_shortcode_vec_map_, first_dep_security_id_, first_dep_shortcode_, real_saci_vec_,
        strat_source_shortcode_vec_map_, sec_name_indexer_, aflash_data_filesource_, trades_writer_, all_om_vec);
  }

  if (!strategy_desc_.is_portfolio_trading_strategy_) {
    HFSAT::ModelCreator::CreateIndicatorInstances(dbglogger_, watch_);
  }

  // PromOrderManager is added after ( in SIM : SimMM and ) SmartOM
  for (unsigned int t_sid_ = 0; t_sid_ < sec_name_indexer_.NumSecurityId(); t_sid_++) {
    std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_sid_);

    if (sid_to_ors_needed_map_[t_sid_]) {
      // (i) for the sources on which ORS data is needed for indicators and hence promordermanager needs to listen to
      // ors file / livesource
      // (ii) dependant security since we should use it to see if we are collectively hitting any margin limits )
      // (iii) all sources where we need market data if using_non_self_market_view_ = true

      // if PromOrderManager setup for ths security ... as should be ... then attach it as listener to ORS source
      if (sid_to_prom_order_manager_map_[t_sid_] != nullptr) {
        HFSAT::ORSMessageFileSource* p_ors_message_filesource_ =
            shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_);
        if (p_ors_message_filesource_ != nullptr) {  // When using non-self for all products , we might reach here for
                                                     // products we do not create ors_data_sources for.

          // p_ors_message_filesource_->AddOrderNotFoundListener ( sid_to_prom_order_manager_map_ [ t_sid_ ] ); //
          // PromOrderManager does not need to listen to reply of NotFound from client replay requests ?
          p_ors_message_filesource_->AddOrderSequencedListener(sid_to_prom_order_manager_map_[t_sid_]);
          p_ors_message_filesource_->AddOrderConfirmedListener(sid_to_prom_order_manager_map_[t_sid_]);
          p_ors_message_filesource_->AddOrderConfCxlReplacedListener(sid_to_prom_order_manager_map_[t_sid_]);
          p_ors_message_filesource_->AddOrderConfCxlReplaceRejectListener(sid_to_prom_order_manager_map_[t_sid_]);
          p_ors_message_filesource_->AddOrderCanceledListener(sid_to_prom_order_manager_map_[t_sid_]);
          p_ors_message_filesource_->AddOrderExecutedListener(sid_to_prom_order_manager_map_[t_sid_]);
          // p_ors_message_filesource_->AddOrderRejectedListener ( sid_to_prom_order_manager_map_ [ t_sid_ ] ); //
          // PromOrderManager does not need to listen to reply of Reject to client sendtrade messages which fail
        }
      }
    }
  }

  if (!strategy_desc_.is_portfolio_trading_strategy_) {
    // link up ModelMath to SMV and PromOrderManager . Doing this at the end so that everything is uptodate by the
    // time
    // target_price_ is sent to strategy
    std::vector<HFSAT::BaseModelMath*> base_model_math_vec_;
    HFSAT::ModelCreator::GetModelMathVec(base_model_math_vec_);
    // for all the model_math objects
    for (auto i = 0u; i < base_model_math_vec_.size(); i++) {
      // for the securities that are ORS sources for indicators in this model attach the base_model_math_ as a
      // listener
      // to
      // the prom_order_manager of that sid_
      std::vector<std::string>& shortcodes_affecting_this_model_ =
          modelfilename_source_shortcode_vec_map_[base_model_math_vec_[i]->model_filename()];
      std::vector<std::string>& ors_source_needed_vec_ =
          modelfilename_ors_needed_by_indicators_vec_map_[base_model_math_vec_[i]->model_filename()];
      HFSAT::ModelCreator::LinkupModelMathToOnReadySources(base_model_math_vec_[i], shortcodes_affecting_this_model_,
                                                           ors_source_needed_vec_);
    }
    market_update_manager_.start();
  }

#define _USE_HISTORICAL_PRICE_PCAPORT_ false
#if _USE_HISTORICAL_PRICE_PCAPORT_
  dbglogger_.AddLogLevel(HPM_INFO);
  dbglogger_.AddLogLevel(HPM_ERROR);
  HFSAT::HistoricPriceManager::SetUniqueInstance(&dbglogger_, &sec_name_indexer_, tradingdate_);
#endif

#define _USE_PRICE_VOL_MANAGER_ false
#if _USE_PRICE_VOL_MANAGER_
  dbglogger_.AddLogLevel(PVM_ERROR);
  dbglogger_.AddLogLevel(PVM_INFO);

  HFSAT::PriceVolatilityManager* p_price_vol_manager_ = HFSAT::PriceVolatilityManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, tradingdate_);

  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); ++i) {
    if (sid_is_dependant_map_[i]) {
      // p_price_vol_manager_ -> AddPriceVolatilityEventListener ( i , );
    }
  }

  p_price_vol_manager_->EnablePriceManager();
#endif

  // To only process data starting MINUTES_TO_PREP minutes before min start time of strategies
  // In all the file sources, read events till
  // we reach the first event after the specified ttime_t

  HFSAT::ttime_t min_strat_trading_time_ = strategy_desc_.GetMinStartTime();
  HFSAT::ttime_t data_seek_time_ = min_strat_trading_time_ - HFSAT::ttime_t((int)secs_to_prep_, 0);
  if (strategy_desc_.is_portfolio_trading_strategy_)
    data_seek_time_ = min_strat_trading_time_ -
                      HFSAT::ttime_t(2100, 0);  // for portfolio strats we want the strategy to start 45 mins before
                                                // Make sure cron job starts 35 mins before strategy start.
  // This is because we have 30 mins of vector data required for computing errors, we want to be safe and have no
  // overlap with yesterday

  if ((!(strat_start_time_ == HFSAT::ttime_t(0, 0))) && strat_start_time_ <= min_strat_trading_time_) {
    // give input is sane , i.e. before trading time
    data_seek_time_ = strat_start_time_;
  }

  // To avoid complications we gotta start the data on the same day which we intend to trade on.
  if (min_strat_trading_time_ >= HFSAT::ttime_t(watch_.last_midnight_sec(), 0) &&
      data_seek_time_ < HFSAT::ttime_t(watch_.last_midnight_sec(), 0)) {
    data_seek_time_ = HFSAT::ttime_t(watch_.last_midnight_sec(), 0);
  }

  // Seek to start time
  common_smv_source->Seek(data_seek_time_);

  HFSAT::ttime_t end_time = strategy_desc_.GetMaxEndTime() + HFSAT::ttime_t(3600, 0);

  // for DI1* we don't want to trade beyond BRT_1600, because it enters into pre-open/close, gap was creating problems
  // in final pnl

  for (auto i = 0u; i < dependant_shortcode_vec_.size(); i++) {
    if (strncmp(dependant_shortcode_vec_[i].c_str(), "DI1", strlen("DI1")) == 0) {
      end_time = strategy_desc_.GetMaxEndTime() + HFSAT::ttime_t(7200, 0);
      break;
    }
  }

  // Run event loop
  common_smv_source->Run(end_time);

  // TODO check for strategies with open positions and report the same
  // setup the result reporting screen

  for (auto i = 0u; i < strategy_desc_.structured_strategy_vec_.size(); i++) {
    if (HFSAT::CurveUtils::IsSpreadStrategy(strategy_desc_.structured_strategy_vec_[i].strategy_name_)) {
      spread_trading_manager_->ReportResults(trades_writer_);
    } else if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(
                   HFSAT::RiskBasedStructuredTrading::StrategyName()) == 0 ||
               HFSAT::StructuredGeneralTrading::IsStructuredGeneralTrading(
                   strategy_desc_.structured_strategy_vec_[i].strategy_name_)) {
      strategy_desc_.structured_strategy_vec_[i].curve_trading_manager_->ReportResults(trades_writer_);
    } else if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(
                   HFSAT::MinRiskPriceBasedAggressiveTrading::StrategyName()) == 0) {
      strategy_desc_.structured_strategy_vec_[i].min_risk_trading_manager_->ReportResults(trades_writer_);
    } else if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(
                   HFSAT::EquityTrading2::StrategyName()) == 0) {
      if (strategy_desc_.structured_strategy_vec_[i].equity_trading_) {
        strategy_desc_.structured_strategy_vec_[i].equity_trading_->ReportResults(trades_writer_);
      }
    } else if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(HFSAT::PairsTrading::StrategyName()) ==
               0) {
      if (strategy_desc_.structured_strategy_vec_[i].pairs_trading_) {
        strategy_desc_.structured_strategy_vec_[i].pairs_trading_->ReportResults(trades_writer_);
      }
    } else if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(
                   HFSAT::RetailFlyTradingManager::StrategyName()) == 0) {
      strategy_desc_.structured_strategy_vec_[i].common_trading_manager_->ReportResults(trades_writer_);
    } else if (strategy_desc_.structured_strategy_vec_[i].strategy_name_.compare(HFSAT::BaseOTrading::StrategyName()) ==
               0) {
      strategy_desc_.structured_strategy_vec_[i].options_trading_->ReportResults(trades_writer_);
    } else {
      strategy_desc_.structured_strategy_vec_[i].trading_manager_->ReportResults(trades_writer_);
    }
  }

  for (auto i = 0u; i < strategy_desc_.strategy_vec_.size(); i++) {
    if (strategy_desc_.strategy_vec_[i].strategy_name_.compare("NikPricePairBasedAggressiveTrading") == 0) {
      strategy_desc_.strategy_vec_[i].trading_manager_->ReportResults(trades_writer_);
    } else if (strategy_desc_.strategy_vec_[i].exec_ != nullptr) {
      // const unsigned int t_sec_id_ = sec_name_indexer_.GetIdFromString (
      // strategy_desc_.strategy_vec_[i].dep_shortcode_ ) ;
      strategy_desc_.strategy_vec_[i].exec_->ReportResults(trades_writer_, true);
    }
  }

  if (strategy_desc_.is_portfolio_trading_strategy_) {
    for (auto i = 0u; i < strategy_desc_.portfolio_strategy_vec_.size(); i++) {
      if (strategy_desc_.portfolio_strategy_vec_[i].port_interface_ != nullptr) {
	strategy_desc_.portfolio_strategy_vec_[i].port_interface_->ReportResults(trades_writer_, true);
      } else {
	strategy_desc_.portfolio_strategy_vec_[i].exec_->ReportResults(trades_writer_, true);
      }
    }
  }

  for (auto i = 0u; i < synth_market_view_vec_.size(); i++) {
    if (synth_market_view_vec_[i] != nullptr) {
      delete (synth_market_view_vec_[i]);
    }
  }

#ifdef CCPROFILING

  std::cout << "Printing the results of CPU Profiler " << std::endl;
  const std::vector<HFSAT::CpucycleProfilerSummaryStruct> prof_summary =
      HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummary();

  for (unsigned int ii = 0; ii < prof_summary.size(); ii++) {
    if (prof_summary[ii].total_occurrence_ > 0) {
      std::cout << prof_summary[ii].tag_name_ << " " << prof_summary[ii].fifty_percentile_ << " "
                << ((double)prof_summary[ii].ninetyfive_percentile_ / (double)prof_summary[ii].fifty_percentile_) << ' '
                << prof_summary[ii].total_occurrence_ << std::endl;

      // std::cout << prof_summary[ii].tag_name_
      // 	    << " Mean: " << prof_summary[ii].mean_
      // 	    << " Median: " << prof_summary[ii].fifty_percentile_
      // 	    << " Ninty: " << prof_summary[ii].ninety_percentile_
      // 	    << " Ninty/fifty: " << ( ( double ) prof_summary[ii].ninetyfive_percentile_ / ( double )
      // prof_summary[ii].fifty_percentile_ )
      // 	    << " Min: " << prof_summary[ii].min_
      // 	    << " Max: " << prof_summary[ii].max_
      // 	    << " Count: " << prof_summary[ii].total_occurrence_ << std::endl ;
    }
  }

#endif  // CCPROFILING
  for (unsigned i = 0; i < all_om_vec.size(); i++) {
    all_om_vec[i]->PrintCancelSeqdExecTimes();
  }

  if (dbglogger_.IsNoLogs()) {
    remove(dbglogger_.GetLogFileName().c_str());
  }
  return 0;
}
