#include <iostream>
#include <sstream>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"

#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/portfolio_price.hpp"

#include "dvctrade/Indicators/pca_deviation_pairs_port.hpp"

// #include "basetrade/InitLogic/base_data_gen.hpp"

#define IF_TESTING_LOAD_PCA_VALUES_ONLY
//#define COMPUTING_PCA_PORT_PRICE

int main(int argc, char** argv) {
#ifdef IF_TESTING_LOAD_PCA_VALUES_ONLY
  HFSAT::PcaWeightsManager pca_wt_manager = HFSAT::PcaWeightsManager::GetUniqueInstance();
  pca_wt_manager.PrintPCAInfo();
#endif

#ifdef COMPUTING_PCA_PORT_PRICE

  if (argc <= 3) {
    std::cerr << " USAGE: EXEC  FILE_WITH_INDICATOR DATE START_TIME" << std::endl;
    exit(0);
  }

  std::string portfolio_shortcode = "";
  int yyyymmdd = atoi(argv[2]);

  int datagen_start_utc_hhmm_ = 0;
  if ((strncmp(argv[3], "EST_", 4) == 0) || (strncmp(argv[3], "CST_", 4) == 0) || (strncmp(argv[3], "CET_", 4) == 0) ||
      (strncmp(argv[3], "BRT_", 4) == 0) || (strncmp(argv[3], "UTC_", 4) == 0)) {
    datagen_start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(yyyymmdd, atoi(argv[3] + 4), argv[3]);
  } else {
    datagen_start_utc_hhmm_ = atoi(argv[3]);
  }
  std::cerr << "Time consideration: " << datagen_start_utc_hhmm_ << std::endl;

  char buffer[10240];
  // = "INDICATOR 0.0598324 OfflineCorradjustedPairsCombo ZF_0 UBFUT 30.0 1 MktSizeWPrice";

  std::vector<const char*> tokens_for_processing;
  // Read from a file
  std::ifstream infile;
  infile.open(argv[1]);
  if (infile.is_open()) {
    while (infile.good()) {
      bzero(buffer, 10240);
      infile.getline(buffer, 10240);
      HFSAT::PerishableStringTokenizer st_(buffer, 10240);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() < 8) continue;
      if (strcmp(tokens_[0], "INDICATOR") == 0) {
        portfolio_shortcode = std::string(tokens_[4]);
        tokens_for_processing.clear();
        for (unsigned int ii = 0; ii < tokens_.size(); ii++) {
          tokens_for_processing.push_back(tokens_[ii]);
        }
        break;
      }
      // process only the first portfolio
      // ignore rest
    }
  }

  if (strcmp(portfolio_shortcode.c_str(), "") == 0) {
    std::cerr << "Invalid portfolio to process" << portfolio_shortcode << "exiting..." << std::endl;
    exit(0);
  }

  std::cout << "Portfolio " << portfolio_shortcode << " Dep : " << tokens_for_processing[3] << std::endl;

  // Setup dbglogger, etc
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/test_pca_port_price." << portfolio_shortcode << "_"
                << tokens_for_processing[3] << "." << yyyymmdd;
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }
  HFSAT::Watch watch_(dbglogger_, yyyymmdd);
  HFSAT::PriceType_t my_price_type = HFSAT::kPriceTypeMktSizeWPrice;

  int tradingdate_ = yyyymmdd;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  std::vector<std::string> source_shortcode_vec_;
  HFSAT::TradingLocation_t dep_trading_location_ = HFSAT::kTLocCHI;

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  std::vector<bool> sid_to_marketdata_needed_map_;  ///< security_id to bool map indicating whether market data is
  /// needed or not for this source [ if true then sources will be
  /// added ]

  HFSAT::PCAPortPrice::CollectShortCodes(source_shortcode_vec_, portfolio_shortcode);

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

  // Please note that it is not the case that for every sid there will be an SecurityMarketView *, since some could just
  // be for ORS based info ( managed by PromOrderManager )
  std::vector<HFSAT::ExchSource_t> sid_to_exch_source_map_;
  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
    sid_to_exch_source_map_.push_back(HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_));
  }

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ =
      HFSAT::sid_to_security_market_view_map();  ///< Unique Instance of map from sid to p_smv_
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();  ///< Unique Instance of map from shortcode to p_smv_

  // map security_id to a newly created SecurityMarketView *
  // map shortcode_ to the SecurityMarketView *
  // map security_id to ExchSource_t
  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
    const char* _this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);

    if (sid_to_marketdata_needed_map_[i]) {  // only for the securites we are processing market data
      HFSAT::SecurityMarketView* p_smv_ =
          new HFSAT::SecurityMarketView(dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_,
                                        i, sid_to_exch_source_map_[i]);
      sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
      shortcode_smv_map_.AddEntry(_this_shortcode_, p_smv_);  // add to shortcode_ to SMV* map
    } else {
      sid_to_smv_ptr_map_.push_back(NULL);  // sid points to NULL since we do not want to process market data for this
    }
  }

  // initialize the marketdata processing market_view_managers
  // during initialization , this links itself to PromOrderManager if necessary
  HFSAT::EUREXPriceLevelMarketViewManager eurex_price_level_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                                 sid_to_smv_ptr_map_);

  HFSAT::HistoricalDispatcher historical_dispatcher_;

  std::map<std::string, HFSAT::CMELoggedMessageFileSource*> shortcode_cme_data_filesource_map_;
  std::map<std::string, HFSAT::EUREXLoggedMessageFileSource*> shortcode_eurex_data_filesource_map_;

  // initialize Adapters ( with right input argument ) for each market data source
  for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);

    switch (sid_to_exch_source_map_[i]) {
      case HFSAT::kExchSourceCME: {
        if (sid_to_marketdata_needed_map_[i]) {
          if (shortcode_cme_data_filesource_map_.find(shortcode_) == shortcode_cme_data_filesource_map_.end()) {
            shortcode_cme_data_filesource_map_[shortcode_] = new HFSAT::CMELoggedMessageFileSource(
                dbglogger_, sec_name_indexer_, tradingdate_, i, sec_name_indexer_.GetSecurityNameFromId(i),
                dep_trading_location_);
            shortcode_cme_data_filesource_map_[shortcode_]->SetPriceLevelGlobalListener(
                &eurex_price_level_market_view_manager_);  // this makes EUREXPriceLevelMarketViewManager process CME
                                                           // data
            shortcode_cme_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

            historical_dispatcher_.AddExternalDataListener(shortcode_cme_data_filesource_map_[shortcode_]);
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
                &eurex_price_level_market_view_manager_);
            shortcode_eurex_data_filesource_map_[shortcode_]->SetExternalTimeListener(&watch_);

            historical_dispatcher_.AddExternalDataListener(shortcode_eurex_data_filesource_map_[shortcode_]);
          }
        }

      } break;
      case HFSAT::kExchSourceMEFF:
      case HFSAT::kExchSourceIDEM:
      case HFSAT::kExchSourceHONGKONG:
      case HFSAT::kExchSourceREUTERS:
      case HFSAT::kExchSourceICE:
      case HFSAT::kExchSourceEBS:
      default: { } break; }
  }

