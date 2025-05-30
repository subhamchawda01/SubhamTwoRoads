#include "dvctrade/ExecLogic/nse_exec_logic_order_reader_from_file.hpp"
#include "dvctrade/ExecLogic/nse_exec_logic_order_reader_from_TCP.hpp"
#include "dvctrade/ExecLogic/nse_rv_strategy_exec_logic.hpp"
#include "dvctrade/ExecLogic/nse_trade_given_notional_exec_logic.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/SimMarketMaker/price_level_sim_market_maker.hpp"
#include "baseinfra/SimMarketMaker/sim_market_maker_helper.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "baseinfra/SimPnls/sim_base_pnl.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/signals.hpp"

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

void InitTradesLogger(int tradingdate, bool livetrading, HFSAT::BulkFileWriter& trades_writer_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/tradelogs/nse_simple_logic_trades." << tradingdate;
  std::string tradesfilename = t_temp_oss_.str();
  // open trades file in append mode in livetrading_
  trades_writer_.Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));
}

std::vector<NSE_SIMPLEEXEC::ParamSet*> GetMultParams(const std::string& _paramfilename_) {
  std::vector<NSE_SIMPLEEXEC::ParamSet*> mult_params;
  std::ifstream paramfile_;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);

      std::string str(readline_buffer_);
      boost::trim(str);
      if (str == "") continue;

      mult_params.push_back(new NSE_SIMPLEEXEC::ParamSet(str.c_str()));
    }
  }
  return (mult_params);
}

