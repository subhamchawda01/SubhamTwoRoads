/**
   \file SimMarketMakerCode/price_level_sim_market_maker.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/SimMarketMaker/bardata_sim_market_maker.hpp"
#include "baseinfra/OrderRouting/market_model_manager.hpp"
#include "baseinfra/SimMarketMaker/trade_ratio_calculator.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"

#define POSTPONE_MSECS 100
// #define USING_AGGRESSIVE_FRONT_CANCELATION

#define USING_ONLY_FULL_MKT_FOR_SIM

namespace HFSAT {

BardataSimMarketMaker* BardataSimMarketMaker::GetUniqueInstance(DebugLogger& dbglogger, Watch& watch,
                                                                      SecurityMarketView& dep_market_view,
                                                                      int market_model_index,
                                                                      SimTimeSeriesInfo& sim_time_series_info, bool _are_we_using_bardata_) {
  MarketModel this_market_model;

  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), market_model_index, this_market_model,
                                     watch.YYYYMMDD());

  std::ostringstream temp_oss;
  temp_oss << "BardataSimMarketMaker " << dep_market_view.shortcode() << ' ' << market_model_index;
  std::string bdsmm_description(temp_oss.str());

  static std::map<std::string, BardataSimMarketMaker*> SMM_description_map;
  if (SMM_description_map.find(bdsmm_description) == SMM_description_map.end()) {
    SMM_description_map[bdsmm_description] =
        new BardataSimMarketMaker(dbglogger, watch, this_market_model, dep_market_view, sim_time_series_info, _are_we_using_bardata_);
  }
  return SMM_description_map[bdsmm_description];
}

BardataSimMarketMaker* BardataSimMarketMaker::GetInstance(DebugLogger& dbglogger, Watch& watch,
                                                                SecurityMarketView& dep_market_view,
                                                                int market_model_index,
                                                                SimTimeSeriesInfo& sim_time_series_info, bool _are_we_using_bardata_) {
  MarketModel this_market_model;
  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), market_model_index, this_market_model,
                                     watch.YYYYMMDD());

  std::ostringstream temp_oss;
  temp_oss << "BardataSimMarketMaker " << dep_market_view.shortcode() << ' ' << market_model_index;
  std::string bdsmm_description(temp_oss.str());
  BardataSimMarketMaker* bdsmm_ =
      new BardataSimMarketMaker(dbglogger, watch, this_market_model, dep_market_view, sim_time_series_info, _are_we_using_bardata_);
  return bdsmm_;
}

BardataSimMarketMaker::BardataSimMarketMaker(DebugLogger& dbglogger, Watch& watch, MarketModel market_model,
                                                   SecurityMarketView& dep_market_view,
                                                   HFSAT::SimTimeSeriesInfo& sim_time_series_info, bool _are_we_using_bardata_)
    : BaseSimMarketMaker(dbglogger, watch, dep_market_view, market_model, sim_time_series_info, _are_we_using_bardata_),
      dep_market_view_(dep_market_view),
      bardata_buffer_() {
  dep_market_view_.ComputeMidPrice();
  dep_market_view_.ComputeMktPrice();

  ///< to get TimePeriod called every msec
  watch.subscribe_OneMinutePeriod(this);
}

BardataSimMarketMaker::~BardataSimMarketMaker() {
}

void BardataSimMarketMaker::SubscribeBardataEvents(SecurityMarketView& dep_market_view) {
  dep_market_view.subscribe_bardata(this);
}

void BardataSimMarketMaker::OnTimePeriodUpdate(const int num_pages_to_add) {

/*
  if (!all_requests_.empty()) {
    std::cout << "ProcessBardataRequestQueue" << std::endl;
    ProcessBardataRequestQueue();
  }
*/
}

void BardataSimMarketMaker::OnBardataUpdate(const unsigned int _security_id_,
                                              const char* _bardata_line_) {

  //std::cout << "BardataSimMarketMaker::OnBardataUpdate: " << _security_id_ << " mkt: " << process_mkt_updates_ << " all_requests_: " << all_requests_.size() 
  //          << "\n" << _bardata_line_ << std::endl;
  if (!process_mkt_updates_) return;

  memset((void*)&bardata_buffer_,0,sizeof(bardata_buffer_)); 
  strcpy(bardata_buffer_,_bardata_line_);
  HFSAT::PerishableStringTokenizer pst(bardata_buffer_,sizeof(bardata_buffer_));
  //std::vector<char const*> const& tokens = pst.GetTokens();

  //std::cout << "Bardata_line: " << bardata_buffer_ << "\n" << std::atof(tokens[6]) << " " << std::atoi(tokens[9]) << std::endl;
  //bardata_price_size_[std::atof(tokens[6])] = std::atoi(tokens[9]);

  if (!all_requests_.empty()) {
    ProcessBardataRequestQueue();
  }
}

}