#define MINUTES_TO_PREP 30
  // To only process data starting MINUTES_TO_PREP minutes before datagen_start_utc_hhmm_
  int event_process_start_utc_hhmm_ = datagen_start_utc_hhmm_;
  {
    event_process_start_utc_hhmm_ =
        ((event_process_start_utc_hhmm_ / 100) * 60) + (event_process_start_utc_hhmm_ % 100);
    event_process_start_utc_hhmm_ = std::max(0, event_process_start_utc_hhmm_ - MINUTES_TO_PREP);
    event_process_start_utc_hhmm_ = (event_process_start_utc_hhmm_ % 60) + ((event_process_start_utc_hhmm_ / 60) * 100);
  }
#undef MINUTES_TO_PREP
  // In all the file sources, read events till
  // we reach the first event after the specified ttime_t
  historical_dispatcher_.SeekHistFileSourcesTo(
      HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(tradingdate_, event_process_start_utc_hhmm_), 0));

  // start event loop
  try {
    historical_dispatcher_.RunHist();
  } catch (int e) {
  }

  // all sources added to historical_dispatcher_
  historical_dispatcher_.DeleteSources();
  // SecurityMarketView * objects
  for (auto i = 0u; i < sid_to_smv_ptr_map_.size(); i++) {
    if (sid_to_smv_ptr_map_[i] != NULL) {
      delete (sid_to_smv_ptr_map_[i]);
    }
  }

#endif
}
