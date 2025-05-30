/*

We are trying to get one model for multiple contracts

multiple dependants data
nse_options
nse_futures


first input:
consider SBIN,

second input: OTMInATM_CS 4 1
SBIN_P0_O3
SBIN_P0_O2
SBIN_P0_O1
SBIN_P0_A
SBIN_C0_A
SBIN_P0_O1
SBIN_P0_O2
SBIN_P0_O3

first column will be unix_time msecs_from_midnight or empty
at each time t, we have the ImpliedVol for all these contracts

# TIMESERIESDATA
# Events doesnt make much sense for the data we are dealing with
# Trades and Time could be better triggers
# 30000/1000 0 1
# Should we care about 20 days / 180 days
# How frequently do we want to recompute

# REGDATA
# Difference between Current and Future is better reprenseted by Time / Trades
# However since we are dealing with multiple dependants we go by Time for now
# Duration in seconds, further we can smooth the changes
# Removing AllZero Lines, Remove AnyZero Lines
# Each Row: needs to be weighted is different on different time to expiries ( ? )
# Each Column: needs to be weighted by vega


# LM
# Relative Indicators:
# ImpliedVolTrend ( with negative sign )
# ImpliedVolSpread ( relative with k-1 contract )
# SectorRelative
# MarketRelative


# PCA USING SAMPLEDATA/REGDATA
0) DataMatrix D{NxD }
1) Compute Mean Vector M{D}
2) Compute Scatter Matrix V{DxD}
3) Compute Eigen Vectors and Eigen Values E{D}
4) Sort based on Eigen Values and Choose K Eigen Vectors
5) Using W{DxK}, T(W) * D = Y

# TIMESERIES MODELS
1) ARMA
2) GARCH

*/

// TimeSeriesData
// RegData
// LM
// PCA
// TimeSeries
// should be / can be used for nse options/ futures

// TimeSeriesData
// SampleInfo
// Date
// Start_and_EndTime
// Skip_Time_Period

// Underlying BasePrice FutPrice ShortcodesString
// vector[II] - indicator specification {instance per shortcode}
// vector[GI] - indicator specification {same instance}

// UNDERLYING
// NSE_SBIN_FUT0 ImpliedVol OTMInATM 4 1
// IINDICATORS
// INDICATOR 1 ImpliedVolSpread OPT 30 ImpliedVol
// INDICATOR 1 ImpliedVolTrend OPT 30 ImpliedVol
// GINDICATORS
// INDICATOR 1 ImpliedVolTrend NSE_BANKNIFTY 30 ImpliedVol
// INDICATOR 1 ImpliedVolTrend NSE_NIFTY 30 ImpliedVol
// MODELEND

// or

// UNDERLYING
// NSE_SBIN_FUT0 Midprice FUT
// NSE_ICICIBANK_FUT0 Midprice FUT
// NSE_AXISBANK_FUT0 Midprice FUT
// NSE_PNB_FUT0 Midprice FUT
// IINDICATORS
// INDICATOR 1 SimpleTrend FUT 30 Midprice
// GINDICATORS
// INDICATOR 1 SimpleTrend NSE_BANKNIFTY 30 Midprice
// MODELEND

#include "basetrade/InitLogic/md_simulator.hpp"

// across functions ( instead of passing references)
// HFSAT::BulkFileWriter bulk_file_writer_(32 * 1024 * 1024);  // 32MB .. increasing this makes the program very slow //

// HFSAT::BulkFileWriter bulk_file_writer_(
int start_date_;
int end_date_;
int start_utc_hhmm_;
int end_utc_hhmm_;
unsigned int progid_;
std::string instruction_file_;
unsigned int msecs_to_wait_to_print_again_;
unsigned int num_trades_to_wait_print_again_;
char* process_algo_;
unsigned int pred_duration_;
std::vector<std::string> dep_shortcode_vec_;
std::vector<std::string> source_shortcode_vec_;
std::string logfilename_;
std::string output_filename_ = "";
bool print_ref_;
int make_models_;

unsigned int number_of_dependants_;
unsigned int number_of_iindicators_;
unsigned int number_of_gindicators_;

HFSAT::InMemData* in_memory_data_;

