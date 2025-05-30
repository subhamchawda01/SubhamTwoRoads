// =====================================================================================
//
//       Filename:  combined_mds_messages_bse_source.cpp
//
//    Description:  A data receiving interface which then processes the data and passes events to books
//
//        Version:  1.0
//        Created:  01/30/2014 01:06:07 PM
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

#include <iostream>
#include <unordered_map>
#include "dvccode/CDef/assumptions.hpp"  //Only include this file on classes which actually holds assumptions, this simplifies the search at any point where one needs to knwo what assumptions have been made
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/bse_refdata_loader.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/recovery_manager_config_parser.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"

//#include "dvccode/Profiler/cpucycle_profiler.hpp"

namespace HFSAT {
namespace BSE {

// Storage class of old size and price
struct Data {
  int32_t price;
  int32_t size;
};

// Simple interface to maintain per security based historical size and price
class HistoryTracker {
 public:
  HistoryTracker() {}
  std::map<uint64_t, Data> order_id_data_;
};

class CombinedMDSMessagesBSEProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  // Indication of whether the recovery has been complete or not and what was the last sequenced applied
  // can be switched to bool later
  double security_id_to_inverse_price_mult_[DEF_MAX_SEC_ID];

  OrderGlobalListenerBSE* p_order_global_listener_bse_;  ///< Listeners of BSE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  // This classes are useful to access per security based refdata
  HFSAT::Utils::BSEDailyTokenSymbolHandler& bse_daily_token_symbol_handler_;
  HFSAT::Utils::BSERefDataLoader& bse_refdata_loader_;

  // This contains recovery packets received from RecoveryHost
  char* recovery_buffer;

  // It's goot to load and store this once as it's not going to be changed
  std::map<char, std::map<int32_t, BSE_UDP_MDS::BSERefData>*> segment_to_bse_ref_data_;
  std::map<char, std::unordered_map<int32_t, int32_t> > segment_to_token_secid_map_;
  TradeTimeManager& trade_time_manager_;

  // client side recovery purpose
  HFSAT::RecoveryManagerConfigParser recovery_manager_config_parser_;