int main(int argc, char** argv) {
  signal(SIGINT, termHandler);
  signal(SIGPIPE, SIG_IGN);

  // check command line arguments
  if (argc != 4) {
    std::cerr << " usage : Input param_file orders_file date" << std::endl;
    exit(0);
  }

  // Variable Initializations
  bool livetrading_ = false;
  std::string filename_input(argv[1]);
  std::string orders_file(argv[2]);
  int tradingdate_ = atoi(argv[3]);

  HFSAT::BulkFileWriter trades_writer_(256 * 1024);  // 256KB
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/alllogs/nse_simple_exec_logic_simlog." << tradingdate_;
  std::string logfilename_ = t_temp_oss_.str();
  unsigned int runtime_id_ = 1000;  // temporary, not sure

  /////////NSE_EXEC_LOGIC_PARAMSET///////////
  std::vector<NSE_SIMPLEEXEC::ParamSet*> params;
  params = GetMultParams(filename_input);

  // Initialize trades logger
  InitTradesLogger(tradingdate_, livetrading_, trades_writer_);

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

  // Create file reader interface
  // In sim mode, we only use file based order reader
  NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReader* order_reader_interface_ =
      new NSE_SIMPLEEXEC::SimpleNseExecLogicOrderReaderFromFile(watch_, dbglogger_, livetrading_, orders_file);

  // Init secnameIndexer
  for (auto i = 0u; i < params.size(); i++) {
    NSE_SIMPLEEXEC::ParamSet* param_ = params[i];
    param_->yyyymmdd_ = tradingdate_;  // setting date here doesnt requires to change everytime in every param
    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(param_->instrument_);
    // add shortcodes to sec_name_indexer
    sec_name_indexer_.AddString(exchange_symbol_, param_->instrument_);
  }

  // create smvs
  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_ctr_);
    const char* _this_exch_symbol_ = sec_name_indexer_.GetSecurityNameFromId(t_ctr_);
    HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exch_symbol_, 0, HFSAT::kExchSourceNSE, true,
        DEFAULT_OFFLINEMIXMMS_FILE, DEFAULT_ONLINE_MIX_PRICE_FILE, DEFAULT_ONLINE_BETA_KALMAN_FILE);
    sid_to_smv_ptr_map_.push_back(p_smv_);
    if (p_smv_->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_)
      p_smv_->InitializeSMVForIndexedBook();
  }

  // create book manager
  HFSAT::IndexedNSEMarketViewManager* indexed_nse_market_view_manager_ =
      new HFSAT::IndexedNSEMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);

  // create logged message filesources and add hooks
  HFSAT::HistoricalDispatcher historical_dispatcher_;
  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {
    // don't trade banned products
    if (IsInstrumentBanned(sec_name_indexer_.GetShortcodeFromId(t_ctr_), list_of_securities_under_ban, dbglogger_)) {
      continue;
    }

    const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(params[t_ctr_]->instrument_);
    HFSAT::NSELoggedMessageFileSource* t_filesource_ = new HFSAT::NSELoggedMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, t_ctr_, exchange_symbol_, HFSAT::kTLocNSE);
    t_filesource_->SetExternalTimeListener(&watch_);
    t_filesource_->SetOrderGlobalListenerNSE(indexed_nse_market_view_manager_);
    historical_dispatcher_.AddExternalDataListener(t_filesource_);

    // create Order Manager, Trader and SimTrader; set SimTrader hooks
    HFSAT::SimTimeSeriesInfo sim_time_series_info_(sec_name_indexer_.NumSecurityId());
    unsigned int t_market_model_index_ = t_ctr_;
    HFSAT::BaseTrader* sim_trader_ = NULL;
    HFSAT::SmartOrderManager* smart_order_manager_ = NULL;
    sim_time_series_info_.sid_to_sim_config_.resize(0);
    sim_time_series_info_.sid_to_sim_config_.push_back(
        HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, params[t_ctr_]->instrument_, "invalid"));

    HFSAT::PriceLevelSimMarketMaker* sim_market_maker_ = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
        dbglogger_, watch_, *(sid_to_smv_ptr_map_[t_ctr_]), t_market_model_index_, sim_time_series_info_);
    sim_market_maker_->SubscribeL2Events(*(sid_to_smv_ptr_map_[t_ctr_]));

    sim_trader_ = HFSAT::SimTraderHelper::GetSimTrader("12345678", sim_market_maker_);
    smart_order_manager_ = new HFSAT::SmartOrderManager(dbglogger_, watch_, sec_name_indexer_, *sim_trader_,
                                                        *(sid_to_smv_ptr_map_[t_ctr_]), 1234, false, 1);

    sim_market_maker_->AddOrderNotFoundListener(smart_order_manager_);
    sim_market_maker_->AddOrderSequencedListener(smart_order_manager_);
    sim_market_maker_->AddOrderConfirmedListener(smart_order_manager_);
    sim_market_maker_->AddOrderConfCxlReplaceRejectedListener(smart_order_manager_);
    sim_market_maker_->AddOrderConfCxlReplacedListener(smart_order_manager_);
    sim_market_maker_->AddOrderCanceledListener(smart_order_manager_);
    sim_market_maker_->AddOrderExecutedListener(smart_order_manager_);
    sim_market_maker_->AddOrderRejectedListener(smart_order_manager_);

    // create simpnl object
    HFSAT::SimBasePNL* p_sim_base_pnl_ = new HFSAT::SimBasePNL(
        dbglogger_, watch_, *(sid_to_smv_ptr_map_[t_ctr_]), runtime_id_, trades_writer_);
    smart_order_manager_->SetBasePNL(p_sim_base_pnl_);

    // Create simple exec based on run type specified in param
    NSE_SIMPLEEXEC::SimpleNseExecLogic* simple_nse_exec_logic_ = NULL;
    switch (params[t_ctr_]->exec_logic_run_type_) {
      case NSE_SIMPLEEXEC::kGetOrdersFromStrategy: {
        simple_nse_exec_logic_ =
            new NSE_SIMPLEEXEC::NseRVStrategyExecLogic(*(sid_to_smv_ptr_map_[t_ctr_]), sim_trader_,
                                                       smart_order_manager_, dbglogger_, watch_, params[t_ctr_], false);
        watch_.subscribe_BigTimePeriod(order_reader_interface_);
        order_reader_interface_->SubscribeNewOrders(params[t_ctr_]->instrument_, simple_nse_exec_logic_);
        break;
      }
      case NSE_SIMPLEEXEC::kDecideOrdersFromParamConstraints: {
        simple_nse_exec_logic_ = new NSE_SIMPLEEXEC::NseTradeGivenNotionalExecLogic(
            *(sid_to_smv_ptr_map_[t_ctr_]), sim_trader_, smart_order_manager_, dbglogger_, watch_, params[t_ctr_],
            false);
        break;
      }
      default: {
        dbglogger_ << "Invalid execlogic run type. Please check config." << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }
    }

    sid_to_smv_ptr_map_[t_ctr_]->subscribe_tradeprints(simple_nse_exec_logic_);
    sid_to_smv_ptr_map_[t_ctr_]->subscribe_rawtradeprints(simple_nse_exec_logic_);
    sid_to_smv_ptr_map_[t_ctr_]->subscribe_price_type(simple_nse_exec_logic_, HFSAT::kPriceTypeMktSizeWPrice);
    smart_order_manager_->AddExecutionListener(simple_nse_exec_logic_);
    sim_market_maker_->AddOrderRejectedListener(simple_nse_exec_logic_);
  }
  historical_dispatcher_.RunHist();
  trades_writer_.Close();
  return 0;
}
