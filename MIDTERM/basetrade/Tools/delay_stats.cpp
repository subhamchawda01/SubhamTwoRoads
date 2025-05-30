/**
   \file Tools/sim_signals.hpp
   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
     Suite No 353, Evoma, #14, Bhattarhalli,
     Old Madras Road, Near Garden City College,
     KR Puram, Bangalore 560049, India
     +91 80 4190 3551
*/


#include "baseinfra/Tools/common_smv_source.hpp"
#include "basetrade/InitLogic/sim_strategy_helper.hpp"
// to get mfm
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CDef/ttime.hpp"

int main(int argc, char**argv) {
  if (argc != 6) {
    std::cerr << "usage: shortcode tradingdate starttime endtime market_model\n";
    exit(0);
  }

  std::vector<std::string> shortcodes_vec_;
  shortcodes_vec_.push_back(argv[1]);
  int tradingdate_ = atoi(argv[2]);
  char* start_tz_hhmm_ = argv[3];
  char* end_tz_hhmm_ = argv[4];  
  int market_model_index_ = atoi(argv[5]);

  HFSAT::ttime_t  exec_start_ttime_ = HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(tradingdate_, HFSAT::DateTime::GetHHMMSSTime(start_tz_hhmm_ + 4), start_tz_hhmm_), 0);
  HFSAT::ttime_t  exec_end_ttime_ = HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(tradingdate_, HFSAT::DateTime::GetHHMMSSTime(end_tz_hhmm_ + 4), end_tz_hhmm_), 0);


  CommonSMVSource* mkt_data_api_ = new CommonSMVSource(shortcodes_vec_, tradingdate_, false);
  mkt_data_api_->SetDepShortcodeVector(shortcodes_vec_);
  // get watch and logger
  HFSAT::DebugLogger& dbglogger_ = mkt_data_api_->getLogger();
  HFSAT::Watch&  watch_ = mkt_data_api_->getWatch();

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/tradelogs/sim_delay_stats." << shortcodes_vec_[0] << "." << tradingdate_;
  std::string log_filename_ = t_temp_oss_.str();
  mkt_data_api_->SetDbgloggerFileName(log_filename_);

  dbglogger_.AddLogLevel(PLSMM_INFO);
  dbglogger_.AddLogLevel(PLSMM_ERROR);
  dbglogger_.AddLogLevel(OLSMM_INFO);
  // dbglogger_.AddLogLevel(ORS_DATA_INFO);


  mkt_data_api_->SetSourceShortcodes(shortcodes_vec_); // prepare smvs for strategies
  mkt_data_api_->SetDepShortcodeVector(shortcodes_vec_); // it makes sid_is_dependant_map and sim_time_series_info
  mkt_data_api_->SetSimSmvRequired(true); // some exhanges makes different smv for sim_trader and strategy not sure why 
  // Initialize the smv source after setting the required variables
  mkt_data_api_->Initialize();
  std::vector<HFSAT::MarketOrdersView*> sid_to_mov_ptr_map_ = mkt_data_api_->getMOVMap();
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  sid_to_mov_ptr_map_.resize(sec_name_indexer_.NumSecurityId(), nullptr);

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = mkt_data_api_->getSMVMap();
  HFSAT::SecurityMarketViewPtrVec& sid_to_sim_smv_ptr_map_ = mkt_data_api_->getSimSMVMap();
  HFSAT::SimTimeSeriesInfo& sim_time_series_info_ = mkt_data_api_->getSimTimeSeriesInfo();
  HFSAT::HistoricalDispatcher& historical_dispatcher_ = mkt_data_api_->getHistoricalDispatcher();

  for (unsigned int i = 0; i < sec_name_indexer_.NumSecurityId(); i++) {
    HFSAT::SecurityMarketView* p_smv_ = sid_to_smv_ptr_map_[i];
    HFSAT::SecurityMarketView* p_sim_smv_ = sid_to_sim_smv_ptr_map_[i];
    // based on exchange/date we get price_smm / order_smm
    HFSAT::BaseSimMarketMaker* p_base_sim_market_maker_ = HFSAT::SimMarketMakerHelper::GetSimMarketMaker(dbglogger_, watch_, p_smv_, p_sim_smv_, market_model_index_, sim_time_series_info_, sid_to_mov_ptr_map_, historical_dispatcher_, mkt_data_api_, true);
    p_base_sim_market_maker_->PrintDelayStats(exec_start_ttime_, exec_end_ttime_);
    // account info is nullptr
    //DATE SHORTCODE LBOUND UBOUND SIZE MIN MEDIAN MEAN 75P 95P 99P MAX
  }
}