 public:
  CombinedMDSMessagesBSEProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        bse_daily_token_symbol_handler_(HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        bse_refdata_loader_(HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        recovery_buffer(new char[MAX_RECOVERY_PACKET_SIZE]),
        segment_to_bse_ref_data_(),
        segment_to_token_secid_map_(),
        trade_time_manager_(TradeTimeManager::GetUniqueInstance(
            _sec_name_indexer_, HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        recovery_manager_config_parser_() {
    // Note, We are here taking advantage of the fact that -1 will be all FF... so we are not setting it per byte basis
    // but rather
    // total number of elements only

    // TODO - MULTISHM
    memset((void*)security_id_to_inverse_price_mult_, 0, DEF_MAX_SEC_ID);

    timeval tv;
    gettimeofday(&tv, NULL);
    //int curr_hhmm = DateTime::GetUTCHHMMFromTime(tv.tv_sec);

    // Reset the buffer
    memset((void*)recovery_buffer, 0, MAX_RECOVERY_PACKET_SIZE);

    // Initialize
    segment_to_bse_ref_data_[BSE_EQ_SEGMENT_MARKING] = new std::map<int32_t, BSE_UDP_MDS::BSERefData>();
    segment_to_bse_ref_data_[BSE_FO_SEGMENT_MARKING] = new std::map<int32_t, BSE_UDP_MDS::BSERefData>();
//    segment_to_bse_ref_data_[BSE_CD_SEGMENT_MARKING] = new std::map<int32_t, BSE_UDP_MDS::BSERefData>();

    std::map<int32_t, BSE_UDP_MDS::BSERefData>& eq_bse_ref_data = *(segment_to_bse_ref_data_[BSE_EQ_SEGMENT_MARKING]);
    std::map<int32_t, BSE_UDP_MDS::BSERefData>& fo_bse_ref_data = *(segment_to_bse_ref_data_[BSE_FO_SEGMENT_MARKING]);
    //std::map<int32_t, BSE_UDP_MDS::BSERefData>& cd_bse_ref_data = *(segment_to_bse_ref_data_[BSE_CD_SEGMENT_MARKING]);

    for (auto& itr : bse_refdata_loader_.GetBSERefData(BSE_EQ_SEGMENT_MARKING)) {
      eq_bse_ref_data[itr.first] = itr.second;

      std::ostringstream internal_symbol_str;
      internal_symbol_str << "BSE"
                          << "_" << (itr.second).symbol;
      std::string exchange_symbol =
          BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
      int32_t security_id = -1;

      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        
        segment_to_token_secid_map_[BSE_EQ_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));

        if (security_id >= 0) {
          security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier);
        }
      }
    }
/*
    for (auto& itr : bse_refdata_loader_.GetBSERefData(BSE_CD_SEGMENT_MARKING)) {
      cd_bse_ref_data[itr.first] = itr.second;
      std::ostringstream internal_symbol_str;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << HFSAT::Utils::ConvertBSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << HFSAT::Utils::ConvertBSEExpiryInSecToDate((itr.second).expiry);
      }
      std::string exchange_symbol =
          BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
      int32_t security_id = -1;

      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        segment_to_token_secid_map_[BSE_CD_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));

        if (security_id >= 0) {
          security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier);
        }
      }
    }


*/

    for (auto& itr : bse_refdata_loader_.GetBSERefData(BSE_FO_SEGMENT_MARKING)) {
      std::ostringstream internal_symbol_str;
      fo_bse_ref_data[itr.first] = itr.second;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << HFSAT::Utils::ConvertBSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "BSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << HFSAT::Utils::ConvertBSEExpiryInSecToDate((itr.second).expiry);
      }
      std::string exchange_symbol =
          BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
      int32_t security_id = -1;
      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        segment_to_token_secid_map_[BSE_FO_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));
        if (security_id >= 0) {
          security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier);
        }
      }
    }

    std::unordered_map<std::string, int> bse_spot_index_2_token_map_ = HFSAT::BSESpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();
      if (0 == bse_spot_index_2_token_map_.size()) {
      dbglogger_ << "IX SEGMENT REF DATA NOT PRESENT" << "\n";
      dbglogger_.DumpCurrentBuffer();
      exit(-1);
    }

    for (auto& itr : bse_spot_index_2_token_map_) {
        std::string exchange_sym = "BSE_IDX" + std::to_string(itr.second);
        int security_id = sec_name_indexer_.GetIdFromSecname(exchange_sym.c_str());
        // std::cout<<"SEC ID: " << security_id << " TOKEN: " << itr.second  << " STR: " << exchange_sym << std::endl;

        segment_to_token_secid_map_[BSE_IX_SEGMENT_MARKING].insert(std::make_pair(itr.second, security_id));  // currently wrong// need to check other things
      }

    /*HFSAT::CpucycleProfiler::SetUniqueInstance(4);
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "OnOrderAdd");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "OnOrderModify");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "OnOrderDelete");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(4, "OnTrade");*/
  }

  ~CombinedMDSMessagesBSEProcessor() {
    // std::cout << HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();
  }

  inline void SetOrderGlobalListenerBSE(OrderGlobalListenerBSE* p_new_listener_) {
    p_order_global_listener_bse_ = p_new_listener_;
  }

  void ResetBseRecoveryIndex(){
/*
      for (unsigned int sec_id = 0; sec_id < sec_name_indexer_.NumSecurityId(); sec_id++) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "DISABLING RECOVERY MANAGER FOR SecId: "<< sec_id << DBGLOG_ENDL_NOFLUSH;
        security_id_to_recovery_complete_[sec_id] = 0;
      }
*/
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }


  inline void ProcessBSEEvent(EOBI_MDS::EOBICommonStruct* next_event_) {
    //std::cout << "ProcessBSEEvent: \n" << next_event_->ToString() << std::endl;
    // There will be a few instruments for which datasymbol/exchange won't be defined, TODO : use bool to reduce latency

    if (segment_to_token_secid_map_[next_event_->segment_type].find(next_event_->token_) ==
        segment_to_token_secid_map_[next_event_->segment_type].end()) {
      //std::cout << " Token To Segment: " << next_event_->token_ << " Segment : " << next_event_->segment_type << std::endl;
      return;
     }

    int32_t security_id_ = segment_to_token_secid_map_[next_event_->segment_type][next_event_->token_];
    if (security_id_ < 0) return;

    switch (next_event_->order_.action_) {
      case '0': {
        // Order New
        // Update watch
        TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_->source_time);

        p_order_global_listener_bse_->OnOrderAdd(security_id_, t_buysell_, next_event_->order_.priority_ts, next_event_->order_.price,
                                                        next_event_->order_.size,
                                                        next_event_->order_.intermediate_);

      } break;
      case '1': {
	//ORDER_MODIFY:
	//ORDER_MODIFY_SAME_PRIORITY:
        TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_->source_time);
        p_order_global_listener_bse_->OnOrderModify(

                security_id_, t_buysell_, next_event_->order_.prev_priority_ts, next_event_->order_.priority_ts,
                next_event_->order_.price, next_event_->order_.size,

                next_event_->order_.prev_price, next_event_->order_.prev_size);

 
      } break;

      case '2': {
	//ORDER_DELETE:
	
        TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_->source_time);

	p_order_global_listener_bse_->OnOrderDelete(security_id_, t_buysell_, next_event_->order_.priority_ts, next_event_->order_.price, //  (int)next_event_->order_.size, 
                                      true, false);


      } break;

      case '3': {
	//ORDER_MASS_DELETE:

         // TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;

        p_time_keeper_->OnTimeReceived(next_event_->source_time);
	p_order_global_listener_bse_->OnOrderMassDelete(security_id_);
      } break;

      case '4': {
	//PARTIAL_EXECUTION:
        TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_->source_time);

