#include <getopt.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvctrade/InitCommon/nse_spread_logic_param.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"
#include "baseinfra/SimMarketMaker/price_level_sim_market_maker.hpp"
#include "baseinfra/SimMarketMaker/sim_config.hpp"
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
#include "dvctrade/SpreadTrading/spread_trader.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"
#include "dvccode/Utils/holiday_manager.hpp"

#include "basetrade/hftrap/MinuteBarDispatcher/minute_bar_dispatcher.hpp"

#define EXPIRY_EXEC "/home/dvctrader/basetrade_install/bin/get_expiry_nse"

#define KALMAN_INIT_EXEC "$HOME/basetrade/hftrap/Scripts/get_initial_kalman_params.pl"
#define KALMAN_EXEC "$HOME/basetrade_install/bin/kalman_exec"

void print_usage(const char* prg_name) {
  printf(" This is the Spread Trading Executable \n");
  printf(
      " Usage:%s --paramfile <pfile> --start_date <start_YYYYMMDD> --end_date <end_YYYYMMDD> --logfile <lfile> "
      "--instrument_1 <ticker> --instrument_2 <ticker2>\n",
      prg_name);
  printf(" --paramfile complete path of file with strategy params \n");
  printf(" --start_date  start_date of test run \n");
  printf(" --end_date end_date of test run\n");
  printf(" --trade_start_date date from which to start trading\n");
  printf(" --serialized_file (name of the file where we want to save/load serialize data)\n");
  printf(" --notify_last_event\n");
  printf(" --hft_data\n");
  printf(" --save_state\n");
  printf(" --load_state\n");
  printf(" --live\n");
  printf(" --use_exec_logic\n");
  printf(" --use_adjusted_data\n");
}

std::vector<std::string> source_shortcode_vec_;
HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
bool banned_products_listed_ = false;
std::set<std::string> list_of_securities_under_ban_;
std::map<std::string, double> corp_adj_map_;

static struct option data_options[] = {{"help", no_argument, 0, 'h'},
                                       {"start_date", required_argument, 0, 'c'},
                                       {"end_date", required_argument, 0, 'd'},
                                       {"trade_start_date", required_argument, 0, 't'},
                                       {"paramfile", required_argument, 0, 'e'},
                                       {"multiparam", no_argument, 0, 'f'},
                                       {"save_state", no_argument, 0, 'j'},
                                       {"notify_last_event", no_argument, 0, 'i'},
                                       {"hft_data", no_argument, 0, 'l'},
                                       {"load_state", no_argument, 0, 'm'},
                                       {"live", no_argument, 0, 'g'},
                                       {"use_exec_logic", no_argument, 0, 'n'},
                                       {"use_adjusted_data", no_argument, 0, 'p'},
                                       {"corp_adj_file", required_argument, 0, 'q'},
                                       {0, 0, 0, 0}};

std::string GetFilename(MT_SPRD::ParamSet* param_, std::string process, int run_date, bool live) {
  if (!live)
    return (NSE_HFTRAP_SERIALIZED_DATAFILES_DIR + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" +
            param_->param_id);

  int file_date = run_date;
  std::string filename;
  boost::to_upper(process);
  if (process == std::string("LOAD"))
    file_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr,
                                                                          run_date, true);

  filename = NSE_HFTRAP_SERIALIZED_DATAFILES_DIR + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" +
             param_->param_id + "_" + std::to_string(file_date);

  return (filename);
}

void save_state(MT_SPRD::SpreadTrader& spread_trader, int run_date, bool live) {
  MT_SPRD::ParamSet params = spread_trader.GetParams();
  std::string filename = GetFilename(&params, std::string("save"), run_date, live);
  std::ofstream ofs(filename);

  if (ofs.good()) {
    boost::archive::text_oarchive oa(ofs);
    oa << spread_trader;
    std::cout << "Serialised file saved for " << run_date << "\t" << params.instrument_1_ << "_" << params.instrument_2_
              << "_" << params.param_id << "\n";
  } else {
    std::cout << "File creation failed! " << filename << "\n";
  }
}

