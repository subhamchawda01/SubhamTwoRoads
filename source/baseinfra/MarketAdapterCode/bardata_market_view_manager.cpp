// =====================================================================================
// 
//       Filename:  bardata_market_view_manager.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  11/24/2022 06:38:18 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551 
// 
// =====================================================================================

#include "baseinfra/MarketAdapter/bardata_market_view_manager.hpp"

namespace HFSAT {

  BardataMarketViewManager::BardataMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                                                     const SecurityNameIndexer& t_sec_name_indexer_,
                                                     const std::vector<SecurityMarketView*>& t_security_market_view_map_, bool _use_self_book_):
    BaseMarketViewManager(t_dbglogger_, t_watch_, t_sec_name_indexer_, t_security_market_view_map_),
    trade_time_manager_(TradeTimeManager::GetUniqueInstance(t_sec_name_indexer_, t_watch_.YYYYMMDD())),
    sid_to_smm_map_(NULL){
  }

  void BardataMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
    for (auto i = 0u; i < security_market_view_map_.size(); i++) {
      SecurityMarketView *smv_ = security_market_view_map_[i];
      smv_->set_skip_listener_notification_end_time(r_start_time_);
    }
  }

  void BardataMarketViewManager::TriggerBardataUpdate(const uint32_t t_security_id_, const char* buffer){
    SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);

#define MAX_LINE_SIZE_LOCAL 1024
#define EXPECTED_TOKENS_LOCAL 6

    char local_buffer[MAX_LINE_SIZE_LOCAL];
    bzero(local_buffer, MAX_LINE_SIZE_LOCAL);
    memcpy((void*)&local_buffer, (void*)buffer, std::string(buffer).length());

    HFSAT::PerishableStringTokenizer pst(local_buffer, MAX_LINE_SIZE_LOCAL);
    std::vector<char const*> const& tokens = pst.GetTokens();
    if(tokens.size() < EXPECTED_TOKENS_LOCAL) return;

    if(std::atof(tokens[6]) > 0){
      smv_.trade_print_info_.trade_price_ = smv_.market_update_info_.mid_price_ = smv_.market_update_info_.bestbid_price_ = smv_.market_update_info_.bestask_price_ = std::atof(tokens[6]);
    }
    smv_.NotifyBardataListeners(t_security_id_, buffer);

    //We have to first wait for all theos to process the updates of the bar, even the -1 entry bar
    if(std::atof(tokens[6]) < 0){
      for(auto itr : *sid_to_smm_map_){
        itr->ProcessBardataRequestQueue();
      }
    }
  }
  void BardataMarketViewManager::TriggerOiBardataUpdate(const uint32_t t_security_id_, const char* buffer){
    SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);

#define MAX_LINE_SIZE_LOCAL 1024
#define EXPECTED_TOKENS_LOCAL 6

    char local_buffer[MAX_LINE_SIZE_LOCAL];
    bzero(local_buffer, MAX_LINE_SIZE_LOCAL);
    memcpy((void*)&local_buffer, (void*)buffer, std::string(buffer).length());

    HFSAT::PerishableStringTokenizer pst(local_buffer, MAX_LINE_SIZE_LOCAL);
    std::vector<char const*> const& tokens = pst.GetTokens();
    if(tokens.size() < EXPECTED_TOKENS_LOCAL) return;

    if(std::atof(tokens[3]) > 0){
      double const t_oi_price_ = std::atof(tokens[3]);
      smv_.NotifyOpenInterestListeners(t_oi_price_/100);
    }
    if(std::atof(tokens[3]) < 0){
      smv_.NotifyBardataListeners(t_security_id_, buffer);      
    }
    
    //smv_.NotifyBardataListeners(t_security_id_, buffer);

    //We have to first wait for all theos to process the updates of the bar, even the -1 entry ba
  }


}