//	p_order_global_listener_bse_->OnPartialOrderExecution(
        p_order_global_listener_bse_->OnTrade(
                security_id_, t_buysell_, next_event_->order_.priority_ts, next_event_->order_.price, next_event_->order_.size);


      } break;

      case '5': {
	//FULL_EXECUTION:
	
        TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_->source_time);

//	p_order_global_listener_bse_->OnFullOrderExecution(  we could probabaly take advantage of this
        p_order_global_listener_bse_->OnTrade(
                security_id_, t_buysell_, next_event_->order_.priority_ts, next_event_->order_.price, next_event_->order_.size);


      } break;

      case '6': {
	//EXECUTION_SUMMARY:
	
        TradeType_t t_buysell_ = next_event_->order_.side == 'B' ? kTradeTypeBuy : kTradeTypeSell;
        p_time_keeper_->OnTimeReceived(next_event_->source_time);
	p_order_global_listener_bse_->OnExecutionSummary(security_id_, t_buysell_, next_event_->order_.price,
                                                              next_event_->order_.size);

      } break;
      case '8': {
	//BSE_SPOT_INDEX_UPDATES:
          //std::cout << "UPDATING INDEX NOW " << std::endl;
	        p_time_keeper_->OnTimeReceived(next_event_->source_time);
          //std::cout << "SENSEX INDEX VALUE " << (double)next_event_->order_.price << std::endl; 
	        p_order_global_listener_bse_->OnIndexPrice(security_id_,(double)next_event_->order_.price);
      } break;
      case '9': {
          //std::cout << "UPDATING OI NOW " << std::endl;
          p_time_keeper_->OnTimeReceived(next_event_->source_time);
          p_order_global_listener_bse_->OnOpenInterestUpdate(security_id_,(double)next_event_->order_.size);
      }break;

      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TYPE OF MESSAGE RECEVIED : " << (int32_t)(next_event_->order_.action_)
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      } break;
    }
  }
};
}
}