void load_state(MT_SPRD::SpreadTrader& spread_trader, int run_date, bool live) {
  MT_SPRD::ParamSet params = spread_trader.GetParams();
  std::string filename = GetFilename(&params, std::string("load"), run_date, live);
  // open the archive
  std::ifstream ifs(filename);
  if (ifs.good()) {
    boost::archive::text_iarchive ia(ifs);
    ia >> spread_trader;
    std::cout << "Serialised file loaded for " << run_date << "\t" << params.instrument_1_ << "_"
              << params.instrument_2_ << "_" << params.param_id << "\n";
  } else {
    std::cout << "Serialized file does not exist!" << filename << "\nExiting...\n";
    exit(-1);
  }
}

void AddSingleInstToIndexer(std::string t_str_) {
  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(t_str_);
  sec_name_indexer_.AddString(exchange_symbol_, t_str_);
  source_shortcode_vec_.push_back(t_str_);
}

std::vector<MT_SPRD::ParamSet*> GetMultParams(const std::string& _paramfilename_) {
  std::vector<MT_SPRD::ParamSet*> mult_params;
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

      mult_params.push_back(new MT_SPRD::ParamSet(str.c_str()));
    }
  }
  return (mult_params);
}

void LoadNSEFOSecuritiesUnderBan() {
  list_of_securities_under_ban_.clear();
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_"
             << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".csv";

  std::ifstream fo_banned_securities_stream;
  fo_banned_securities_stream.open(t_temp_oss.str().c_str(), std::ifstream::in);

  char line_buffer[1024];

  while (fo_banned_securities_stream.good()) {
    fo_banned_securities_stream.getline(line_buffer, 1024);
    if (std::string(line_buffer).length() < 1) continue;
    list_of_securities_under_ban_.insert(line_buffer);

    if (false == banned_products_listed_) {
      std::cout << "SECURITY : " << line_buffer << " IS UNDER BAN FOR TODAY\n";
    }
  }

  fo_banned_securities_stream.close();
  banned_products_listed_ = true;
}

bool IsInstrumentBanned(std::string instrument) {
  if (false == banned_products_listed_) {
    LoadNSEFOSecuritiesUnderBan();
  }

  if (list_of_securities_under_ban_.end() != list_of_securities_under_ban_.find(instrument)) {
    std::cout << instrument << " cant be traded as it is under ban today\n";
    return true;
  }

  return false;
}

void LoadAdjustmentFactor(int current_date, std::string corp_adj_file, HFSAT::DebugLogger& dbglogger_) {
  std::ifstream cafile;
  cafile.open(corp_adj_file.c_str(), std::ifstream::in);
  if (cafile.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];

    while (cafile.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      cafile.getline(readline_buffer_, kParamFileLineBufferLen);

      char* date_str = strtok(readline_buffer_, " \t\n");
      if (date_str == NULL) continue;
      int ca_date = atoi(date_str);

      if (ca_date != current_date) continue;

      std::string ca_instrument = std::string(strtok(NULL, " \t\n"));
      double adjustment_factor = atof(strtok(NULL, " \t\n"));
      corp_adj_map_[ca_instrument] = adjustment_factor;
      dbglogger_ << "Today is Corporate Adjustment for " << ca_instrument << " " << adjustment_factor << "\n";
    }
  }
}

