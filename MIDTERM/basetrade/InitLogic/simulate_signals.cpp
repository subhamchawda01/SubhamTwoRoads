/**
    \file InitLogic/simulate_signals.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

// mkt_data_api and sim_market_maker
#include "baseinfra/Tools/common_smv_source.hpp"

// getsmm function is defined here
#include "basetrade/InitLogic/sim_strategy_helper.hpp"

// base trader
#include "baseinfra/BaseTrader/sim_trader_helper.hpp"
// som
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"

// [order <-> exec module]
#include "dvctrade/ExecLogic/sbe_risk_reader_from_file.hpp"
#include "dvctrade/ExecLogic/signal_based_trading.hpp"

// pnl module
#include "baseinfra/SimPnls/sim_base_pnl.hpp"

// strategy desc
#include "dvctrade/ExecLogic/sbe_interface.hpp"

// to get mfm
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/ttime.hpp"
/*
ORDERS_FILE
format:


EXEC_PARAMS_FILE
format:
FILE1
     format:
     PARAMVALUE AVG_SPREAD 0.5
FILE2
FILE3



STRAT_ID

TRADING_DATE

START_TIME

END_TIME

*/

int main(int argc, char** argv) {
  if (argc != 7) {
    std::cerr << "usage: risk_file params_file simulation_id trading_date start_time end_time\n";
    exit(0);
  }

  // collect input paramters
  std::string risk_filename_(argv[1]);
  std::string params_filename_(argv[2]);
  unsigned int simulator_id_ = atoi(argv[3]);
  int tradingdate_ = atoi(argv[4]);
  char* start_tz_hhmm_ = argv[5];
  char* end_tz_hhmm_ = argv[6];

  bool livetrading_ = false;

  int exec_start_utc_mfm_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
      tradingdate_, HFSAT::DateTime::GetHHMMSSTime(start_tz_hhmm_ + 4), start_tz_hhmm_));
  HFSAT::ttime_t exec_start_ttime_ =
      HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(
                         tradingdate_, HFSAT::DateTime::GetHHMMSSTime(start_tz_hhmm_ + 4), start_tz_hhmm_),
                     0);
  int exec_end_utc_mfm_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
      tradingdate_, HFSAT::DateTime::GetHHMMSSTime(end_tz_hhmm_ + 4), end_tz_hhmm_));
  HFSAT::ttime_t exec_end_ttime_ =
      HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(
                         tradingdate_, HFSAT::DateTime::GetHHMMSSTime(end_tz_hhmm_ + 4), end_tz_hhmm_),
                     0);

  // we have (instrument, param_set), which are used to create smvs, so we need to read
  // param file first ( paramset has instrument as datatype )
  // sec_name_indexer risk is controlled by risk of params
  std::vector<HFSAT::SBEParamSet*> paramset_vec_;
  std::vector<std::string> shortcodes_vec_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::SBEInterface::CollectParamSetVec(paramset_vec_, params_filename_);
  for (auto t_paramset_ : paramset_vec_) {
    shortcodes_vec_.push_back(t_paramset_->instrument_);
  }

  // we set everything using common_smv_tool
  // create smvs
  CommonSMVSource* mkt_data_api_ = new CommonSMVSource(shortcodes_vec_, tradingdate_, livetrading_);
  mkt_data_api_->SetDepShortcodeVector(shortcodes_vec_);
  // get watch and logger
  HFSAT::DebugLogger& dbglogger_ = mkt_data_api_->getLogger();
  HFSAT::Watch& watch_ = mkt_data_api_->getWatch();

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/tradelogs/simulator_log." << tradingdate_ << "." << simulator_id_;
  std::string log_filename_ = t_temp_oss_.str();
  mkt_data_api_->SetDbgloggerFileName(log_filename_);

  HFSAT::BulkFileWriter trades_writer_(256 * 1024);
  t_temp_oss_.str("");
  t_temp_oss_.clear();
  t_temp_oss_ << "/spare/local/logs/tradelogs/simulator_trades." << tradingdate_ << "." << simulator_id_;
  std::string trades_filename_ = t_temp_oss_.str();
  trades_writer_.Open(trades_filename_.c_str(), (livetrading_ ? std::ios::app : std::ios::out));

  // set mkt_data_api
  // logger is created
  // exchange symbol mapping is created if required NSE as well
  // trading location is inferred from first shortcode
  // smvs are set
  // proms are created
  mkt_data_api_->SetSourceShortcodes(shortcodes_vec_);    // prepare smvs for strategies
  mkt_data_api_->SetDepShortcodeVector(shortcodes_vec_);  // it makes sid_is_dependant_map and sim_time_series_info
  mkt_data_api_->SetSimSmvRequired(true);  // some exhanges makes different smv for sim_trader and strategy not sure why
  // Initialize the smv source after setting the required variables
  mkt_data_api_->Initialize();

  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_ = mkt_data_api_->getMOVMap();
  sid_to_mov_ptr_map_.resize(sec_name_indexer_.NumSecurityId(), nullptr);

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = mkt_data_api_->getSMVMap();
  HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_ = mkt_data_api_->getSimSMVMap();
  std::map<std::string, HFSAT::ExternalDataListener*> shortcode_filesource_map_ =
      mkt_data_api_->getShortcodeFilesourceMap();
  HFSAT::SimTimeSeriesInfo& sim_time_series_info_ = mkt_data_api_->getSimTimeSeriesInfo();
  HFSAT::HistoricalDispatcher& historical_dispatcher_ = mkt_data_api_->getHistoricalDispatcher();

  // set sim / ors_api ( som / basetrade ())
  // to create som, we need base_trader
  // for base_trader we need sim_market_maker
  std::vector<HFSAT::SmartOrderManager*> t_order_manager_vec_;
  HFSAT::MultBasePNL* p_mult_base_pnl_ = new HFSAT::MultBasePNL(dbglogger_, watch_);

  for (unsigned int i = 0; i < sec_name_indexer_.NumSecurityId(); i++) {
    HFSAT::SecurityMarketView* p_smv_ = sid_to_smv_ptr_map_[i];
    HFSAT::SecurityMarketView* p_sim_smv_ = sid_to_sim_smv_ptr_map_[i];
    int t_sim_market_model_index_ = 0;
    // based on exchange/date we get price_smm / order_smm
    HFSAT::BaseSimMarketMaker* p_base_sim_market_maker_ = HFSAT::SimMarketMakerHelper::GetSimMarketMaker(
        dbglogger_, watch_, p_smv_, p_sim_smv_, t_sim_market_model_index_, sim_time_series_info_, sid_to_mov_ptr_map_,
        historical_dispatcher_, mkt_data_api_, true);
    // account info is nullptr
    HFSAT::BaseTrader* p_base_trader_ = HFSAT::SimTraderHelper::GetSimTrader("nullptr", p_base_sim_market_maker_);
    HFSAT::SmartOrderManager* p_smart_order_manager_ = new HFSAT::SmartOrderManager(
        dbglogger_, watch_, sec_name_indexer_, *(p_base_trader_), *(p_smv_), simulator_id_, livetrading_, 1);

    // further smm would fwd bunch of things to som
    p_base_sim_market_maker_->AddOrderNotFoundListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderSequencedListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderConfirmedListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderConfCxlReplaceRejectedListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderConfCxlReplacedListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderCanceledListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderExecutedListener(p_smart_order_manager_);
    p_base_sim_market_maker_->AddOrderRejectedListener(p_smart_order_manager_);

    // we want to link smm to datalistenerlistener because we want to make sure, when we process ontimeperiodupdate,
    // only correponding dep functions are called, watch should be independent of list of sources
    auto t_filesource_ = shortcode_filesource_map_[p_smv_->shortcode()];
    t_filesource_->AddExternalDataListenerListener(p_base_sim_market_maker_);
    p_base_sim_market_maker_->AddSecIdToSACI(p_smart_order_manager_->server_assigned_client_id_, i);
    // we need to link some pnl module to som so it pnl_module reiceves exec_info from som
    // we use simbasepnl per dep and then we add  multbasepnl as its listener
    HFSAT::SimBasePNL* p_sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_, *(p_smv_), i, trades_writer_);
    p_smart_order_manager_->SetBasePNL(p_sim_base_pnl);
    int t_pnl_index_ = p_mult_base_pnl_->AddSecurity(p_sim_base_pnl) - 1;
    p_sim_base_pnl->AddListener(t_pnl_index_, p_mult_base_pnl_);
    t_order_manager_vec_.push_back(p_smart_order_manager_);
  }

  // given we read param here, we could choose between set of exec logics but for now there is only
  // also we are choosing to trade multiple products using same exec and not one exec for each product
  HFSAT::SignalBasedTrading* t_sbe_trader_ =
      new HFSAT::SignalBasedTrading(dbglogger_, watch_, sid_to_smv_ptr_map_, t_order_manager_vec_, paramset_vec_,
                                    livetrading_, p_mult_base_pnl_, exec_start_utc_mfm_, exec_end_utc_mfm_);
  HFSAT::SBERiskReaderFromFile* risk_reader_ = new HFSAT::SBERiskReaderFromFile(watch_, dbglogger_, risk_filename_);
  watch_.subscribe_BigTimePeriod(risk_reader_);
  risk_reader_->SubscribeRiskChanges(t_sbe_trader_);

  mkt_data_api_->Seek(exec_start_ttime_);
  mkt_data_api_->Run(exec_end_ttime_);
  t_sbe_trader_->ReportResults(trades_writer_);
  trades_writer_.Close();
  return 0;
}
