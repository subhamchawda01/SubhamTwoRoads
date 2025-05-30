/**
   \file InitLogic/base_trade_init.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <vector>
#include "basetrade/InitLogic/sim_strategy.hpp"

// #define CCPROFILING

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225
#define USE_CHIX_L1 1
HFSAT::DebugLogger dbglogger_(4 * 1024 * 1024,
                              256 * 1024);         // making logging more efficient, otherwise change it backto 10240, 1
HFSAT::BulkFileWriter trades_writer_(256 * 1024);  // 256KB

// To be used in termination_handler
bool global_livetrading_ = false;
int global_tradingdate_ = 0;
std::string global_strategy_desc_filename_ = "";
unsigned int global_progid_ = 0;
unsigned int global_market_model_index_ = 0;

void termination_handler(int signum) {
  // TODO check for strategies with open positions and print the same
  dbglogger_.Close();
  trades_writer_.Close();

  if (signum == SIGSEGV) {  // On a segfault inform , so this is picked up and fixed.
    char hostname_[128];
    hostname_[127] = '\0';
    gethostname(hostname_, 127);

    std::string email_string_ = "";
    std::string email_address_ = "";
    {
      std::ostringstream t_oss_;

      t_oss_ << "sim_strategy received SIGSEGV on " << hostname_ << "\n";
      t_oss_ << " livetrading_= " << global_livetrading_ << " tradingdate_= " << global_tradingdate_
             << " strategy_desc_filename_= " << global_strategy_desc_filename_ << " progid_= " << global_progid_
             << " market_model_index_= " << global_market_model_index_ << "\n";

      email_string_ = t_oss_.str();
    }

    HFSAT::Email email_;
    email_.setSubject(email_string_);

    if (!strncmp(getenv("USER"), "dvctrader", strlen("dvctrader"))) {
      email_address_ = "nseall@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ankit", strlen("ankit"))) {
      email_address_ = "ankit@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "rahul", strlen("rahul"))) {
      email_address_ = "rahul@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "rkumar", strlen("rkumar"))) {
      email_address_ = "rakesh.kumar@tworoads.co.in";
    } else if (!strncmp(getenv("USER"), "ravi", strlen("ravi"))) {
      email_address_ = "ravi.parikh@tworoads.co.in";
    } else {  // Not sure if others want to receive these emails.
      email_address_ = "nseall@tworoads.co.in";
    }

    email_.addRecepient(email_address_);
    email_.addSender(email_address_);
    email_.content_stream << email_string_ << "<br/>";
    email_.sendMail();

    abort();
  }

  exit(0);
}

/// expect :
/// 1. $tradeinitexec "SIM" STRATEGYDESCFILENAME PROGID TRADINGDATE
///    $tradeinitexec "SIM" STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX
void ParseCommandLineParams(int argc, char** argv, bool& livetrading_, int& tradingdate_,
                            std::string& strategy_desc_filename_, unsigned int& progid_,
                            std::string& network_account_info_filename_, unsigned int& market_model_index_,
                            std::vector<std::string>& dbg_code_vec_) {
  if (argc < 4) {  // 4 is min of live and sim
    std::cerr << "expecting :\n"
              << " 1. $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE " << '\n'
              << "    $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 " << '\n'
              << "    $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 "
                 "NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" " << '\n';

    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  } else {
    if (strcmp(argv[1], "LIVE") == 0) {
      std::cerr << "expecting :\n"
                << " 1. $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE " << '\n'
                << "    $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 " << '\n'
                << "    $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 "
                   "NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" " << '\n';

      HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
    } else {
      livetrading_ = false;
      if (argc < 5) {
        std::cerr << "expecting :\n"
                  << " 1. $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE [ ADD_DBG_CODE DBG_CODE1 "
                     "DBG_CODE2 ... ] " << '\n'
                  << "    $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 [ "
                     "ADD_DBG_CODE DBG_CODE1 DBG_CODE2 ... ] " << '\n'
                  << "    $tradeinitexec SIM STRATEGYDESCFILENAME PROGID TRADINGDATE MARKETMODELINDEX=0 "
                     "NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" [ ADD_DBG_CODE DBG_CODE1 DBG_CODE2 ... ] " << '\n';

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
              // expecting NETWORKINFOFILENAME
              if (strcmp(argv[6], "SAME_NETWORK_INFO") != 0) {
                network_account_info_filename_ = argv[6];
              }

              if (argc >= 8) {
                if (strcmp(argv[7], "ADD_DBG_CODE") == 0) {
                  for (int i = 8; i < argc; i++) {
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

  global_livetrading_ = livetrading_;
  global_tradingdate_ = tradingdate_;
  global_strategy_desc_filename_ = strategy_desc_filename_;
  global_progid_ = progid_;
  global_market_model_index_ = market_model_index_;
}

class ShortcodePriceTypePrinter : public HFSAT::SecurityMarketViewChangeListener {
  const HFSAT::SecurityMarketView& this_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;

 public:
  ShortcodePriceTypePrinter(const HFSAT::SecurityMarketView& _this_smv_, HFSAT::DebugLogger& _dbglogger_,
                            HFSAT::Watch& _watch_)
      : this_smv_(_this_smv_), dbglogger_(_dbglogger_), watch_(_watch_) {}

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (dbglogger_.CheckLoggingLevel(INDICATOR_PRICE_TEST)) {
      std::ostringstream t_temp_oss_;

      t_temp_oss_ << " On Base Shortcode Market Update @ : " << watch_.tv()
                  << " For : " << _market_update_info_.secname_ << " "
                  << " kPriceTypeMidprice " << _market_update_info_.mid_price_ << " "
                  << " kPriceTypeMktSizeWPrice " << _market_update_info_.mkt_size_weighted_price_ << " "
                  << " kPriceTypeMktSinusoidal " << _market_update_info_.mkt_sinusoidal_price_ << " "
                  << " kPriceTypeOrderWPrice " << _market_update_info_.order_weighted_price_ << " "
                  << " kPriceTypeOfflineMixMMS " << _market_update_info_.offline_mix_mms_price_ << "\n";

      dbglogger_ << t_temp_oss_.str();
      dbglogger_.CheckToFlushBuffer();
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
};

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
  bool livetrading_ = false;
  int tradingdate_ = 0;
  unsigned int progid_ = 0;
  std::string strategy_desc_filename_ = "";
  std::string logfilename_ = "";
  std::string tradesfilename_ = "";

  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");
  unsigned int market_model_index_ = 0u;
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  std::vector<std::string> source_shortcode_vec_;  ///< vector of all sources which we need data for or are trading
  std::vector<std::string>
      ors_needed_by_indicators_vec_;  ///< vector of all sources which we need ORS messages for, to build indicators
  std::vector<std::string>
      dependant_shortcode_vec_;  ///< separate vector for the sources which we have to made simtraders etc of
  // bool using_non_self_market_view_ = false; ///< if using_non_self_market_view_ then ORS messages are listened to by
  // SimMarketMaker, PromOrderMmanager ...

  std::vector<bool> sid_to_marketdata_needed_map_;  ///< security_id to bool map indicating whether market data is
  /// needed or not for this source ...

  std::vector<std::string> dbg_code_vec_;  ///< set of codes that we want the logger to print on

#ifdef CCPROFILING
  // Cpu Cycle Profiling
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(10);
  cpucycle_profiler_.SetTag(0, "ProcessAllEvents to PLMVM");
  cpucycle_profiler_.SetTag(1, "PLMVM to SMVup");
  cpucycle_profiler_.SetTag(2, "SMV to LinearModelAggregator::OnIndicatorUpdate");
  cpucycle_profiler_.SetTag(3, "LinearModelAggregator::OnIndicatorUpdate to BaseTrading::UpdateTarget");
  cpucycle_profiler_.SetTag(4, "BaseTrading::UpdateTarget to SimMarketMaker::SendOrderExch");
#endif  // CCPROFILING

  // parse command line paramters
  ParseCommandLineParams(argc, argv, livetrading_, tradingdate_, strategy_desc_filename_, progid_,
                         network_account_info_filename_, market_model_index_, dbg_code_vec_);

  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/tradelogs/log." << tradingdate_ << "." << progid_;
    logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);

    for (auto i = 0u; i < dbg_code_vec_.size(); i++) {
      // TODO .. add ability to write "WATCH_INFO" instead of 110, and making it
      int dbg_code_to_be_logged_ = HFSAT::DebugLogger::TextToLogLevel(dbg_code_vec_[i].c_str());
      if (dbg_code_to_be_logged_ < 0) {
        dbglogger_.SetNoLogs();
        break;
      } else {
        dbglogger_ << "Add LogLevel " << dbg_code_to_be_logged_ << DBGLOG_ENDL_FLUSH;
        dbglogger_.AddLogLevel(dbg_code_to_be_logged_);
      }
    }

    dbglogger_.AddLogLevel(WATCH_ERROR);

    dbglogger_.AddLogLevel(OM_ERROR);
    // dbglogger_.AddLogLevel ( OM_INFO );

    dbglogger_.AddLogLevel(PLSMM_ERROR);
    // dbglogger_.AddLogLevel ( PLSMM_INFO );
    // dbglogger_.AddLogLevel ( PLSMM_TEST );

    dbglogger_.AddLogLevel(TRADING_ERROR);
    // dbglogger_.AddLogLevel ( TRADING_INFO );

    dbglogger_.AddLogLevel(BOOK_ERROR);

    dbglogger_.AddLogLevel(LRDB_ERROR);

    dbglogger_.AddLogLevel(DBG_MODEL_ERROR);

    dbglogger_.AddLogLevel(SMVSELF_ERROR);
    // dbglogger_.AddLogLevel ( SMVSELF_INFO );
  }

  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/tradelogs/trades." << tradingdate_ << "." << progid_;
    tradesfilename_ = t_temp_oss_.str();
  }

  // open trades file in append mode in livetrading_
  trades_writer_.Open(tradesfilename_.c_str(), (livetrading_ ? std::ios::app : std::ios::out));

  HFSAT::Watch watch_(dbglogger_, tradingdate_);
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  HFSAT::PcaWeightsManager::SetUniqueInstance(tradingdate_);

  std::map<std::string, std::vector<std::string> > modelfilename_source_shortcode_vec_map_;  ///< map from
  /// modelfilename_ to
  /// source_shortcode_vec_ to
  /// pass to setup ModelMath
  /// as listener to
  /// market_data events
  std::map<std::string, std::vector<std::string> >
      modelfilename_ors_needed_by_indicators_vec_map_;  ///< map from modelfilename_ to source_shortcode_vec_ to pass to
  /// setup ModelMath as listener to OrderRouting message events

  // process strategy_desc_file
  HFSAT::StrategyDesc strategy_desc_(dbglogger_, strategy_desc_filename_, tradingdate_);

  // build the list of source_shortcode_vec_ to initialize and dependant_shortcode_vec_ ( to make simmarketmakers etc )
  for (auto i = 0u; i < strategy_desc_.strategy_vec_.size(); i++) {
    if (modelfilename_source_shortcode_vec_map_.find(strategy_desc_.strategy_vec_[i].modelfilename_) ==
        modelfilename_source_shortcode_vec_map_.end()) {
      HFSAT::VectorUtils::UniqueVectorAdd(dependant_shortcode_vec_, strategy_desc_.strategy_vec_[i].dep_shortcode_);
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, strategy_desc_.strategy_vec_[i].dep_shortcode_);
      HFSAT::ModelCreator::CollectShortCodes(
          dbglogger_, watch_, strategy_desc_.strategy_vec_[i].modelfilename_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_],
          modelfilename_ors_needed_by_indicators_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_],
          false);  // allow_non_dependant_ = false

      HFSAT::VectorUtils::UniqueVectorAdd(
          source_shortcode_vec_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
      HFSAT::VectorUtils::UniqueVectorAdd(
          ors_needed_by_indicators_vec_,
          modelfilename_ors_needed_by_indicators_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);

      HFSAT::BaseTrading::CollectORSShortCodes(dbglogger_, strategy_desc_.strategy_vec_[i].strategy_name_,
                                               strategy_desc_.strategy_vec_[i].dep_shortcode_, source_shortcode_vec_,
                                               ors_needed_by_indicators_vec_);  // strictly speaking the ors sources
                                                                                // here aren't needed by indicators.
                                                                                // they are needed by risk managing code
    }
  }

  for (auto i = 0u; i < source_shortcode_vec_.size();
       i++) {  // go through all source_shortcodes and check for hybrid securities
    if (source_shortcode_vec_[i].compare("HYB_ESPY") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("ES_0"));
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("SPY"));
    } else if (source_shortcode_vec_[i].compare("HYB_NQQQ") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("NQ_0"));
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("QQQ"));
    } else if (source_shortcode_vec_[i].compare("HYB_YDIA") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("YM_0"));
      HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_vec_, std::string("DIA"));
    }
  }

  // get exchange symbols corresponding to the shortcodes of interest
  // add exchange symbols to SecurityNameIndexer
  for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
    if (!sec_name_indexer_.HasString(
            source_shortcode_vec_[i])) {  // need to add this source to sec_name_indexer_ since it was not added already
      // A unique instance of ExchangeSymbolManager gets the current symbol that the exchange knows this shortcode as
      // and also allocates permanent storage to this instrument, that allows read access from outside.
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(source_shortcode_vec_[i]);
      sec_name_indexer_.AddString(exchange_symbol_, source_shortcode_vec_[i]);
      sid_to_marketdata_needed_map_.push_back(
          true);  // we need market data for every symbol in source_shortcode_vec_ since this was based on the modelfile
    }
  }

  for (auto i = 0u; i < ors_needed_by_indicators_vec_.size(); i++) {
    if (!sec_name_indexer_.HasString(ors_needed_by_indicators_vec_[i])) {  // need to add this source to
                                                                           // sec_name_indexer_ since it was not added
                                                                           // already
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(ors_needed_by_indicators_vec_[i]);
      sec_name_indexer_.AddString(exchange_symbol_, ors_needed_by_indicators_vec_[i]);
      sid_to_marketdata_needed_map_.push_back(false);
    }
  }

  // Please note that it is not the case that for every sid there will be an SMV *, since some could just be for ORS
  // based info, managed by PromOrderManager
  std::vector<bool> sid_is_dependant_map_;  ///< security_id to bool map indicating whether this is a dependant or not
  std::vector<HFSAT::ExchSource_t> sid_to_exch_source_map_;
  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
    sid_is_dependant_map_.push_back(
        HFSAT::VectorUtils::LinearSearchValue(dependant_shortcode_vec_, sec_name_indexer_.GetShortcodeFromId(i)));
    sid_to_exch_source_map_.push_back(
        HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_, tradingdate_));
  }

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  // map security_id to a newly created SecurityMarketView *
  // map shortcode_ to the SecurityMarketView *
  // map security_id to ExchSource_t
  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
    const char* _this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);
    if (sid_to_exch_source_map_[i] != HFSAT::kExchSourceHYB) {
      if (sid_to_marketdata_needed_map_[i]) {  // only for the securites we are processing market data
        bool set_temporary_bool_checking_if_this_is_an_indexed_book_ = false;
        if ((sid_to_exch_source_map_[i] == HFSAT::kExchSourceLIFFE) ||
            (sid_to_exch_source_map_[i] == HFSAT::kExchSourceEUREX)
            // || ( sid_to_exch_source_map_[i] == HFSAT::kExchSourceCME )
            ) {
          set_temporary_bool_checking_if_this_is_an_indexed_book_ = true;
        }

        // if ( ( _this_shortcode_.compare ( 0 , 3 , "LFI" ) == 0 ) ||
        //      ( _this_shortcode_.compare ( 0 , 6 , "SP_LFI" ) == 0 ) ||
        //      ( _this_shortcode_.compare ( 0 , 6 , "SP_LFL" ) == 0 ) ||
        //      ( _this_shortcode_.compare ( 0 , 3 , "LFL" ) == 0 ) )
        //   { // Using original liffe-book for products requiring order-level computations ...
        //     // currently only in simtrader
        //     set_temporary_bool_checking_if_this_is_an_indexed_book_ = false ;
        //   }

        HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
            dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_, i,
            sid_to_exch_source_map_[i], set_temporary_bool_checking_if_this_is_an_indexed_book_);

        ShortcodePriceTypePrinter* sptp_ = new ShortcodePriceTypePrinter((*p_smv_), dbglogger_, watch_);

        p_smv_->subscribe_price_type(sptp_, HFSAT::kPriceTypeMktSizeWPrice);
        p_smv_->subscribe_price_type(sptp_, HFSAT::kPriceTypeOrderWPrice);
        p_smv_->subscribe_price_type(sptp_, HFSAT::kPriceTypeMidprice);
        p_smv_->subscribe_price_type(sptp_, HFSAT::kPriceTypeMktSinusoidal);
        p_smv_->subscribe_price_type(sptp_, HFSAT::kPriceTypeOfflineMixMMS);

        sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
        shortcode_smv_map_.AddEntry(_this_shortcode_, p_smv_);  // add to shortcode_ to SMV* map
      } else {
        sid_to_smv_ptr_map_.push_back(NULL);  // sid points to NULL since we do not want to process market data for this
      }
    } else {
      sid_to_smv_ptr_map_.push_back(NULL);  // sid points to NULL since we will add HSMV * in the next loop
    }
  }

  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
    const char* _this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);
    if (sid_to_exch_source_map_[i] == HFSAT::kExchSourceHYB) {
      if (sid_to_marketdata_needed_map_[i]) {  // only for the securites we are processing market data ... should be
                                               // always true

        HFSAT::SecurityMarketView* p_smv1_ = NULL;
        HFSAT::SecurityMarketView* p_smv2_ = NULL;

        if (_this_shortcode_.compare("HYB_ESPY") == 0) {
          p_smv1_ = shortcode_smv_map_.GetSecurityMarketView("ES_0");
          p_smv2_ = shortcode_smv_map_.GetSecurityMarketView("SPY");
        }
        if (_this_shortcode_.compare("HYB_NQQQ") == 0) {
          p_smv1_ = shortcode_smv_map_.GetSecurityMarketView("NQ_0");
          p_smv2_ = shortcode_smv_map_.GetSecurityMarketView("QQQ");
        }
        if (_this_shortcode_.compare("HYB_YDIA") == 0) {
          p_smv1_ = shortcode_smv_map_.GetSecurityMarketView("YM_0");
          p_smv2_ = shortcode_smv_map_.GetSecurityMarketView("DIA");
        }

        HFSAT::HybridSecurityMarketView* p_hyb_smv_ = new HFSAT::HybridSecurityMarketView(
            dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_, i,
            sid_to_exch_source_map_[i], p_smv1_, p_smv2_);
        sid_to_smv_ptr_map_[i] = p_hyb_smv_;  // add to security_id_ to SMV* map, was previously set to NULL
        shortcode_smv_map_.AddEntry(_this_shortcode_, p_hyb_smv_);  // add to shortcode_ to SMV* map
      }
    }
  }

  std::vector<HFSAT::PromOrderManager*> sid_to_prom_order_manager_map_(
      sec_name_indexer_.NumSecurityId(), NULL);  ///< used for all sources in case using_non_self_market_view_=true.
  /// Otherwise used for ors_needed_by_indicators_vec_, and
  /// dependant_shortcode_vec_
  std::vector<bool> sid_to_ors_needed_map_(sec_name_indexer_.NumSecurityId(), false);
  /** Setup PromOrderManager if either
   * (i) doing so for all sources to adjust market view based on this
   * (ii) is a dependant source and hence needed by execstrategy ( and simtrader in historical )
   * (iii) is needed by some indicator
   */
  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
    const char* _this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);

    sid_to_ors_needed_map_[i] = ((sid_is_dependant_map_[i]) || (HFSAT::VectorUtils::LinearSearchValue(
                                                                   ors_needed_by_indicators_vec_, _this_shortcode_)));

    if (sid_to_ors_needed_map_[i]) {  // Ors-needed will only be true if either dependant / indicators require ors data.
      sid_to_prom_order_manager_map_[i] = HFSAT::PromOrderManager::GetUniqueInstance(
          dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, i, _this_exchange_symbol_);
    }
  }