int main(int argc, char* argv[]) {
  int c;  // getopt argument
  int hflag = 0;
  std::string paramfilename_ = "";
  std::string logfilename_ = "";
  std::string corp_adj_file = "";
  int start_date_ = 0;
  int end_date_ = 0;
  int trade_start_date = 0;
  std::string ticker1_ = "";
  std::string ticker2_ = "";
  bool is_notify_last_event = false;
  bool save_state_ = false;
  bool load_state_ = false;
  bool multiparam = false;
  // All serialized datafiles will be stored/load from this dir, with given filenames
  std::string serialize_filename = NSE_HFTRAP_SERIALIZED_DATAFILES_DIR;
  bool is_hft_data_ = false;
  bool use_exec_logic_ = false;
  std::string trade_fname_ = "";
  bool use_adjusted_data_ = false;
  bool live = false;

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case 'e':
        paramfilename_ = optarg;
        break;

      case 'f':
        multiparam = true;
        break;

      case 'c':
        start_date_ = atoi(optarg);
        break;

      case 'd':
        end_date_ = atoi(optarg);
        break;

      case 't':
        trade_start_date = atoi(optarg);
        break;

      case 'i':
        is_notify_last_event = true;
        break;

      case 'j':
        save_state_ = true;
        break;

      case 'g':
        live = true;
        break;

      case 'l':
        is_hft_data_ = true;
        break;

      case 'm':
        load_state_ = true;
        break;

      case 'n':
        use_exec_logic_ = true;
        break;

      case 'p':
        use_adjusted_data_ = true;
        break;

      case 'q':
        corp_adj_file = optarg;
        break;

      case '?':
        if (optopt == 'b' || optopt == 'c' || optopt == 'd' || optopt == 'e' || optopt == 'o') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag) {
    print_usage(argv[0]);
    exit(-1);
  }

  if (start_date_ == 0 || end_date_ == 0 || paramfilename_.empty()) {
    print_usage(argv[0]);
    exit(-1);
  }

  if (trade_start_date == 0) {
    trade_start_date = start_date_;
  }

  struct tm start_timeinfo = {0};

  start_timeinfo.tm_year = (trade_start_date / 10000) - 1900;
  start_timeinfo.tm_mon = ((trade_start_date / 100) % 100) - 1;
  start_timeinfo.tm_mday = trade_start_date % 100;

  // Trade Start Time
  time_t trade_start_time = mktime(&start_timeinfo);

  std::vector<MT_SPRD::ParamSet*> params;
  std::vector<MT_SPRD::ParamSet*> params_next_month_;
  if (multiparam) {
    params = GetMultParams(paramfilename_);
    params_next_month_ = GetMultParams(paramfilename_);
  } else {
    params.push_back(new MT_SPRD::ParamSet(paramfilename_));
    params_next_month_.push_back(new MT_SPRD::ParamSet(paramfilename_));
  }

  if (!is_hft_data_) {
    HFSAT::MinuteBarDispatcher& mbar_dispatcher = HFSAT::MinuteBarDispatcher::GetUniqueInstance();

    // Ask dispatcher to notify us when all data is consumed
    if (true == is_notify_last_event) mbar_dispatcher.RequestNotifyOnLastEvent();

    // default segment F&O
    char segment = NSE_FUTOPT_SEGMENT;
    if (true == use_adjusted_data_) segment = NSE_FUTOPT_ADJUSTED_SEGMENT;

    std::vector<MT_SPRD::SpreadTrader*> spread_traders;
    for (auto i = 0u; i < params.size(); i++) {
      MT_SPRD::ParamSet* param_;
      param_ = params[i];

      // create debuglogger
      logfilename_ =
          NSE_HFTRAP_DEBUG_LOGS + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" + param_->param_id;
      HFSAT::DebugLogger dbglogger_(256 * 1024, 1);
      dbglogger_.OpenLogFile(logfilename_.c_str(), std::ios_base::app);

      int unique_id;
      unique_id = mbar_dispatcher.getUniqueId(param_->instrument_1_);
      if (unique_id != -1) {
        HFSAT::MidTermFileSource* t_filesource_ =
            new HFSAT::MidTermFileSource(dbglogger_, param_->instrument_1_.c_str(), segment, "0,1");
        mbar_dispatcher.AddExternalDataListener(t_filesource_);
      }
      unique_id = mbar_dispatcher.getUniqueId(param_->instrument_2_);
      if (unique_id != -1) {
        HFSAT::MidTermFileSource* t_filesource_ =
            new HFSAT::MidTermFileSource(dbglogger_, param_->instrument_2_.c_str(), segment, "0,1");
        mbar_dispatcher.AddExternalDataListener(t_filesource_);
      }

      // Add instrument_ids to the spreadTrader params
      param_->inst1_id = 2 * i + 1;
      param_->inst2_id = 2 * i + 2;

      bool ban = false;

      spread_traders.push_back(new MT_SPRD::SpreadTrader(param_, param_, dbglogger_, NULL, NULL, false,
                                                         use_adjusted_data_, trade_start_time, live, ban));
      MT_SPRD::SpreadTrader* spread_trader_ = spread_traders[i];

      mbar_dispatcher.SubscribeStrategy(spread_traders[i]);

      if (load_state_) {
        std::cout << "RUNNING IN LOAD MODE" << std::endl;
        load_state(*spread_trader_, start_date_, live);

        if (param_->spread_comp_mode_ == 2) {
          std::vector<double> x_, P_, Q_, R_;
          spread_trader_->LoadKalmanState(x_, P_, Q_, R_);
          spread_trader_->InitializeKalman(x_, P_, Q_, R_);
        }
      } else {
        if (param_->spread_comp_mode_ == 2) {
          // initial setup of kalman params
          // create temp logfile for kalman script
          std::string t_logfile_ = "/spare/local/dvctrader/file.temp.ks";
          char t_cmdline_[1024];
          sprintf(t_cmdline_, "%s %d %d %s %s %s 50 %d %d %s %s", KALMAN_INIT_EXEC, start_date_, start_date_,
                  param_->instrument_1_.c_str(), param_->instrument_2_.c_str(), t_logfile_.c_str(),
                  param_->zscore_vec_len_, param_->asset_comp_mode_, KALMAN_EXEC,
                  (use_adjusted_data_ ? "use_adjusted_data" : ""));
          std::cout << t_cmdline_ << "\n";
          FILE* prg_output = popen(t_cmdline_, "r");
          char* line = NULL;
          size_t len = 0;
          getline(&line, &len, prg_output);
          HFSAT::PerishableStringTokenizer pst = HFSAT::PerishableStringTokenizer(line, len);
          std::vector<char const*> const& tokens = pst.GetTokens();
          std::vector<double> x_;
          std::vector<double> P_;
          std::vector<double> Q_;
          std::vector<double> R_;
          x_.resize(2);
          x_[0] = atof(tokens[7]);
          x_[1] = atof(tokens[8]);

          P_.resize(4);
          P_[0] = atof(tokens[10]);
          P_[1] = atof(tokens[11]);
          P_[2] = atof(tokens[12]);
          P_[3] = atof(tokens[13]);

          Q_.resize(4);
          Q_[0] = atof(tokens[15]);
          Q_[1] = atof(tokens[16]);
          Q_[2] = atof(tokens[17]);
          Q_[3] = atof(tokens[18]);

          R_.resize(1);
          R_[0] = atof(tokens[20]);
          pclose(prg_output);

          spread_trader_->InitializeKalman(x_, P_, Q_, R_);
        }
      }
      // call function to load adf/halflife stats from DB
      spread_trader_->LoadStatMaps(start_date_, end_date_);
      spread_trader_->LoadEarningsMaps();

      if (i == (params.size() - 1)) mbar_dispatcher.RunHist(start_date_, end_date_);
    }

    // save all files
    if (save_state_) {
      std::cout << "RUNNING IN SAVE MODE" << std::endl;

      for (unsigned int i = 0; i < spread_traders.size(); i++) {
        MT_SPRD::SpreadTrader* spread_trader_ = spread_traders[i];
        MT_SPRD::ParamSet param_ = spread_trader_->GetParams();

        if (param_.spread_comp_mode_ == 2) spread_trader_->SaveKalmanState();

        save_state(*spread_trader_, end_date_, live);
      }
    }
  } else {
    if (HFSAT::HolidayManager::GetUniqueInstance().IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr,
                                                                     start_date_, true)) {
      std::cout << "Exchange is closed on " << start_date_ << ". Exiting!\n";
      exit(-1);
    }

    HFSAT::HistoricalDispatcher historical_dispatcher_;
    if (start_date_ != end_date_) {
      std::cout << "In HFT mode, a single run can only be for a single day .. Exiting.\n";
      exit(-1);
    }

    // create debuglogger
    logfilename_ = std::string(NSE_HFTRAP_DEBUG_LOGS) + "LOG_" + std::to_string(start_date_);
    HFSAT::DebugLogger dbglogger_(256 * 1024, 1);
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ios_base::app);

    // Load corporate action map
    if (corp_adj_file != "") {
      LoadAdjustmentFactor(start_date_, corp_adj_file, dbglogger_);
    }

    HFSAT::Watch watch_(dbglogger_, start_date_);
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(start_date_);
    HFSAT::SecurityDefinitions::GetUniqueInstance(start_date_).LoadNSESecurityDefinitions();
    HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();

    for (unsigned int i = 0; i < params.size(); i++) {
      MT_SPRD::ParamSet* param_ = params[i];

      // add relevant shortcodes to source shortcode vec
      std::ostringstream oss_;
      oss_ << "NSE_" << param_->instrument_1_ << "_FUT0";
      AddSingleInstToIndexer(oss_.str());
      oss_.str("");
      oss_ << "NSE_" << param_->instrument_1_ << "_FUT1";
      AddSingleInstToIndexer(oss_.str());
      oss_.str("");
      oss_ << "NSE_" << param_->instrument_2_ << "_FUT0";
      AddSingleInstToIndexer(oss_.str());
      oss_.str("");
      oss_ << "NSE_" << param_->instrument_2_ << "_FUT1";
      AddSingleInstToIndexer(oss_.str());
    }

    // create SMVs
    for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_ctr_);
      const char* _this_exch_symbol_ = sec_name_indexer_.GetSecurityNameFromId(t_ctr_);
      HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
          dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exch_symbol_, t_ctr_, HFSAT::kExchSourceNSE,
          true, DEFAULT_OFFLINEMIXMMS_FILE, DEFAULT_ONLINE_MIX_PRICE_FILE, DEFAULT_ONLINE_BETA_KALMAN_FILE);
      sid_to_smv_ptr_map_.push_back(p_smv_);
      if (p_smv_->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_)
        p_smv_->InitializeSMVForIndexedBook();
    }
    // create market book manager
    HFSAT::IndexedNSEMarketViewManager* indexed_nse_market_view_manager_ =
        new HFSAT::IndexedNSEMarketViewManager(dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);

    // create logged message filesources and add hooks
    std::map<int, HFSAT::NSELoggedMessageFileSource*> secid_to_fsource_map_;

    for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {
      std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_ctr_);
      const char* _this_exch_symbol_ = sec_name_indexer_.GetSecurityNameFromId(t_ctr_);
      HFSAT::NSELoggedMessageFileSource* t_filesource_ = new HFSAT::NSELoggedMessageFileSource(
          dbglogger_, sec_name_indexer_, start_date_, t_ctr_, _this_exch_symbol_, HFSAT::kTLocNSE);
      t_filesource_->SetExternalTimeListener(&watch_);
      t_filesource_->SetOrderGlobalListenerNSE(indexed_nse_market_view_manager_);
      secid_to_fsource_map_[t_ctr_] = t_filesource_;
      historical_dispatcher_.AddExternalDataListener(t_filesource_);
    }

    HFSAT::SimTimeSeriesInfo sim_time_series_info_(sec_name_indexer_.NumSecurityId());
    unsigned int t_market_model_index_ = 0u;
    std::vector<HFSAT::BaseSimMarketMaker*> smm_vector_;
    std::vector<HFSAT::BaseTrader*> sim_trader_vector_;
    std::vector<HFSAT::SmartOrderManager*> som_vector_;

    if (use_exec_logic_) {
      for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer_.NumSecurityId(); t_ctr_++) {
        std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(t_ctr_);
        sim_time_series_info_.sid_to_sim_config_.push_back(
            HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, _this_shortcode_, "invalid"));
      }

      std::vector<int> sec_ids;
      for (unsigned int t_ctr_ = 0; t_ctr_ < source_shortcode_vec_.size(); t_ctr_++) {
        int sid = sec_name_indexer_.GetIdFromString(source_shortcode_vec_[t_ctr_]);
        sec_ids.push_back(sid);
      }

      for (unsigned int t_ctr_ = 0; t_ctr_ < sec_ids.size(); t_ctr_++) {
        int sid = sec_ids[t_ctr_];
        HFSAT::PriceLevelSimMarketMaker* plsmm = HFSAT::PriceLevelSimMarketMaker::GetUniqueInstance(
            dbglogger_, watch_, *(sid_to_smv_ptr_map_[sid]), t_market_model_index_, sim_time_series_info_);
        plsmm->SubscribeL2Events(*(sid_to_smv_ptr_map_[sid]));
        smm_vector_.push_back(plsmm);

        HFSAT::BaseTrader* p_base_trader_ = HFSAT::SimTraderHelper::GetSimTrader("12345678", plsmm);
        HFSAT::SmartOrderManager* p_som_ = new HFSAT::SmartOrderManager(
            dbglogger_, watch_, sec_name_indexer_, *p_base_trader_, *(sid_to_smv_ptr_map_[sid]), 1234, false, 1);
        som_vector_.push_back(p_som_);

        secid_to_fsource_map_[sid]->AddExternalDataListenerListener(plsmm);
        int saci = p_som_->server_assigned_client_id_;
        plsmm->AddSecIdToSACI(saci, sid);

        plsmm->AddOrderNotFoundListener(p_som_);
        plsmm->AddOrderSequencedListener(p_som_);
        plsmm->AddOrderConfirmedListener(p_som_);
        plsmm->AddOrderConfCxlReplaceRejectedListener(p_som_);
        plsmm->AddOrderConfCxlReplacedListener(p_som_);
        plsmm->AddOrderCanceledListener(p_som_);
        plsmm->AddOrderExecutedListener(p_som_);
        plsmm->AddOrderRejectedListener(p_som_);
      }
    }

    std::vector<MT_SPRD::SpreadTrader*> spread_traders;
    for (unsigned int i = 0; i < params.size(); i++) {
      MT_SPRD::ParamSet* param_ = params[i];
      MT_SPRD::ParamSet* param_next_month_ = params_next_month_[i];

      MT_SPRD::SpreadExecLogic* sprd_exec_front_month_ = NULL;
      MT_SPRD::SpreadExecLogic* sprd_exec_next_month_ = NULL;

      int uts1 = param_->unit_trade_size_1_;
      int uts2 = param_->unit_trade_size_2_;

      int on_pos1 = param_->overnight_position_1;
      int on_pos2 = param_->overnight_position_2;
      bool override_on_pos = param_->override_on_pos;

      bool is_flat = param_->get_flat;

      bool ban = false;

      // set up sim market maker, om etc if spread exec is to be used
      if (use_exec_logic_) {
        int fmsid_pass, fmsid_aggr, smsid_pass, smsid_aggr, fmom_pass, fmom_aggr, smom_pass, smom_aggr;
        if (param_->is_inst1_pass_) {
          fmsid_pass = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_1_ + "_FUT0");
          fmsid_aggr = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_2_ + "_FUT0");
          smsid_pass = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_1_ + "_FUT1");
          smsid_aggr = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_2_ + "_FUT1");
          fmom_pass = 4 * i;
          smom_pass = 4 * i + 1;
          fmom_aggr = 4 * i + 2;
          smom_aggr = 4 * i + 3;
        } else {
          fmsid_pass = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_2_ + "_FUT0");
          fmsid_aggr = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_1_ + "_FUT0");
          smsid_pass = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_2_ + "_FUT1");
          smsid_aggr = sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_1_ + "_FUT1");
          fmom_pass = 4 * i + 2;
          smom_pass = 4 * i + 3;
          fmom_aggr = 4 * i;
          smom_aggr = 4 * i + 1;
        }
        if (live) {
          trade_fname_ = NSE_HFTRAP_TRADE_EXEC + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" +
                         param_->param_id + "_" + std::to_string(start_date_);
        } else {
          trade_fname_ =
              NSE_HFTRAP_TRADE_EXEC + param_->instrument_1_ + "_" + param_->instrument_2_ + "_" + param_->param_id;
        }

        // set up trades writer for tradefile
        HFSAT::BulkFileWriter* trades_file_ =
            new HFSAT::BulkFileWriter(trade_fname_.c_str(), 131072, std::ios::out | std::ios::app);

        // correct lot size in params for spread_exec_logic
        param_->SetContractLotSizes(
            HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_1_ + "_FUT0", start_date_),
            HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_2_ + "_FUT0", start_date_));
        param_next_month_->SetContractLotSizes(
            HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_1_ + "_FUT1", start_date_),
            HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_2_ + "_FUT1", start_date_));

        // set up spread_exec_logic instances
        sprd_exec_front_month_ =
            new MT_SPRD::SpreadExecLogic(dbglogger_, *trades_file_, watch_, fmsid_pass, fmsid_aggr,
                                         *(sid_to_smv_ptr_map_[fmsid_pass]), *(sid_to_smv_ptr_map_[fmsid_aggr]),
                                         *(som_vector_[fmom_pass]), *(som_vector_[fmom_aggr]), 0, 0, param_, live, ban);
        sprd_exec_next_month_ = new MT_SPRD::SpreadExecLogic(
            dbglogger_, *trades_file_, watch_, smsid_pass, smsid_aggr, *(sid_to_smv_ptr_map_[smsid_pass]),
            *(sid_to_smv_ptr_map_[smsid_aggr]), *(som_vector_[smom_pass]), *(som_vector_[smom_aggr]), 0, 0,
            param_next_month_, live, ban);

        sid_to_smv_ptr_map_[fmsid_pass]->subscribe_price_type(sprd_exec_front_month_, HFSAT::kPriceTypeMktSizeWPrice);
        sid_to_smv_ptr_map_[fmsid_aggr]->subscribe_price_type(sprd_exec_front_month_, HFSAT::kPriceTypeMktSizeWPrice);
        sid_to_smv_ptr_map_[smsid_pass]->subscribe_price_type(sprd_exec_next_month_, HFSAT::kPriceTypeMktSizeWPrice);
        sid_to_smv_ptr_map_[smsid_aggr]->subscribe_price_type(sprd_exec_next_month_, HFSAT::kPriceTypeMktSizeWPrice);
      }

      // set up hooks for spread_exec
      MT_SPRD::SpreadTrader* spread_trader_ =
          new MT_SPRD::SpreadTrader(param_, param_next_month_, dbglogger_, sprd_exec_front_month_,
                                    sprd_exec_next_month_, param_->is_inst1_pass_, false, trade_start_time, live, ban);
      spread_traders.push_back(spread_trader_);

      if (load_state_) {
        load_state(*spread_trader_, start_date_, live);
        if (sprd_exec_front_month_) {
          spread_trader_->SetExecClasses(sprd_exec_front_month_, sprd_exec_next_month_);
          spread_trader_->SetSpreadExecVars();
        }

        if (param_->spread_comp_mode_ == 2) {
          std::vector<double> x_, P_, Q_, R_;
          spread_trader_->LoadKalmanState(x_, P_, Q_, R_);
          spread_trader_->InitializeKalman(x_, P_, Q_, R_);
        }

        // if there is corporate adjustement, adjust previously loaded data
        if (corp_adj_map_.find(param_->instrument_1_) != corp_adj_map_.end() ||
            corp_adj_map_.find(param_->instrument_2_) != corp_adj_map_.end()) {
          double adj_factor_1 = (corp_adj_map_.find(param_->instrument_1_) != corp_adj_map_.end())
                                    ? corp_adj_map_[param_->instrument_1_]
                                    : 1;
          double adj_factor_2 = (corp_adj_map_.find(param_->instrument_2_) != corp_adj_map_.end())
                                    ? corp_adj_map_[param_->instrument_2_]
                                    : 1;
          dbglogger_ << "Calling OnCorporateAction with: " << adj_factor_1 << ", " << adj_factor_2 << "\n";
          spread_trader_->OnCorporateAction(adj_factor_1, adj_factor_2);
        }
      } else {
        if (param_->spread_comp_mode_ == 2) {
          // initial setup of kalman params
          // create temp logfile for kalman script
          std::string t_logfile_ = "/spare/local/dvctrader/file.temp.ks";
          char t_cmdline_[1024];
          sprintf(t_cmdline_, "%s %d %d %s %s %s 50 %d %d %s %s", KALMAN_INIT_EXEC, start_date_, start_date_,
                  param_->instrument_1_.c_str(), param_->instrument_2_.c_str(), t_logfile_.c_str(),
                  param_->zscore_vec_len_, param_->asset_comp_mode_, KALMAN_EXEC,
                  (use_adjusted_data_ ? "use_adjusted_data" : ""));
          std::cout << t_cmdline_ << "\n";
          FILE* prg_output = popen(t_cmdline_, "r");
          char* line = NULL;
          size_t len = 0;
          getline(&line, &len, prg_output);
          HFSAT::PerishableStringTokenizer pst = HFSAT::PerishableStringTokenizer(line, len);
          std::vector<char const*> const& tokens = pst.GetTokens();
          std::vector<double> x_;
          std::vector<double> P_;
          std::vector<double> Q_;
          std::vector<double> R_;
          x_.resize(2);
          x_[0] = atof(tokens[7]);
          x_[1] = atof(tokens[8]);

          P_.resize(4);
          P_[0] = atof(tokens[10]);
          P_[1] = atof(tokens[11]);
          P_[2] = atof(tokens[12]);
          P_[3] = atof(tokens[13]);

          Q_.resize(4);
          Q_[0] = atof(tokens[15]);
          Q_[1] = atof(tokens[16]);
          Q_[2] = atof(tokens[17]);
          Q_[3] = atof(tokens[18]);

          R_.resize(1);
          R_[0] = atof(tokens[20]);
          pclose(prg_output);

          spread_trader_->InitializeKalman(x_, P_, Q_, R_);
        }
      }

      // set up hooks for spread_exec
      for (unsigned int t_ctr_ = 4 * i; t_ctr_ < 4 * i + 4; t_ctr_++) {
        std::string shortcode = source_shortcode_vec_[t_ctr_];
        HFSAT::SecurityMarketView* p_smv_ = sid_to_smv_ptr_map_[sec_name_indexer_.GetIdFromString(shortcode)];
        p_smv_->subscribe_tradeprints(spread_trader_);
        p_smv_->subscribe_price_type(spread_trader_, HFSAT::kPriceTypeMktSizeWPrice);
        p_smv_->subscribe_rawtradeprints(spread_trader_);
      }

      int t_expiry_date_ = HFSAT::NSESecurityDefinitions::ComputeNextExpiry(start_date_);

      spread_trader_->SetHFTClasses(
          &watch_, sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_1_ + "_FUT0"),
          sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_1_ + "_FUT1"),
          sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_2_ + "_FUT0"),
          sec_name_indexer_.GetIdFromString("NSE_" + param_->instrument_2_ + "_FUT1"), t_expiry_date_);

      // set lotsize as per exchange file
      spread_trader_->UpdateLotSizes(
          HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_1_ + "_FUT0", start_date_),
          HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_1_ + "_FUT1", start_date_),
          HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_2_ + "_FUT0", start_date_),
          HFSAT::SecurityDefinitions::GetContractMinOrderSize("NSE_" + param_->instrument_2_ + "_FUT1", start_date_));

      spread_trader_->SetUnitTradeSize(uts1, uts2);
      spread_trader_->LoadStatMaps(start_date_, end_date_);
      spread_trader_->LoadEarningsMaps();
      spread_trader_->SetCurrentDayEarningsGetflat(start_date_);

      if (override_on_pos) {
        std::cout << "Setting overnight pos to " << on_pos1 << ":" << on_pos2 << '\n';
        spread_trader_->SetOvernightPosition(on_pos1, on_pos2);
      }

      spread_trader_->UpdateFlat(is_flat);
    }

    for (unsigned int i = 0; i < spread_traders.size(); i++) {
      MT_SPRD::SpreadTrader* spread_trader_ = spread_traders[i];
      MT_SPRD::SpreadExecLogic* sprd_exec_front_month_ = spread_trader_->GetFrontMonthExec();
      int p1, p2;
      sprd_exec_front_month_->GetOMPositions(p1, p2);
      std::cout << "After setting position: " << p1 << ":" << p2 << '\n';
    }

    // run historical dispatcher
    historical_dispatcher_.RunHist();

    for (unsigned int i = 0; i < spread_traders.size(); i++) {
      MT_SPRD::SpreadTrader* spread_trader_ = spread_traders[i];

      // call AllConsumed if needed
      if (is_notify_last_event) {
        spread_trader_->OnAllEventsConsumed();
      }

      if (save_state_) {
        MT_SPRD::SpreadExecLogic* sprd_exec_front_month_ = spread_trader_->GetFrontMonthExec();
        if (sprd_exec_front_month_) spread_trader_->GetSpreadExecVars();

        MT_SPRD::ParamSet param_ = spread_trader_->GetParams();
        if (param_.spread_comp_mode_ == 2) spread_trader_->SaveKalmanState();

        save_state(*spread_trader_, end_date_, live);
      }
    }
  }
}
