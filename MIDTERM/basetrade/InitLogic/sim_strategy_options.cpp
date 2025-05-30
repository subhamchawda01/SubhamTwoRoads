/**
   \file InitLogic/sim_strategy.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "basetrade/InitLogic/sim_strategy_options.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"

//#define CCPROFILING 0

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225
#define SECONDS_TO_PREP 1800

HFSAT::OptionsRiskManager* options_risk_manager_;
std::string position_file_;
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
void termination_handler(int signum) {
  // TODO check for strategies with open positions and print the same
  std::vector<std::vector<HFSAT::OptionVars*> > options_data_matrix_ = options_risk_manager_->options_data_matrix_;
  std::vector<HFSAT::OptionVars*> underlying_data_vec_ = options_risk_manager_->underlying_data_vec_;

  std::cout << "Exiting due to signal " << SimpleSignalString(signum) << std::endl;
  char hostname_[128];
  hostname_[127] = '\0';
  gethostname(hostname_, 127);

  std::string position_email_string_ = "";
  std::ostringstream t_oss_;

  t_oss_ << "Options Strategy: " << options_risk_manager_->runtime_id_
         << " exiting signal: " << SimpleSignalString(signum) << " on " << hostname_ << "\n";

  std::string subject_email_string_ = t_oss_.str();
  position_email_string_ = position_email_string_ + t_oss_.str();

  for (auto i = 0u; i < underlying_data_vec_.size(); i++) {
    std::ostringstream t_oss_1;
    if (underlying_data_vec_[i]->position_ != 0) {
      t_oss_1 << "SHC: " << underlying_data_vec_[i]->smv_->shortcode()
              << " SECNAME: " << underlying_data_vec_[i]->smv_->secname()
              << " POS: " << underlying_data_vec_[i]->position_
              << " LAST_TRADED_PRICE: " << underlying_data_vec_[i]->last_traded_price_ << "\n";
      position_email_string_ = position_email_string_ + t_oss_1.str();
    }

    for (unsigned int j = 0; j < options_data_matrix_[i].size(); j++) {
      if (options_data_matrix_[i][j]->position_ != 0) {
        std::ostringstream t_oss_2;
        t_oss_2 << "SHC: " << options_data_matrix_[i][j]->smv_->shortcode()
                << " SECNAME: " << options_data_matrix_[i][j]->smv_->secname()
                << " POS: " << options_data_matrix_[i][j]->position_
                << " LAST_TRADED_PRICE: " << options_data_matrix_[i][j]->last_traded_price_ << "\n";

        position_email_string_ = position_email_string_ + t_oss_2.str();
      }
    }
  }

  if (position_email_string_ != subject_email_string_) {
    HFSAT::Email email_;
    email_.setSubject(subject_email_string_);
    email_.addRecepient("uttkarsh.sarraf@tworoads.co.in");
    email_.addSender("nseall@tworoads.co.in");
    email_.content_stream << position_email_string_ << "<br/>";
    email_.sendMail();
  }

  global_dbglogger_->Close();
  std::ofstream out(position_file_);
  out << position_email_string_;
  out.close();
  exit(0);
}

void ParseCommandLineParams(int argc, char** argv, bool& livetrading_, int& tradingdate_,
                            std::string& strategy_desc_filename_, unsigned int& progid_,
                            HFSAT::ttime_t& t_strat_start_time_, std::vector<std::string>& dbg_code_vec_,
                            bool& use_l1_data_, bool& ignore_user_msg_) {
  if (argc < 4) {  // 4 is min of live and sim
    std::cerr << "expecting :\n"
              << " 1. sim_strategy_options SIM STRATEGYDESCFILENAME PROGID TRADINGDATE [ENABLE_USER_MSG START_TIME]"
              << '\n';

    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  } else {
    strategy_desc_filename_ = argv[2];
    progid_ = std::max(0, atoi(argv[3]));
    tradingdate_ = atoi(argv[4]);

    if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
      std::cerr << "tradingdate_ " << tradingdate_ << " out of range [ " << MIN_YYYYMMDD << " " << MAX_YYYYMMDD << " ] "
                << std::endl;
      exit(1);
    }
    int cur_idx = 5;

    if (argc > 6 && (strcmp(argv[cur_idx], "USEL1DATA") != 0) && (strcmp(argv[cur_idx], "ADD_DBG_CODE") != 0)) {
      ignore_user_msg_ = (atoi(argv[5]) == 0);
      // Process start time
      std::string start_time_ = argv[6];  // 1398858612.294373
      int t_len_ = strlen(argv[6]);
      if (t_len_ >= 10) {
        t_strat_start_time_.tv_sec = atoi(start_time_.substr(0, 10).c_str());
        t_strat_start_time_.tv_usec = 0;
        if (t_len_ > 11 && argv[6][10] == '.') {
          t_strat_start_time_.tv_usec = atoi(start_time_.substr(11, std::min(6, t_len_ - 11)).c_str());
        }
      }
      cur_idx += 2;
    }

    if (argc > cur_idx) {
      if (strcmp(argv[cur_idx], "USEL1DATA") == 0) {
        use_l1_data_ = true;
      } else if (strcmp(argv[cur_idx], "ADD_DBG_CODE") == 0) {
        for (int i = cur_idx + 1; i < argc; i++) {
          dbg_code_vec_.push_back(std::string(argv[i]));
        }
      }
    }
  }

  global_livetrading_ = livetrading_;
  global_tradingdate_ = tradingdate_;
  global_strategy_desc_filename_ = strategy_desc_filename_;
  std::ifstream t(strategy_desc_filename_.c_str());
  global_strategy_desc_ = std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  t.close();
  global_progid_ = progid_;
}

void InitTradesLogger(int tradingdate, int progid, bool livetrading) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/tradelogs/trades." << tradingdate << "." << progid;
  std::string tradesfilename = t_temp_oss_.str();
  trades_writer_.Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));
}

void InitDbglogger(int tradingdate, int progid, std::vector<std::string>& dbg_code_vec,
                   CommonSMVSource* common_smv_source, HFSAT::DebugLogger& dbglogger) {
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/logs/tradelogs/log." << tradingdate << "." << progid;
  std::string logfilename = t_temp_oss.str();

  common_smv_source->SetDbgloggerFileName(logfilename);

  for (auto i = 0u; i < dbg_code_vec.size(); i++) {
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

HFSAT::BaseTrader* GetBaseTrader(HFSAT::NetworkAccountInfoManager& network_account_manager,
                                 HFSAT::SecurityMarketView* dep_smv, HFSAT::BaseSimMarketMaker* smm) {
  assert(dep_smv != NULL && smm != NULL);

  std::string trade_info = network_account_manager.GetDepTradeAccount(dep_smv->exch_source(), dep_smv->shortcode());
  return HFSAT::SimTraderHelper::GetSimTrader(trade_info, smm);
}

HFSAT::BaseSimMarketMaker* GetSMM(HFSAT::Watch& watch_, HFSAT::SecurityMarketView* dep_smv,
                                  HFSAT::SecurityMarketView* sim_smv, unsigned int market_model_index,
                                  HFSAT::SimTimeSeriesInfo& sim_time_series_info,
                                  HFSAT::HistoricalDispatcher& historical_dispatcher, HFSAT::DebugLogger& dbglogger_) {
  if (dep_smv == NULL || sim_smv == NULL || dep_smv->exch_source() != sim_smv->exch_source() ||
      dep_smv->security_id() != sim_smv->security_id()) {
    return NULL;
  }

  sim_smv = dep_smv;
  std::string shortcode = dep_smv->shortcode();
  // HFSAT::SecurityNameIndexer& sec_name_indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  switch (dep_smv->exch_source()) {
    case HFSAT::kExchSourceNSE: {
      HFSAT::PriceLevelSimMarketMaker* plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
          dbglogger_, watch_, *sim_smv, market_model_index, sim_time_series_info);
      plsmm->SubscribeL2Events(*dep_smv);
      return plsmm;
      break;
    }
    default:
      break;
  }
  return NULL;
}

// Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
//    HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadNSESecurityDefinitions();
//    HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadBSESecurityDefinitions();

void ReadStrategyFile(HFSAT::DebugLogger& t_dbglogger_, const std::string& _strategy_desc_filename_,
                      const int _tradingdate_, std::string& _strategy_name_, std::string& _model_filename_,
                      std::string& _adapter_type_, int& _runtime_id_, int& _t_start_mfm_, int& _t_end_mfm_,
                      HFSAT::ttime_t& _t_start_time_, HFSAT::ttime_t& _t_end_time_, unsigned int& t_market_model_index_,
                      bool& t_use_accurate_seq2conf_) {
  std::ifstream strategy_desc_file_;
  strategy_desc_file_.open(_strategy_desc_filename_.c_str(), std::ifstream::in);

  if (strategy_desc_file_.is_open()) {
    const int kStrategyDescFileLineBufferLen = 1024;
    char readline_buffer_[kStrategyDescFileLineBufferLen];
    bzero(readline_buffer_, kStrategyDescFileLineBufferLen);

    while (strategy_desc_file_.good()) {
      bzero(readline_buffer_, kStrategyDescFileLineBufferLen);
      strategy_desc_file_.getline(readline_buffer_, kStrategyDescFileLineBufferLen);
      t_dbglogger_ << readline_buffer_ << '\n';
      t_dbglogger_.CheckToFlushBuffer();  // added logging to see later what was running

      std::string this_strategy_full_line_(readline_buffer_);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, kStrategyDescFileLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 1) break;

      _strategy_name_ = tokens_[1];
      _model_filename_ = tokens_[2];
      _adapter_type_ = tokens_[3];
      _runtime_id_ = atoi(tokens_[4]);

      const char* _tz_hhmm_str_start_ = tokens_[5];
      _t_start_time_ = HFSAT::ttime_t(
          HFSAT::DateTime::GetTimeFromTZHHMMSS(_tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_start_ + 4),
                                               _tz_hhmm_str_start_),
          0);
      _t_start_mfm_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
          _tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_start_ + 4), _tz_hhmm_str_start_));

      const char* _tz_hhmm_str_end_ = tokens_[6];
      _t_end_time_ =
          HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(
                             _tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_end_ + 4), _tz_hhmm_str_end_),
                         0);
      _t_end_mfm_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
          _tradingdate_, HFSAT::DateTime::GetHHMMSSTime(_tz_hhmm_str_end_ + 4), _tz_hhmm_str_end_));

      if (tokens_.size() > 7) {
        t_market_model_index_ = atoi(tokens_[7]);
      }

      if (tokens_.size() > 8) {
        t_use_accurate_seq2conf_ = (atoi(tokens_[8]) > 0 ? true : false);
      }
    }
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
 * for each model make the modelmath aggregator subscribe to all the market books of interest i.e for the securities of
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
  bool use_accurate_seq2conf_ = false;
  unsigned int secs_to_prep_ = SECONDS_TO_PREP;
  bool ignore_user_msg_ = true;
  HFSAT::ttime_t strat_start_time_(0, 0);
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  std::vector<std::string> source_shortcode_vec_;  ///< vector of all sources which we need data for or are trading
  std::vector<std::string> ors_needed_by_indicators_vec_;
  std::vector<std::string> dependant_shortcode_vec_;
  std::vector<std::string> dbg_code_vec_;

#ifdef CCPROFILING
  // Cpu Cycle Profiling
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(5);
  cpucycle_profiler_.SetTag(0, "ProcessAllEvents to PLMVM ");
  cpucycle_profiler_.SetTag(1, " RF OnIndicatorUpdate 1 ");
  cpucycle_profiler_.SetTag(2, " RF OnIndicatorUpdate 2 ");
  cpucycle_profiler_.SetTag(3, " RF OnIndicatorUpdate 3 ");
#endif  // CCPROFILING

  bool use_fake_faster_data_ = false;
  bool use_l1_data_ = false;

  // parse command line paramters
  ParseCommandLineParams(argc, argv, livetrading_, tradingdate_, strategy_desc_filename_, progid_, strat_start_time_,
                         dbg_code_vec_, use_l1_data_, ignore_user_msg_);

  // Make an object of CommonSMVSource and use it as an API
  std::vector<std::string> dummy_shc_list;
  CommonSMVSource* common_smv_source = new CommonSMVSource(dummy_shc_list, tradingdate_);

  // Get the dbglogger and watch after creating the source
  HFSAT::Watch& watch_ = common_smv_source->getWatch();
  HFSAT::DebugLogger& dbglogger_ = common_smv_source->getLogger();
  global_dbglogger_ = &dbglogger_;

  // Setup DebugLogger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec_, common_smv_source, dbglogger_);
  InitTradesLogger(tradingdate_, progid_, livetrading_);

  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/tradelogs/position." << tradingdate_ << "." << progid_;
    position_file_ = t_temp_oss_.str();
  }

  // Read the strategy file
  std::string strategy_name_;
  std::string model_filename_;
  std::string adapter_type_;
  int runtime_id_;
  int t_start_mfm_;
  int t_end_mfm_;
  HFSAT::ttime_t t_start_time_;
  HFSAT::ttime_t t_end_time_;

  ReadStrategyFile(dbglogger_, strategy_desc_filename_, tradingdate_, strategy_name_, model_filename_, adapter_type_,
                   runtime_id_, t_start_mfm_, t_end_mfm_, t_start_time_, t_end_time_, market_model_index_,
                   use_accurate_seq2conf_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_);
  economic_events_manager_.AdjustSeverity("", HFSAT::kExchSourceNSE);
  HFSAT::PcaWeightsManager::SetUniqueInstance(tradingdate_);

  // nse is added by default
  HFSAT::SecurityDefinitions::GetUniqueInstance(global_tradingdate_).LoadNSESecurityDefinitions();
  HFSAT::MultModelCreator::CollectShortCodes(dbglogger_, watch_, model_filename_, source_shortcode_vec_,
                                             ors_needed_by_indicators_vec_, dependant_shortcode_vec_);

  // By this time, source_shortcode_vec, ors_needed etc. should have been populated

  // Set all the parameters in the common_smv_source
  common_smv_source->SetSourceShortcodes(source_shortcode_vec_);
  common_smv_source->SetSourcesNeedingOrs(ors_needed_by_indicators_vec_);
  common_smv_source->SetDepShortcodeVector(dependant_shortcode_vec_);
  common_smv_source->SetDepShortcode(dependant_shortcode_vec_[0]);
  common_smv_source->SetFakeFasterData(use_fake_faster_data_);
  common_smv_source->SetIgnoreUserMsg(ignore_user_msg_);
  common_smv_source->SetSimSmvRequired(true);
  common_smv_source->SetNSEL1Mode(use_l1_data_);
  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = common_smv_source->getSMVMap();
  HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_ = common_smv_source->getSimSMVMap();

  HFSAT::SimTimeSeriesInfo& sim_time_series_info_ = common_smv_source->getSimTimeSeriesInfo();
  if (!use_accurate_seq2conf_) {
    sim_time_series_info_.DisableAccurate();
  }

  HFSAT::NetworkAccountInfoManager& network_account_info_manager_ = common_smv_source->getNetworkAccountInfoManager();
  HFSAT::TradingLocation_t dep_trading_location_ = common_smv_source->getDepTradingLocation();
  HFSAT::HistoricalDispatcher& historical_dispatcher_ = common_smv_source->getHistoricalDispatcher();
  // std::vector<bool>& sid_to_ors_needed_map_ = common_smv_source->getSidORSNeededMap();

  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();
  HFSAT::ShortcodeORSMessageFilesourceMap& shortcode_ors_data_filesource_map_ =
      HFSAT::ShortcodeORSMessageFilesourceMap::GetUniqueInstance();
  std::map<std::string, HFSAT::BaseSimMarketMaker*> shortcode_to_smm_map;

  // Note that since market update manager is being initialized before the indicators
  // the PCAPortPrice::OnMarketDataResumed will be called before the OnMarketUpdate and
  // hence the security_id_last_price_map_ will not be uptodate when OnPortfolioPriceReset is called
  HFSAT::MarketUpdateManager& market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, tradingdate_));

  // created both iv model maths + fut model_maths
  std::vector<HFSAT::BaseMultipleModelMath*> modelmath_vec_ = HFSAT::MultModelCreator::CreateModelMathComponent(
      dbglogger_, watch_, model_filename_, t_start_mfm_, t_end_mfm_, runtime_id_);
  // instantiate both iv model indicators and fut model indicators
  HFSAT::MultModelCreator::CreateIndicatorInstances(dbglogger_, watch_);

  std::vector<HFSAT::BaseModelMath*> fut_modelmath_vec_;
  HFSAT::MultModelCreator::GetFutModelMathVec(fut_modelmath_vec_);

  std::vector<HFSAT::SecurityMarketView*> underlying_smv_vec_ = HFSAT::MultModelCreator::GetUnderlyingSMVVector();
  std::vector<HFSAT::PriceType_t> underlying_pricetype_vec_ = HFSAT::MultModelCreator::GetPriceTypeVec();
  std::map<std::string, std::vector<HFSAT::SecurityMarketView*> > shc_const_smv_map_ =
      HFSAT::MultModelCreator::GetShcToConstSMVMap();

  HFSAT::ImpliedVolAdapter* implied_vol_adapter = new HFSAT::ImpliedVolAdapter(
      dbglogger_, watch_, underlying_smv_vec_, underlying_pricetype_vec_, shc_const_smv_map_);

  std::vector<HFSAT::SmartOrderManager*> smart_order_manager_vec_;
  std::map<std::string, std::vector<HFSAT::SmartOrderManager*> > shc_const_som_map_;

  std::vector<HFSAT::BaseTrader*> base_trader_vec;
  std::vector<HFSAT::BaseSimMarketMaker*> smm_vec;
  std::vector<HFSAT::MultBasePNL*> mult_base_pnl_vec;
  HFSAT::MultBasePNL* total_mult_base_pnl_ = new HFSAT::MultBasePNL(dbglogger_, watch_);

  for (auto i = 0u; i < underlying_smv_vec_.size(); i++) {
    auto shortcode_ = underlying_smv_vec_[i]->shortcode();
    modelmath_vec_[i]->SubscribeMarketInterrupts(market_update_manager_);
    modelmath_vec_[i]->AddListener(implied_vol_adapter, i);

    auto security_id = sec_name_indexer_.GetIdFromString(shortcode_);
    auto dep_smv = shortcode_smv_map_.GetSecurityMarketView(shortcode_);
    auto sim_dep_smv = sid_to_sim_smv_ptr_map_[security_id];

    if (shortcode_to_smm_map.find(shortcode_) == shortcode_to_smm_map.end()) {
      shortcode_to_smm_map[shortcode_] = GetSMM(watch_, dep_smv, sim_dep_smv, market_model_index_,
                                                sim_time_series_info_, historical_dispatcher_, dbglogger_);
    }

    smm_vec.push_back(shortcode_to_smm_map[shortcode_]);

    // Get base trader instance and push it to the vec
    auto base_trader = GetBaseTrader(network_account_info_manager_, dep_smv, shortcode_to_smm_map[shortcode_]);
    base_trader_vec.push_back(base_trader);

    // Subscribe SMM to ors messages, do we need this ?

    if (shortcode_to_smm_map[shortcode_]) {
      auto ors_filesource = shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_);

      if (ors_filesource) {
        ors_filesource->AddOrderNotFoundListener(shortcode_to_smm_map[shortcode_]);
        ors_filesource->AddOrderSequencedListener(shortcode_to_smm_map[shortcode_]);
        ors_filesource->AddOrderConfirmedListener(shortcode_to_smm_map[shortcode_]);
        ors_filesource->AddOrderConfCxlReplacedListener(shortcode_to_smm_map[shortcode_]);
        // ors_filesource->AddOrderConfCxlReplaceRejectListener(shortcode_to_smm_map[shortcode]);
        ors_filesource->AddOrderCanceledListener(shortcode_to_smm_map[shortcode_]);
        ors_filesource->AddOrderExecutedListener(shortcode_to_smm_map[shortcode_]);
        ors_filesource->AddOrderRejectedListener(shortcode_to_smm_map[shortcode_]);
        ors_filesource->AddOrderCxlSeqdListener(shortcode_to_smm_map[shortcode_]);
      }
    }

    // Create Order manager instances
    auto smart_om = new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *base_trader, *dep_smv,
                                                 runtime_id_, livetrading_, 1);
    smart_order_manager_vec_.push_back(smart_om);
    smart_om->ComputeQueueSizes();

    HFSAT::MultBasePNL* p_structured_sim_base_pnl_ = new HFSAT::MultBasePNL(dbglogger_, watch_);
    mult_base_pnl_vec.push_back(p_structured_sim_base_pnl_);

    HFSAT::SimBasePNL* sim_base_pnl = NULL;
    sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_, *dep_smv, runtime_id_, trades_writer_);
    smart_om->SetBasePNL(sim_base_pnl);

    int t_index_ = p_structured_sim_base_pnl_->AddSecurity(sim_base_pnl) - 1;
    sim_base_pnl->AddListener(t_index_, p_structured_sim_base_pnl_);

    t_index_ = total_mult_base_pnl_->AddSecurity(p_structured_sim_base_pnl_) - 1;
    p_structured_sim_base_pnl_->AddPortListener(t_index_, total_mult_base_pnl_);

    if (shortcode_to_smm_map[shortcode_] && smart_om) {
      shortcode_to_smm_map[shortcode_]->AddOrderNotFoundListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderSequencedListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderConfirmedListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderConfCxlReplaceRejectedListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderConfCxlReplacedListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderCanceledListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderExecutedListener(smart_om);
      shortcode_to_smm_map[shortcode_]->AddOrderRejectedListener(smart_om);
    }

    shc_const_som_map_[shortcode_] = std::vector<HFSAT::SmartOrderManager*>();
  }

  for (auto i = 0u; i < underlying_smv_vec_.size(); i++) {
    auto dep_shortcode_ = underlying_smv_vec_[i]->shortcode();
    HFSAT::MultBasePNL* p_structured_sim_base_pnl_ = mult_base_pnl_vec[i];

    for (unsigned int j = 0; j < shc_const_smv_map_[underlying_smv_vec_[i]->shortcode()].size(); j++) {
      auto shortcode_ = shc_const_smv_map_[dep_shortcode_][j]->shortcode();

      auto security_id = sec_name_indexer_.GetIdFromString(shortcode_);
      auto dep_smv = shortcode_smv_map_.GetSecurityMarketView(shortcode_);
      auto sim_dep_smv = sid_to_sim_smv_ptr_map_[security_id];

      if (shortcode_to_smm_map.find(shortcode_) == shortcode_to_smm_map.end()) {
        shortcode_to_smm_map[shortcode_] = GetSMM(watch_, dep_smv, sim_dep_smv, market_model_index_,
                                                  sim_time_series_info_, historical_dispatcher_, dbglogger_);
      }

      smm_vec.push_back(shortcode_to_smm_map[shortcode_]);

      // Get base trader instance and push it to the vec
      auto base_trader = GetBaseTrader(network_account_info_manager_, dep_smv, shortcode_to_smm_map[shortcode_]);
      base_trader_vec.push_back(base_trader);

      // Subscribe SMM to ors messages
      if (shortcode_to_smm_map[shortcode_]) {
        auto ors_filesource = shortcode_ors_data_filesource_map_.GetORSMessageFileSource(shortcode_);

        if (ors_filesource) {
          ors_filesource->AddOrderNotFoundListener(shortcode_to_smm_map[shortcode_]);
          ors_filesource->AddOrderSequencedListener(shortcode_to_smm_map[shortcode_]);
          ors_filesource->AddOrderConfirmedListener(shortcode_to_smm_map[shortcode_]);
          ors_filesource->AddOrderConfCxlReplacedListener(shortcode_to_smm_map[shortcode_]);
          // ors_filesource->AddOrderConfCxlReplaceRejectListener(shortcode_to_smm_map[shortcode]);
          ors_filesource->AddOrderCanceledListener(shortcode_to_smm_map[shortcode_]);
          ors_filesource->AddOrderExecutedListener(shortcode_to_smm_map[shortcode_]);
          ors_filesource->AddOrderRejectedListener(shortcode_to_smm_map[shortcode_]);
          ors_filesource->AddOrderCxlSeqdListener(shortcode_to_smm_map[shortcode_]);
        }
      }

      // Create Order manager instances
      auto smart_om = new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *base_trader, *dep_smv,
                                                   runtime_id_, livetrading_, 1);
      shc_const_som_map_[dep_shortcode_].push_back(smart_om);
      smart_om->ComputeQueueSizes();

      HFSAT::SimBasePNL* sim_base_pnl = NULL;
      sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_,  *dep_smv, runtime_id_, trades_writer_);
      smart_om->SetBasePNL(sim_base_pnl);

      int t_index_ = p_structured_sim_base_pnl_->AddSecurity(sim_base_pnl) - 1;
      sim_base_pnl->AddListener(t_index_, p_structured_sim_base_pnl_);

      if (shortcode_to_smm_map[shortcode_] && smart_om) {
        shortcode_to_smm_map[shortcode_]->AddOrderNotFoundListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderSequencedListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderConfirmedListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderConfCxlReplaceRejectedListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderConfCxlReplacedListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderCanceledListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderExecutedListener(smart_om);
        shortcode_to_smm_map[shortcode_]->AddOrderRejectedListener(smart_om);
      }
    }
  }

  HFSAT::BaseOptionRiskPremium* base_option_risk_premium_ =
      new HFSAT::BaseOptionRiskPremium(dbglogger_, watch_, underlying_smv_vec_, tradingdate_, t_start_mfm_, t_end_mfm_);
  options_risk_manager_ = new HFSAT::OptionsRiskManager(
      dbglogger_, watch_, base_option_risk_premium_, underlying_smv_vec_, smart_order_manager_vec_, fut_modelmath_vec_,
      shc_const_som_map_, mult_base_pnl_vec, total_mult_base_pnl_, t_start_mfm_, t_end_mfm_, livetrading_, runtime_id_,
      sec_name_indexer_);
  HFSAT::BaseMultipleTrading* base_exec_ = new HFSAT::BaseMultipleTrading(
      dbglogger_, watch_, underlying_smv_vec_, shc_const_som_map_, options_risk_manager_, base_option_risk_premium_,
      t_start_mfm_, t_end_mfm_, livetrading_, runtime_id_, sec_name_indexer_);

  implied_vol_adapter->AddListener(base_exec_);

  if (!ignore_user_msg_) {
    HFSAT::ControlMessageFileSource* control_messasge_filesource_ = NULL;
    bool control_file_present = true;
    control_messasge_filesource_ = new HFSAT::ControlMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, underlying_smv_vec_[0]->security_id(),
        sec_name_indexer_.GetSecurityNameFromId(underlying_smv_vec_[0]->security_id()), dep_trading_location_,
        runtime_id_, control_file_present);

    if (control_messasge_filesource_ != NULL && control_file_present) {
      historical_dispatcher_.AddExternalDataListener(control_messasge_filesource_);
      control_messasge_filesource_->SetExternalTimeListener(&watch_);
      control_messasge_filesource_->AddControlMessageListener(options_risk_manager_);
    }
    base_exec_->SetStartTrading(true);
  } else {
    base_exec_->SetStartTrading(true);
  }

  // for all the model_math objects
  // this is commented out for, since we are updating our IV on sime timeperiod
  // FuturePrice-> SMV on ready is in IVAdaptor
  // We assume Option L1 Price changes doesnt offer any information ! Rather we should selective listen
  // If so how do we really get on MisPricing !?
  /*for (auto i = 0u; i < modelmath_vec_.size(); i++) {
    HFSAT::MultModelCreator::LinkupModelMathToOnReadySources(modelmath_vec_[i]);
    }*/

  // For all fut model math objects subscribe to SMV OnReady
  for (auto i = 0u; i < fut_modelmath_vec_.size(); i++) {
    if (fut_modelmath_vec_[i] != NULL) {
      std::vector<std::string> t_shortcode_affecting_vec_;
      std::vector<std::string> t_ors_needed_vec_;
      HFSAT::ModelCreator::CollectShortCodes(dbglogger_, watch_, fut_modelmath_vec_[i]->model_filename(),
                                             t_shortcode_affecting_vec_, t_ors_needed_vec_, false, false);
      HFSAT::ModelCreator::LinkupModelMathToOnReadySources(fut_modelmath_vec_[i], t_shortcode_affecting_vec_,
                                                           t_ors_needed_vec_);
    }
  }

  market_update_manager_.start();

  // To only process data starting MINUTES_TO_PREP minutes before min start time of strategies
  // In all the file sources, read events till
  // we reach the first event after the specified ttime_t

  HFSAT::ttime_t min_strat_trading_time_ = t_start_time_;
  HFSAT::ttime_t data_seek_time_ = min_strat_trading_time_ - HFSAT::ttime_t((int)secs_to_prep_, 0);
  if (!(strat_start_time_ == HFSAT::ttime_t(0, 0)) && strat_start_time_ <= min_strat_trading_time_) {
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

  HFSAT::ttime_t end_time = t_end_time_ + HFSAT::ttime_t(3600, 0);

  // Run event loop
  common_smv_source->Run(end_time);

  options_risk_manager_->ReportResults(trades_writer_, true);

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

  return 0;
}