// After creating the promOMs , set SMV non-self options.
// _USE_NON_SELF_ == true && _USE_SMART_NON_SELF_ == false => naive_non_self
// _USE_NON_SELF_ == true && _USE_SMART_NON_SELF_ == true => smart_non_self
// _USE_NON_SELF_ == false && _USE_SMART_NON_SELF_ == false => full_mkt
// _USE_NON_SELF_ == false && _USE_SMART_NON_SELF_ == true => full_mkt
#define _USE_NON_SELF_ true
#define _USE_SMART_NON_SELF_ true

  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);

    if (sid_to_ors_needed_map_[i] &&
        HFSAT::SecurityDefinitions::GetRemoveSelfOrdersFromBook(_this_shortcode_, tradingdate_)) {
#if _USE_NON_SELF_
      sid_to_smv_ptr_map_[i]->SetSelfOrdersFromBook(true);
#endif
#if _USE_SMART_NON_SELF_
      sid_to_smv_ptr_map_[i]->SetSmartSelfOrdersFromBook(true);
#endif
    }
  }

  // initialize the marketdata processing market_view_managers
  // during initialization , this links itself to PromOrderManager if necessary

  HFSAT::IndexedCmeMarketViewManager indexed_cme_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedLiffePriceLevelMarketViewManager indexed_liffe_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_puma_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);

  HFSAT::IndexedTmxMarketViewManager indexed_tmx_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);

  HFSAT::HKEXIndexedMarketViewManager hkex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                sid_to_smv_ptr_map_);
  HFSAT::OSEL1PriceMarketViewManager ose_l1_price_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);

  HFSAT::BMFOrderLevelMarketViewManager bmf_order_level_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                             sid_to_smv_ptr_map_);
  HFSAT::IndexedRtsMarketViewManager indexed_rts_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedMicexMarketViewManager indexed_micex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);

  HFSAT::HistoricalDispatcher historical_dispatcher_;

  bool using_tmx_hist_data_ = false;

  std::map<std::string, HFSAT::CMELoggedMessageFileSource*> shortcode_cme_data_filesource_map_;
  std::map<std::string, HFSAT::EUREXLoggedMessageFileSource*> shortcode_eurex_data_filesource_map_;
  std::map<std::string, HFSAT::NTPLoggedMessageFileSource*> shortcode_ntp_data_filesource_map_;
  std::map<std::string, HFSAT::LIFFELoggedMessageFileSource*> shortcode_liffe_data_filesource_map_;
  std::map<std::string, HFSAT::TMXLoggedMessageFileSource*> shortcode_tmx_data_filesource_map_;
  std::map<std::string, HFSAT::RTSLoggedMessageFileSource*> shortcode_rts_data_filesource_map_;
  std::map<std::string, HFSAT::MICEXLoggedMessageFileSource*> shortcode_micex_data_filesource_map_;
  std::map<std::string, HFSAT::HKEXLoggedMessageFileSource*> shortcode_hkex_data_filesource_map_;
  std::map<std::string, HFSAT::OSEL1LoggedMessageFileSource*> shortcode_ose_data_filesource_map_;
  std::map<std::string, HFSAT::ORSMessageFileSource*> shortcode_ors_data_filesource_map_;

  HFSAT::SimTimeSeriesInfo sim_time_series_info_(sec_name_indexer_.NumSecurityId());

  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  HFSAT::TradingLocation_t dep_trading_location_ = HFSAT::kTLocCHI;
  if (dependant_shortcode_vec_.empty()) {
    HFSAT::ExitVerbose(HFSAT::kDepShortCodeVecEmptyTradeInit);
  } else {
    dep_trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(
        HFSAT::SecurityDefinitions::GetContractExchSource(dependant_shortcode_vec_[0], tradingdate_));
  }

  // depending on historical study or real trading, initialize Adapters ( with right input argument ) for each market
  // data source
  {  // historical .. so load filesources
    for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
      if (sim_time_series_info_.sid_to_sim_config_.size() < i) {
        sim_time_series_info_.sid_to_sim_config_.resize(i);
      }
      std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);

      switch (sid_to_exch_source_map_[i]) {
        case HFSAT::kExchSourceCME: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_cme_data_filesource_map_.find(shortcode_) == shortcode_cme_data_filesource_map_.end()) {
              shortcode_cme_data_filesource_map_[shortcode_] = new HFSAT::CMELoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_cme_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  &indexed_cme_market_view_manager_);
              shortcode_cme_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

              historical_dispatcher_.AddExternalDataListener(shortcode_cme_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceEUREX: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_eurex_data_filesource_map_.find(shortcode_) == shortcode_eurex_data_filesource_map_.end()) {
              shortcode_eurex_data_filesource_map_[shortcode_] = new HFSAT::EUREXLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_eurex_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  &indexed_cme_market_view_manager_);
              shortcode_eurex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_eurex_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceBMF:
        case HFSAT::kExchSourceNTP: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
              shortcode_ntp_data_filesource_map_[shortcode_] = new HFSAT::NTPLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_ntp_data_filesource_map_[shortcode_]->SetNTPPriceLevelGlobalListener(
                  &indexed_ntp_market_view_manager_);

              shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ntp_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceBMFEQ: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_ntp_data_filesource_map_.find(shortcode_) == shortcode_ntp_data_filesource_map_.end()) {
              shortcode_ntp_data_filesource_map_[shortcode_] = new HFSAT::NTPLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_, false, true);
              shortcode_ntp_data_filesource_map_[shortcode_]->SetOrderLevelGlobalListener(
                  &bmf_order_level_market_view_manager_);
              shortcode_ntp_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ntp_data_filesource_map_[shortcode_]);
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceTMX: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_tmx_data_filesource_map_.find(shortcode_) == shortcode_tmx_data_filesource_map_.end()) {
              shortcode_tmx_data_filesource_map_[shortcode_] = new HFSAT::TMXLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_tmx_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                  &indexed_tmx_market_view_manager_);
              shortcode_tmx_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_tmx_data_filesource_map_[shortcode_]);
              using_tmx_hist_data_ = true;

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;

        case HFSAT::kExchSourceLIFFE: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_liffe_data_filesource_map_.find(shortcode_) == shortcode_liffe_data_filesource_map_.end()) {
              shortcode_liffe_data_filesource_map_[shortcode_] = new HFSAT::LIFFELoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_liffe_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  &indexed_liffe_price_level_market_view_manager_);
              shortcode_liffe_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

              historical_dispatcher_.AddExternalDataListener(shortcode_liffe_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceRTS: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_rts_data_filesource_map_.find(shortcode_) == shortcode_rts_data_filesource_map_.end()) {
              shortcode_rts_data_filesource_map_[shortcode_] = new HFSAT::RTSLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_rts_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  &indexed_rts_market_view_manager_);
              shortcode_rts_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_rts_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceMICEX:
        case HFSAT::kExchSourceMICEX_EQ:
        case HFSAT::kExchSourceMICEX_CR: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_micex_data_filesource_map_.find(shortcode_) == shortcode_micex_data_filesource_map_.end()) {
              shortcode_micex_data_filesource_map_[shortcode_] = new HFSAT::MICEXLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_micex_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                  &indexed_micex_market_view_manager_);
              shortcode_micex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

              historical_dispatcher_.AddExternalDataListener(shortcode_micex_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceHONGKONG: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_hkex_data_filesource_map_.find(shortcode_) == shortcode_hkex_data_filesource_map_.end()) {
              shortcode_hkex_data_filesource_map_[shortcode_] = new HFSAT::HKEXLoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_hkex_data_filesource_map_[shortcode_]->SetHKHalfBookGlobalListener(&hkex_market_view_manager_);
              shortcode_hkex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

              historical_dispatcher_.AddExternalDataListener(shortcode_hkex_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceJPY: {
          if (sid_to_marketdata_needed_map_[i]) {
            if (shortcode_ose_data_filesource_map_.find(shortcode_) == shortcode_ose_data_filesource_map_.end()) {
              // TODO : For OSE dependant , setup order-level source and mvm.
              shortcode_ose_data_filesource_map_[shortcode_] = new HFSAT::OSEL1LoggedMessageFileSource(
                  dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                  dep_trading_location_);
              shortcode_ose_data_filesource_map_[shortcode_]->SetFullBookGlobalListener(
                  &ose_l1_price_market_view_manager_);
              shortcode_ose_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ose_data_filesource_map_[shortcode_]);
            }
          }

          if (sid_to_ors_needed_map_[i]) {
            if (shortcode_ors_data_filesource_map_.find(shortcode_) == shortcode_ors_data_filesource_map_.end()) {
              shortcode_ors_data_filesource_map_[shortcode_] =
                  new HFSAT::ORSMessageFileSource(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                  sec_name_indexer_.GetSecurityNameFromId(i), dep_trading_location_);
              shortcode_ors_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);
              historical_dispatcher_.AddExternalDataListener(shortcode_ors_data_filesource_map_[shortcode_]);

              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_.size() <= i) {
                sim_time_series_info_.sid_to_sim_config_.push_back(HFSAT::SimConfig::GetSimConfigsForShortcode(
                    dbglogger_, watch_, shortcode_, strategy_desc_.simconfig_filename_));
              }
              if (sid_is_dependant_map_[i] && sim_time_series_info_.sid_to_sim_config_[i].use_accurate_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_, i,
                                                                       dep_trading_location_);
              }
              if (sid_is_dependant_map_[i] &&
                  sim_time_series_info_.sid_to_sim_config_[i].use_accurate_cxl_seqd_to_conf_ &&
                  sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i].empty()) {
                sim_time_series_info_.sid_to_time_to_cxl_seqd_to_conf_times_[i] =
                    HFSAT::ORSMessageStatsComputer::GetCxlSeqdToConfTimes(dbglogger_, sec_name_indexer_, tradingdate_,
                                                                          i, dep_trading_location_);
              }
            }
          }
        } break;
        case HFSAT::kExchSourceMEFF:
        case HFSAT::kExchSourceIDEM:
        case HFSAT::kExchSourceREUTERS:
        case HFSAT::kExchSourceICE:
        case HFSAT::kExchSourceEBS:
        default: { } break; }
    }
  }
  HFSAT::MarketUpdateManager& market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_, tradingdate_));

  HFSAT::MulticastSenderSocket* p_strategy_param_sender_socket_ = NULL;

  // if in live then setup the control_screen
  // looking at the strategy_desc_file, get the list of models and initialize them
  // for each product traded make a simmarketmaker ( if sim ) or live order_routing_daemon ( if live )
  // for each line in the strategy_desc_file initialize :
  //     internal_order_routing ( bound to the appropriate SimMarketMaker )
  //     strategy_code

  for (auto i = 0u; i < strategy_desc_.strategy_vec_.size(); i++) {
    const std::string& shortcode_ = strategy_desc_.strategy_vec_[i].dep_shortcode_;
    const unsigned int security_id_ = sec_name_indexer_.GetIdFromString(shortcode_);

    strategy_desc_.strategy_vec_[i].p_dep_market_view_ = shortcode_smv_map_.GetSecurityMarketView(shortcode_);
    strategy_desc_.strategy_vec_[i].exch_traded_on_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate_);
    strategy_desc_.strategy_vec_[i].p_base_trader_ = NULL;
    HFSAT::BaseSimMarketMaker* p_base_sim_market_maker_ = NULL;  // created outside to add listeneres later

    // if SIM then create SimMarketMaker and SimTrader
    // on create SimMarketMaker makes itself a listener to SecurityMarketView passed
    switch (strategy_desc_.strategy_vec_[i].exch_traded_on_) {
      case HFSAT::kExchSourceCME: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);
        // note that use of network_account_info_manager_ in SIM mode here causes one to need the
        // network_account_info_filename_ in SIM
        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceCME, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceEUREX: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);

        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceEUREX, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceBMF:
      case HFSAT::kExchSourceNTP:
      case HFSAT::kExchSourceBMFEQ: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);

        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceBMF, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceTMX: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);
        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceTMX, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceLIFFE: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);

        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceLIFFE, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceRTS: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);

        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceRTS, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceMICEX:
      case HFSAT::kExchSourceMICEX_EQ:
      case HFSAT::kExchSourceMICEX_CR: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);
        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceMICEX, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceHONGKONG: {
        p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_,
            sim_time_series_info_);
        strategy_desc_.strategy_vec_[i].p_base_trader_ =
            new HFSAT::BaseSimTrader(network_account_info_manager_.GetDepTradeAccount(
                                         HFSAT::kExchSourceHONGKONG, strategy_desc_.strategy_vec_[i].dep_shortcode_),
                                     p_base_sim_market_maker_);
      } break;
      case HFSAT::kExchSourceJPY: {
        // TODO
        // p_base_sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance ( dbglogger_, watch_,
        // *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), market_model_index_ , sim_time_series_info_ );
        // strategy_desc_.strategy_vec_[i].p_base_trader_ = new HFSAT::OSESimTrader (
        // network_account_info_manager_.GetDepTradeAccount ( HFSAT::kExchSourceHONGKONG,
        // strategy_desc_.strategy_vec_[i].dep_shortcode_ ),
        //                                                                          p_base_sim_market_maker_ ) ;
      } break;

      default:
        break;
    }
    {  // add SimMarketMaker as a listener to ORS
      if (p_base_sim_market_maker_ != NULL) {
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderNotFoundListener(p_base_sim_market_maker_);
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderSequencedListener(p_base_sim_market_maker_);
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderConfirmedListener(p_base_sim_market_maker_);
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderConfCxlReplacedListener(p_base_sim_market_maker_);
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderCanceledListener(p_base_sim_market_maker_);
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderExecutedListener(p_base_sim_market_maker_);
        shortcode_ors_data_filesource_map_[shortcode_]->AddOrderRejectedListener(p_base_sim_market_maker_);
      }
    }
    // on create SmartOrderManager adds itself as listeners to SecurityMarketView
    // and BasePNL also becomes SMV listener

    HFSAT::SmartOrderManager* p_smart_order_manager_ = new HFSAT::SmartOrderManager(
        dbglogger_, watch_, sec_name_indexer_, *(strategy_desc_.strategy_vec_[i].p_base_trader_),
        *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), strategy_desc_.strategy_vec_[i].runtime_id_,
        livetrading_, 1);
    HFSAT::SimBasePNL* p_sim_base_pnl_ = new HFSAT::SimBasePNL(
        dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_),
        strategy_desc_.strategy_vec_[i].runtime_id_, trades_writer_);
    p_smart_order_manager_->SetBasePNL(p_sim_base_pnl_);

    {
      // add SmartOrderManager as a listener to SimMarketMaker
      // since all simulated market replies need to be sent to SmartOM
      if ((p_base_sim_market_maker_ != NULL) && (p_smart_order_manager_ != NULL)) {
        p_base_sim_market_maker_->AddOrderNotFoundListener(p_smart_order_manager_);
        p_base_sim_market_maker_->AddOrderSequencedListener(p_smart_order_manager_);
        p_base_sim_market_maker_->AddOrderConfirmedListener(p_smart_order_manager_);
        p_base_sim_market_maker_->AddOrderConfCxlReplacedListener(p_smart_order_manager_);
        p_base_sim_market_maker_->AddOrderCanceledListener(p_smart_order_manager_);
        p_base_sim_market_maker_->AddOrderExecutedListener(p_smart_order_manager_);
        p_base_sim_market_maker_->AddOrderRejectedListener(p_smart_order_manager_);
      }
    }

    // create ModelMath component if not created .
    // It is fine to have multiple strategy lines with same modelfilename_, and hence having the same ModelmathComponent
    // (since modelmathcomponent is only created in ModelCreator if modelfilename is missing as key in map
    // It appears that in that case if the dependant is also the same ( which is likely to be the case ), for the second
    // strategy instance
    //   the indicators will be a listener to SMV before this SimMarketMaker instance is even created and hence they
    //   will be a listener before SimMarketMaker
    //   But that is fine since ModelMath only calls it's listeners on an SMVOnReady ( ) call from SMV, which is called
    //   after all SMVChangeListeners are done.
    //   Hence strategy should get new Model output target price after SimMarketMaker and SmartOrderManager get
    //   SMVChange callbacks
    HFSAT::BaseModelMath* base_model_math_ = HFSAT::ModelCreator::CreateModelMathComponent(
        dbglogger_, watch_, strategy_desc_.strategy_vec_[i].modelfilename_, nullptr, 0, 86400000, 1);

    // subscribes all indicators to market data interrupts
    base_model_math_->SubscribeMarketInterrupts(market_update_manager_);

    // On Create Exec Strategy attaches itself as a listener to SecurityMarketView *
    if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(HFSAT::DirectionalAggressiveTrading::StrategyName()) ==
        0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalAggressiveTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                   HFSAT::DirectionalInterventionAggressiveTrading::StrategyName()) == 0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::DirectionalInterventionAggressiveTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(HFSAT::PriceBasedTrading::StrategyName()) == 0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                   HFSAT::PriceBasedAggressiveTrading::StrategyName()) == 0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedAggressiveTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                   HFSAT::PriceBasedSecurityAggressiveTrading::StrategyName()) == 0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedSecurityAggressiveTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                   HFSAT::PriceBasedInterventionAggressiveTrading::StrategyName()) == 0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedInterventionAggressiveTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(HFSAT::PriceBasedVolTrading::StrategyName()) ==
               0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PriceBasedVolTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                   HFSAT::PricePairBasedAggressiveTrading::StrategyName()) == 0) {
      if (strategy_desc_.strategy_vec_[i].p_dep_market_view_->shortcode().compare("BR_WIN_0") == 0)
        strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PricePairBasedAggressiveTrading(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_),
            *(shortcode_smv_map_.GetSecurityMarketView("BR_IND_0")), *p_smart_order_manager_,
            strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
            economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
            modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
      else if (strategy_desc_.strategy_vec_[i].p_dep_market_view_->shortcode().compare("BR_IND_0") == 0)
        strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::PricePairBasedAggressiveTrading(
            dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_),
            *(shortcode_smv_map_.GetSecurityMarketView("BR_WIN_0")), *p_smart_order_manager_,
            strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
            economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
            strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
            modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    } else if (strategy_desc_.strategy_vec_[i].strategy_name_.compare(
                   HFSAT::ReturnsBasedAggressiveTrading::StrategyName()) == 0) {
      strategy_desc_.strategy_vec_[i].exec_ = new HFSAT::ReturnsBasedAggressiveTrading(
          dbglogger_, watch_, *(strategy_desc_.strategy_vec_[i].p_dep_market_view_), *p_smart_order_manager_,
          strategy_desc_.strategy_vec_[i].paramfilename_, livetrading_, p_strategy_param_sender_socket_,
          economic_events_manager_, strategy_desc_.strategy_vec_[i].trading_start_utc_mfm_,
          strategy_desc_.strategy_vec_[i].trading_end_utc_mfm_, strategy_desc_.strategy_vec_[i].runtime_id_,
          modelfilename_source_shortcode_vec_map_[strategy_desc_.strategy_vec_[i].modelfilename_]);
    }

    // Add as ModelMath target price listener
    if (strategy_desc_.strategy_vec_[i].exec_ != NULL) {
      base_model_math_->AddListener(strategy_desc_.strategy_vec_[i].exec_);
      strategy_desc_.strategy_vec_[i].exec_->SetModelMathComponent(base_model_math_);
      market_update_manager_.AddMarketDataInterruptedListener(strategy_desc_.strategy_vec_[i].exec_);

      // not applicable in SIM
      // if ( control_message_live_source_ != NULL )
      //   { // should be NULL for simtrading
      //     control_message_live_source_->AddControlMessageListener ( strategy_desc_.strategy_vec_[ i ].runtime_id_,
      //     strategy_desc_.strategy_vec_[i].exec_ ) ;
      //   }

      // TODO eventually we need to replace this by making riskmanager a listner of positions and SMV and exec a
      // listener or riskmanager
      if (sid_to_prom_order_manager_map_[security_id_] != NULL) {
        sid_to_prom_order_manager_map_[security_id_]->AddGlobalPositionChangeListener(
            strategy_desc_.strategy_vec_[i].exec_);
      }
    }
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
      if (sid_to_prom_order_manager_map_[t_sid_] != NULL) {
        {
          if (shortcode_ors_data_filesource_map_.find(shortcode_) != shortcode_ors_data_filesource_map_.end() &&
              shortcode_ors_data_filesource_map_[shortcode_]) {  // When using non-self for all products , we might
                                                                 // reach here for products we do not create
                                                                 // ors_data_sources for.

            // shortcode_ors_data_filesource_map_ [ shortcode_ ]->AddOrderNotFoundListener (
            // sid_to_prom_order_manager_map_ [ t_sid_ ] ); // PromOrderManager does not need to listen to reply of
            // NotFound from client replay requests ?
            shortcode_ors_data_filesource_map_[shortcode_]->AddOrderSequencedListener(
                sid_to_prom_order_manager_map_[t_sid_]);
            shortcode_ors_data_filesource_map_[shortcode_]->AddOrderConfirmedListener(
                sid_to_prom_order_manager_map_[t_sid_]);
            shortcode_ors_data_filesource_map_[shortcode_]->AddOrderConfCxlReplacedListener(
                sid_to_prom_order_manager_map_[t_sid_]);
            shortcode_ors_data_filesource_map_[shortcode_]->AddOrderCanceledListener(
                sid_to_prom_order_manager_map_[t_sid_]);
            shortcode_ors_data_filesource_map_[shortcode_]->AddOrderExecutedListener(
                sid_to_prom_order_manager_map_[t_sid_]);
            // shortcode_ors_data_filesource_map_ [ shortcode_ ]->AddOrderRejectedListener (
            // sid_to_prom_order_manager_map_ [ t_sid_ ] ); // PromOrderManager does not need to listen to reply of
            // Reject to client sendtrade messages which fail
          }
        }
      }
    }
  }

  // link up ModelMath to SMV and PromOrderManager . Doing this at the end so that everything is uptodate by the time
  // target_price_ is sent to strategy
  std::vector<HFSAT::BaseModelMath*> base_model_math_vec_;
  HFSAT::ModelCreator::GetModelMathVec(base_model_math_vec_);
  // for all the model_math objects
  for (auto i = 0u; i < base_model_math_vec_.size(); i++) {
    // for the securities that are ORS sources for indicators in this model attach the base_model_math_ as a listener to
    // the prom_order_manager of that sid_
    std::vector<std::string>& shortcodes_affecting_this_model_ =
        modelfilename_source_shortcode_vec_map_[base_model_math_vec_[i]->model_filename()];
    std::vector<std::string>& ors_source_needed_vec_ =
        modelfilename_ors_needed_by_indicators_vec_map_[base_model_math_vec_[i]->model_filename()];
    HFSAT::ModelCreator::LinkupModelMathToOnReadySources(base_model_math_vec_[i], shortcodes_affecting_this_model_,
                                                         ors_source_needed_vec_);
  }

  market_update_manager_.start();

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

  // start event loop
  try {
#define MINUTES_TO_PREP 30
    // To only process data starting MINUTES_TO_PREP minutes before min start time of strategies

    // In all the file sources, read events till
    // we reach the first event after the specified ttime_t
    HFSAT::ttime_t data_seek_time_ = strategy_desc_.GetMinStartTime() - HFSAT::ttime_t(MINUTES_TO_PREP * 60, 0);
    if (using_tmx_hist_data_) {
      data_seek_time_ = strategy_desc_.GetMinStartTime() - HFSAT::ttime_t(50 * 60, 0);
    }
    historical_dispatcher_.SeekHistFileSourcesTo(data_seek_time_);
#undef MINUTES_TO_PREP

    historical_dispatcher_.RunHist();
  } catch (int e) {
  }

  // TODO check for strategies with open positions and report the same

  // setup the result reporting screen
  for (auto i = 0u; i < strategy_desc_.strategy_vec_.size(); i++) {
    if (strategy_desc_.strategy_vec_[i].exec_ != NULL) {
      strategy_desc_.strategy_vec_[i].exec_->ReportResults(trades_writer_);
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
    }
  }

#endif  // CCPROFILING

  return 0;
}
