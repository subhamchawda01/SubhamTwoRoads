/**
    \file LoggedSources/nse_logged_message_filesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/external_data_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filenamer.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"


// Storage class of old size and price
struct Data {
  double price;
  int32_t size;
};

// Simple interface to maintain per security based historical size and price
class HistoryTracker {
 public:
  HistoryTracker() {}
  std::map<uint64_t, Data> order_id_data_;
};

namespace HFSAT {

/// @brief reads NSE logged data stored in file for this { symbol, tradingdate }
/// Reads NSE_MDS::NSECommonStruct
///    depending on NSE_MDS::NSECommonStruct::msgType
///    and for msgType == NSE_MDS::NSE_ORDER
///        depending on NSE_MDS::NSECommonStruct::data_::nse_order_::action_
///        calls the listeners, if any, of the security, action_
class NSELoggedMessageFileSource2 : public ExternalDataListener {
 protected:
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;
  const unsigned int security_id_;
  const char* exchange_symbol_;
  char datasource_symbol_[NSE_DOTEX_OFFLINE_SYMBOL_LENGTH];
  int curr_date_;
  OrderGlobalListenerNSE* p_order_global_listener_nse_;
  OrderGlobalListenerNSE* p_order_level_listener_sim_;

  ExternalTimeListener* p_time_keeper_;

  BulkFileReader bulk_file_reader_;
  NSE_MDS::NSETBTDataCommonStruct next_event_;
  HFSAT::MDS_MSG::GenericMDSMessage generic_msg_;
  TradingLocation_t trading_location_file_read_;
  int delay_usecs_to_add_;
  bool need_to_add_delay_usecs_;
  bool skip_intermediate_message_;  // Skip the intra-day recovery messages.
  bool actual_data_skip_;
  double price_multiplier;
  HFSAT::TradeType_t last_seen_delta_type_;
  HFSAT::Utils::NSEDailyTokenSymbolHandler &nse_daily_token_symbol_handler_;
  HFSAT::Utils::NSERefDataLoader &nse_refdata_loader_;
  std::map<char, std::map<int32_t, NSE_UDP_MDS::NSERefData>*> segment_to_nse_ref_data_;
  std::map<char, std::unordered_map<int32_t, int32_t> > segment_to_token_secid_map_;
  std::map<int32_t, HistoryTracker*> token_to_history_tracker;
  double low_exec_band_price;
  unsigned long int orderadd_msg_count;
  unsigned long int ordermodify_msg_count;
  unsigned long int ordercancel_msg_count;
  unsigned long long int trade_msg_count;
  unsigned long long int trade_execution_count;
  unsigned long int total_events;

 public:
  /**
   * @param t_dbglogger_ for logging errors
   * @param t_sec_name_indexer_ to detect if the security is of interest and not to process if not. If string matching
   * is more efficient we could use t_exchange_symbol_ as well.
   * @param t_preevent_YYYYMMDD_ tradingdate to load the appropriate file
   * @param t_security_id_ also same as t_sec_name_indexer_ [ t_exchange_symbol_ ]
   * @param t_exchange_symbol_ needed to match
   *
   * For now assuming t_exchange_symbol_ matching is not required
   */
  NSELoggedMessageFileSource2(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                             const unsigned int t_preevent_YYYYMMDD_, const unsigned int t_security_id_,
                             const char* t_exchange_symbol_, const TradingLocation_t r_trading_location_,
                             bool use_todays_data_ = false);

  inline int socket_file_descriptor() const { return 0; }

  void SetActualDataSkip(bool _actual_data_skip_){
    std::cout <<"Actual Skip Set To True" << std::endl;
    actual_data_skip_ = _actual_data_skip_;
  }

  void SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_);

  void ComputeEarliestDataTimestamp(bool& t_hasevents_);

  void ProcessAllEvents();

  void ProcessEventsTill(const ttime_t t_endtime_);

  inline void SetOrderGlobalListenerNSE(OrderGlobalListenerNSE* p_new_listener_) {
    p_order_global_listener_nse_ = p_new_listener_;
  }
  inline void SetOrderLevelListenerSim(OrderGlobalListenerNSE* p_new_listener_) {
    p_order_level_listener_sim_ = p_new_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* t_new_listener_) { p_time_keeper_ = t_new_listener_; }

  inline void SetTimeToSkipUntilFirstEvent(const ttime_t r_start_time_) {
    if (p_order_global_listener_nse_) {
      p_order_global_listener_nse_->SetTimeToSkipUntilFirstEvent(r_start_time_);
    }

    if (p_order_level_listener_sim_) {
      p_order_level_listener_sim_->SetTimeToSkipUntilFirstEvent(r_start_time_);
    }
  }

  void InitalizeRefData(){

    //std::cout << "InitalizeRefData \n";
    segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();

    std::map<int32_t, NSE_UDP_MDS::NSERefData>& fo_nse_ref_data = *(segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING]);

    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_FO_SEGMENT_MARKING)) {
      std::ostringstream internal_symbol_str;
      fo_nse_ref_data[itr.first] = itr.second;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "NSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "NSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);
      }
      std::string exchange_symbol =
          NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

      int32_t security_id = -1;
      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());

        //std::cout << " exchange_symbol " << exchange_symbol  << " SEC ID " << security_id << " TOKEN " << itr.second.token <<std::endl;
        segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));
      }
    }
  }


 private:
  void _ProcessThisMsg();
};
}