void ProcessOneDay(int t_ddate_) {
  // Make an object of CommonSMVSource and use it as an API
  std::cerr << "Processing " << t_ddate_ << "\n";
  std::vector<std::string> dummy_shc_list;
  CommonSMVSource* common_smv_source = new CommonSMVSource(dummy_shc_list, t_ddate_);
  // std::unique_ptr<CommonSMVSource> common_smv_source(new CommonSMVSource(dummy_shc_list, t_ddate_));

  HFSAT::Watch& watch_ = common_smv_source->getWatch();
  HFSAT::DebugLogger& dbglogger_ = common_smv_source->getLogger();

  // Create all the unique instances of required managers
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(t_ddate_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(t_ddate_).LoadNSESecurityDefinitions();
  HFSAT::MDModelCreator::CollectMDModelShortCodes(dbglogger_, watch_, instruction_file_, dep_shortcode_vec_,
                                                  source_shortcode_vec_);

  // int start_mfm_ = (3600 * (start_utc_hhmm_ / 100) + 60 * (start_utc_hhmm_ % 100)) * 1000;
  // Set all the parameters in the common_smv_source
  common_smv_source->SetSourceShortcodes(source_shortcode_vec_);
  common_smv_source->SetDepShortcodeVector(dep_shortcode_vec_);
  common_smv_source->SetDbgloggerFileName(logfilename_);
  common_smv_source->SetStartEndTime(start_utc_hhmm_, end_utc_hhmm_);
  common_smv_source->SetStartEndUTCDate(t_ddate_, t_ddate_);
  common_smv_source->Initialize();

  // Get the security id to smv ptr from common source
  HFSAT::SecurityMarketViewPtrVec sid_to_smv_ptr_map = common_smv_source->getSMVMap();

  // Initialize the market update manager. This guy subscribes itself to all the security market views.
  // Indicators subscribe to this market update manager for data interrupted cases.

  // local variable destroyed auto
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::MarketUpdateManager& market_update_manager_ = *(HFSAT::MarketUpdateManager::GetUniqueInstance(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map, t_ddate_));

  //-----------------------------------------------------------------------------------------------------
  HFSAT::CommonIndicator::set_global_start_mfm(
      HFSAT::GetMsecsFromMidnightFromHHMM(HFSAT::GetMsecsFromMidnightFromHHMM(start_utc_hhmm_)));
  HFSAT::CommonIndicator::set_global_end_mfm(
      HFSAT::GetMsecsFromMidnightFromHHMM(HFSAT::GetMsecsFromMidnightFromHHMM(end_utc_hhmm_)));

  // 1) All Indicators are instantiated
  // 2) IndicatorsStats object is created and subscribed to listen all the indicators.
  // 3) Indicators subscribe to smv inside their own constructor/

  // create printer
  // create indicator instances
  // add indicators to printer
  // final step of the printer is to dump into file
  // all other intermediate processing isdone in memory

  // step1 : instruction_file, date, start_time, end_time, sampling_info
  // step2 : stats ( what stats ? )
  // step3 : pred_duration, processing_algo, filter
  // step4 : stats ( what stats ? )

  HFSAT::MDIndicatorLogger* nse_md_logger_ = nullptr;

  // time vector { 1 }
  // dep matrix  { n }
  // iindicator matrix { n * i }
  // gindicator matrix { d }

  nse_md_logger_ = HFSAT::MDModelCreator::CreateMDIndicatorLogger(dbglogger_, watch_,
                                                                  // bulk_file_writer_,
                                                                  instruction_file_, msecs_to_wait_to_print_again_,
                                                                  num_trades_to_wait_print_again_);

  //  number_of_dependants_ = (number_of_dependants_ > 0u ? number_of_dependants_ : nse_md_logger_->GetNumDeps());
  //  number_of_iindicators_ = (number_of_iindicators_ > 0u ? number_of_iindicators_ :  nse_md_logger_->GetNumIInds());
  //  number_of_gindicators_ = (number_of_gindicators_ > 0u ? number_of_gindicators_ :  nse_md_logger_->GetNumGInds());

  // subscribes all indicators to market data interrupts
  nse_md_logger_->SubscribeMarketInterrupts(market_update_manager_);
  nse_md_logger_->setStartTM(start_utc_hhmm_);  // this is checked in IndicatorLogger::PrintVals
  nse_md_logger_->setEndTM(t_ddate_, t_ddate_, end_utc_hhmm_);
  // Subscribe to all smvs
  market_update_manager_.start();
  HFSAT::MDModelCreator::LinkupLoggerToOnReadySources(nse_md_logger_, source_shortcode_vec_);

  // ----------------------------------------------------------------------------------------------------------------------

  // Seek the Historical Dispatcher.
  // All the required parameters like start_time, end_time etc. has been set earlier.
  common_smv_source->Seek();

  // Run the Historical Dispatcher
  // This guy starts the entire loop of:
  // LoggedMessageFileSource->(Indexed)MarketUpdateManagers->SMV(SecurityMarketView)->OnMarketUpdate/OnTrade->Indicators

  common_smv_source->Run();

  if (pred_duration_ > 0) {
    //    nse_md_logger_->MakeRegData(process_algo_, pred_duration_, in_memory_data_);
    // dump data in final stage ( one write and huge data
    // nse_md_logger_->DumpData(2, output_filename_, std::ios::out | std::ios::binary | std::ios_base::app, print_ref_);
  } else {
    nse_md_logger_->DumpData(1, output_filename_, std::ios::out | std::ios::binary | std::ios_base::app, print_ref_);
  }

  /*  for(auto i = 0u; i < sid_to_smv_ptr_map.size(); i++) {
    std::cerr << sid_to_smv_ptr_map[i]->shortcode() << "\n";
    sid_to_smv_ptr_map[i]->DumpStatus();
    if(sid_to_smv_ptr_map[i]->is_ready_complex(2)) {
      std::cerr << "complex ready\n";
    }
    }*/

  // ------------- CLEANUP-----------------

  // indicator_logger_
  if (nse_md_logger_ != nullptr) {
    delete nse_md_logger_;
  }

  // historical dispatcher & smv's
  common_smv_source->cleanup();
  //  delete common_smv_source;
  // exchange symbol manager
  HFSAT::ExchangeSymbolManager::RemoveUniqueInstance();

  // security definiation
  HFSAT::SecurityDefinitions::RemoveUniqueInstance();

  // market update manager
  HFSAT::MarketUpdateManager::RemoveUniqueInstance();

  // indirect code
  HFSAT::CurrencyConvertor::RemoveInstance();

  // rest are all local space will be destroyed hopefully !
  dep_shortcode_vec_.clear();
  source_shortcode_vec_.clear();
  // HFSAT::MDModelCreator::dep_market_view_vec_.clear();
  // HFSAT::MDModelCreator::baseprice_vec_.clear();
  // --------------------------------------

  // HFSAT::CommonIndicator::ClearIndicatorMap();
  //  delete common_smv_source;

  // other misc
  dbglogger_.Close();
  HFSAT::OptionObject::ClearOptionObjects();
}

// regression
void MakeModels() {
  // how many deps, iindicators, gindicator
  // for each contract
  // for each iindicator and for each gindicator get beta

  // dump beta matrix file and write the model file
  std::vector<std::vector<double> > beta_matrix_;
  std::vector<double> dep_stdevs_;
  std::vector<double> err_mean_;
  std::vector<double> err_stdev_;
  std::vector<double> model_corrs_;
  std::vector<double> model_stdevs_;

  /*std::cout << number_of_dependants_ << " "
            << number_of_iindicators_ << " "
            << number_of_gindicators_ << " "
            << in_memory_data_->NumLines() << " "
            << in_memory_data_->NumWords() << "\n";*/
  beta_matrix_.resize(number_of_dependants_);
  for (auto i = 0u; i < number_of_dependants_; i++) {
    std::vector<double> dependant_series_;
    in_memory_data_->GetColumn(i, dependant_series_);
    std::vector<double> yhat_(dependant_series_.size(), 0);
    for (unsigned int j = 0; j < number_of_iindicators_; j = j + number_of_dependants_) {
      std::vector<double> independant_series_;
      double beta_;

      in_memory_data_->GetColumn(number_of_dependants_ + j, independant_series_);
      beta_ = HFSAT::GetSLRCoeff(dependant_series_, independant_series_);

      beta_matrix_[i].push_back(beta_);
      for (auto n = 0u; n < independant_series_.size(); n++) {
        yhat_[n] += independant_series_[n] * beta_;
      }
    }
    for (unsigned int k = 0; k < number_of_gindicators_; k++) {
      std::vector<double> independant_series_;
      double beta_;

      in_memory_data_->GetColumn(number_of_dependants_ + number_of_iindicators_ + k, independant_series_);
      beta_ = HFSAT::GetSLRCoeff(dependant_series_, independant_series_);

      beta_matrix_[i].push_back(beta_);
      for (auto n = 0u; n < independant_series_.size(); n++) {
        yhat_[n] += independant_series_[n] * beta_;
      }
    }

    // dep_stdevs_.push_back(GetStdev(dependant_series_));
    // model_corrs_.push_back(GetCorrelation(dependant_series_, yhat_));

    std::vector<double> errors_;
    //   for(unsigned int n = 0; n < ; n++){
    //  }
  }
}

// main will parse command line and call functions
int main(int argc, char** argv) {
  // bool use_l1_data_ = false;

  // Parse all the command line parameters
  if (argc >= 9) {
    instruction_file_ = argv[1];
    start_date_ = atoi(argv[2]);
    end_date_ = atoi(argv[3]);

    char* start_hhmm_ = argv[4];
    char* end_hhmm_ = argv[5];
    char* tz_ = NULL;

    //    start_utc_hhmm_ = atoi(start_utc_hhmm_);
    //    end_utc_hhmm_ = atoi(end_hhmm_);

    if (strncmp(start_hhmm_, "IST_", 4) == 0) {
      tz_ = start_hhmm_;
      start_hhmm_ = start_hhmm_ + 4;
      start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(start_date_, atoi(start_hhmm_), tz_);
    } else {
      start_utc_hhmm_ = atoi(start_hhmm_);
    }

    if (strncmp(end_hhmm_, "IST_", 4) == 0) {
      tz_ = end_hhmm_;
      end_hhmm_ = end_hhmm_ + 4;
      end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(start_date_, atoi(end_hhmm_), tz_);
    } else {
      end_utc_hhmm_ = atoi(end_hhmm_);
    }

    progid_ = atoi(argv[6]);

    if (strncmp(argv[7], "OF@", 3) == 0) {
      strtok(argv[7], "@");
      output_filename_ = strtok(NULL, ",");
      print_ref_ = (atoi(strtok(NULL, ",")) > 0 ? true : false);
    }

    if (strncmp(argv[8], "TD@", 3) == 0) {
      strtok(argv[8], "@");
      msecs_to_wait_to_print_again_ = atoi(strtok(NULL, ","));
      num_trades_to_wait_print_again_ = atoi(strtok(NULL, ","));
    }

    if (argc == 10 && strncmp(argv[9], "RD@", 3) == 0) {
      strtok(argv[9], "@");
      process_algo_ = strtok(NULL, ",");
      pred_duration_ = atoi(strtok(NULL, ","));
    }

    if (argc == 11 && strncmp(argv[10], "MM@", 3) == 0) {
      strtok(argv[10], "@");
      //     make_models_ = strtok(NULL, ",");
    }

    /*std::cerr << argc << " " << instruction_file_ << " "
              << start_date_ << " "
              << end_date_ << " "
              << start_utc_hhmm_ << " "
              << end_utc_hhmm_ << " "
              << progid_ << " "
              << output_filename_ << " "
              << msecs_to_wait_to_print_again_ << " "
              << num_trades_to_wait_print_again_ << " "
              << process_algo_ << " "
              << pred_duration_ << "\n";*/
  } else {
    std::cerr << "not enought arguments, try including \n";
    std::cerr << "instruction_file start_date end_date start_time end_time prog_id OF@outputfile,print_ref "
                 "TD@msecs,numtrades RD@palgo,duration MM@1 \n";
    exit(-1);
  }

  // ddate_
  // instruction_file_,logfilename_, start_utc_hhmm_, end_utc_hhmm_, bulkfilewriter, msecs_to_wait_to_print_again_,
  // num_trades, process_algo_, pred_duration_
  int ddate_;
  in_memory_data_ = new HFSAT::InMemData();
  number_of_dependants_ = 0u;
  number_of_iindicators_ = 0u;
  number_of_gindicators_ = 0u;
  // just clearing the exisiting file
  std::ofstream local_ofstream_;
  local_ofstream_.open(output_filename_, std::ofstream::out | std::ofstream::trunc);
  local_ofstream_.close();

  for (ddate_ = end_date_; ddate_ >= start_date_;) {
    while (HFSAT::HolidayManagerNoThrow::IsExchangeHoliday(HFSAT::EXCHANGE_KEYS::kExchSourceNSEStr, ddate_, true)) {
      ddate_ = HFSAT::DateTime::CalcPrevWeekDay(ddate_);
    }
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/datalogs/nsedatalog." << ddate_ << "." << progid_;
    logfilename_ = t_temp_oss_.str();
    ProcessOneDay(ddate_);
    ddate_ = HFSAT::DateTime::CalcPrevWeekDay(ddate_);
  }

  // regression
  // FSRR / LM
  // forward stagewise ?
  // MakeFSRRModels();
  // MakeLMModels();
  // simple beta sacrilege:
  if (make_models_ == 1) {
    MakeModels();
  }
}
