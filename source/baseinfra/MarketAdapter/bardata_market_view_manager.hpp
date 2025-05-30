// =====================================================================================
// 
//       Filename:  bardata_market_view_manager.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  11/24/2022 06:32:22 AM
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

#pragma once

#include <set>
#include <list>
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#include "baseinfra/SimMarketMaker/base_sim_market_maker.hpp"

namespace HFSAT {

  class BardataMarketViewManager : public BaseMarketViewManager,
                                   public NSEBardataListener, public BSEBardataListener {

    public :

      BardataMarketViewManager(DebugLogger& t_dbglogger_, const Watch& t_watch_,
                               const SecurityNameIndexer& t_sec_name_indexer_,
                               const std::vector<SecurityMarketView*>& t_security_market_view_map_, bool _use_self_book_);

      void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_);
      void TriggerBardataUpdate(const uint32_t t_security_id_, const char* buffer);
      void TriggerOiBardataUpdate(const uint32_t t_security_id_, const char* buffer);
      void UpdateBarDataSMMVec(std::vector<HFSAT::BaseSimMarketMaker *> *_sid_to_smm_map_){
        sid_to_smm_map_ = _sid_to_smm_map_;
      }

    private :
      TradeTimeManager& trade_time_manager_;
      SecurityMarketView* market_view_ptr_;
      std::vector<HFSAT::BaseSimMarketMaker *> *sid_to_smm_map_;

  };
}
