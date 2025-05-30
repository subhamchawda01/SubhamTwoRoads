// =====================================================================================
//
//       Filename:  bse_tbt_data_processor.hpp
//
//    Description:  This class is a processed events handler and based on input mode performs actions,
//                  i.e Logger mode logs data to files, ComShm mode enables write to combined shm queue etc.
//                  Simply and interface of what to do with the processed events
//
//        Version:  1.0
//        Created:  09/18/2015 10:17:36 AM
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
#include <cstdlib>
#include <unordered_set>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/bse_mds_shm_interface.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/lock.hpp"
//#include "infracore/BSEMD/bse_tbt_templates.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Listeners/live_products_manager_listener.hpp"
#include "infracore/Tools/live_products_manager.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

#define MAX_BUFFERED_EVENTS_STORAGE 8192

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

namespace HFSAT {
namespace BSEMD {

class MarketEventListener {
 public:
  ~MarketEventListener() {}
  virtual void OnMarketEventDispatch(EOBI_MDS::EOBICommonStruct* market_event) {
    std::cout << "SHOULDN'T POINT HERE: " << std::endl;
    std::exit(-1);
  }
};

class BSETBTDataProcessor : public HFSAT::LiveProductsManagerListener {
 private:

//This function is called when the recovery needs to be stopped. 
//It then processes all the normal mkt data which are stored in buffer during the recovery.
void EndRecovery(int64_t security_id_) {
  timeval current_time;

  gettimeofday(&current_time, NULL);

  for (size_t i = 0; i < message_buffer_.size(); i++) {
    std::pair<EOBI_MDS::EOBICommonStruct*, int64_t> this_pair_ = message_buffer_[i];
    EOBI_MDS::EOBICommonStruct* this_cstr_ = this_pair_.first;

    if (this_cstr_ == NULL) {
      std::cout << "memory error during free" << std::endl;
      return;
    }

    std::pair<uint64_t, uint64_t> price_pair_ = price_buffer_[i];

    o_security_id_ = this_pair_.second;
    o_action_ = this_cstr_->order_.action_;
    o_seq_num_ = this_cstr_->order_.msg_seq_num_;
    o_side_ = this_cstr_->order_.side == 'B' ? 1 : 2;
    o_priority_ts_ = this_cstr_->order_.priority_ts;
    // o_price_            = (uint64_t) (this_cstr_->order_.price * EOBI_PRICE_DIVIDER);
    o_price_ = price_pair_.first;
    o_size_ = this_cstr_->order_.size;
    o_prev_priority_ts_ = this_cstr_->order_.prev_priority_ts;
    // o_prev_price_       = (uint64_t) (this_cstr_->order_.prev_price * EOBI_PRICE_DIVIDER);
    o_prev_price_ = price_pair_.second;
    o_prev_size_ = this_cstr_->order_.prev_size;
    o_intermediate_ = this_cstr_->order_.intermediate_;
    o_trd_reg_ts_ = this_cstr_->order_.trd_reg_ts;

    int market_segment_id_ = sec_to_mkt_seg_id_[this_pair_.second];

    if (this_cstr_->order_.msg_seq_num_ > mkt_seg_id_to_seq_num_[market_segment_id_]) {
      //ProcessOrder();
      OnMarketEvent(this_cstr_);
    }

    delete this_cstr_;
  }

  std::cout << "\n"
            << "Buffered messages processed."
            << "\n" << std::endl;

  message_buffer_.clear();
  price_buffer_.clear();
  initial_recovery_not_finished_ = false;

  //EndRecoveryForAllProducts();

  security_in_recovery[security_id_] = false;
}


  void ProcessEventsForLoggerMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
/*    if (true == using_simulated_clocksource_) {
      bse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&bse_tbt_data_common_struct_->source_time, NULL);
    }
*/
    data_logger_thread_->log(*bse_tbt_data_common_struct);
  }

  void ProcessEventsForComShmMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    if (tokens_requested_.find(bse_tbt_data_common_struct->token_) != tokens_requested_.end()) {
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_, false);
    }
    if (true == is_logging_enabled_) {
      generic_logger_->Log(*generic_mds_message_);
    }
  }

  void ProcessEventsForProShmMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    bse_mds_shm_interface_->WriteGenericStruct(bse_tbt_data_common_struct);
  }

  void ProcessEventsForRawMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    if (tokens_requested_raw_mode_.find(bse_tbt_data_common_struct->token_) != tokens_requested_raw_mode_.end()) {
      market_event_listener_->OnMarketEventDispatch(bse_tbt_data_common_struct);
    }
  }

  void ProcessEvetnsForConvertLoggerMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    ProcessEventsForProShmMode(bse_tbt_data_common_struct);
    // Log it

    memcpy((void*)(&(generic_mds_message_->generic_data_).bse_data_), (void*)bse_tbt_data_common_struct,
           sizeof(EOBI_MDS::EOBICommonStruct));
    generic_logger_->Log(*generic_mds_message_); 
  }

  void ProcessEvetnsForDataMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    multicast_sender_socket_->WriteN(sizeof(EOBI_MDS::EOBICommonStruct), (void*)bse_tbt_data_common_struct);
  }

  void UpdateBardataStruct(std::string& shortcode, EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    if (shortcode_to_bardata_map_.find(shortcode) == shortcode_to_bardata_map_.end()) {
      shortcode_to_bardata_map_[shortcode] = new EOBI_MDS::BSEBarDataCommonStruct();
      EOBI_MDS::BSEBarDataCommonStruct* temp_bardata_struct_ = shortcode_to_bardata_map_[shortcode];
      // int bardata_time_ = bse_tbt_data_common_struct->source_time.tv_sec - (bse_tbt_data_common_struct->source_time.tv_sec % bardata_period_);
      
      int bardata_time_ = bse_tbt_data_common_struct->source_time.tv_sec - (bse_tbt_data_common_struct->source_time.tv_sec % bardata_period_);
      int expiry_ = HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode);

      temp_bardata_struct_->bardata_time = bardata_time_;
      strcpy(temp_bardata_struct_->shortcode, shortcode.c_str());

      temp_bardata_struct_->start_time = bse_tbt_data_common_struct->source_time.tv_sec;
      temp_bardata_struct_->close_time = bse_tbt_data_common_struct->source_time.tv_sec;
      temp_bardata_struct_->expiry = expiry_;

      temp_bardata_struct_->open_price = bse_tbt_data_common_struct->order_.price;
      temp_bardata_struct_->close_price = bse_tbt_data_common_struct->order_.price;
      temp_bardata_struct_->low_price = bse_tbt_data_common_struct->order_.price;
      temp_bardata_struct_->high_price = bse_tbt_data_common_struct->order_.price;
      temp_bardata_struct_->volume = bse_tbt_data_common_struct->order_.size;
      
      temp_bardata_struct_->trades = 1;
/*
      std::cout << "if:UpdateBardataStruct: " << temp_bardata_struct_->shortcode << " " << temp_bardata_struct_->bardata_time << " " 
                << temp_bardata_struct_->start_time << " " << temp_bardata_struct_->close_time << " " << temp_bardata_struct_->expiry <<  " " 
                << temp_bardata_struct_->open_price << " " << temp_bardata_struct_->close_price << " " << temp_bardata_struct_->low_price << " " 
                << temp_bardata_struct_->high_price << " " << temp_bardata_struct_->volume << " " << temp_bardata_struct_->trades << std::endl;
*/
    } else {
      EOBI_MDS::BSEBarDataCommonStruct* temp_bardata_struct_ = shortcode_to_bardata_map_[shortcode];

      temp_bardata_struct_->close_time = bse_tbt_data_common_struct->source_time.tv_sec;

      temp_bardata_struct_->close_time = bse_tbt_data_common_struct->source_time.tv_sec;
      temp_bardata_struct_->close_price = bse_tbt_data_common_struct->order_.price;
      if (temp_bardata_struct_->low_price > (bse_tbt_data_common_struct->order_.price))
        temp_bardata_struct_->low_price = bse_tbt_data_common_struct->order_.price;
      if (temp_bardata_struct_->high_price < (bse_tbt_data_common_struct->order_.price))
        temp_bardata_struct_->high_price = bse_tbt_data_common_struct->order_.price;
      temp_bardata_struct_->volume += bse_tbt_data_common_struct->order_.size;
            
      temp_bardata_struct_->trades += 1;
/*
      std::cout << "else:UpdateBardataStruct: " << temp_bardata_struct_->shortcode << " " << temp_bardata_struct_->bardata_time << " " 
                << temp_bardata_struct_->start_time << " " << temp_bardata_struct_->close_time << " " << temp_bardata_struct_->expiry <<  " " 
                << temp_bardata_struct_->open_price << " " << temp_bardata_struct_->close_price << " " << temp_bardata_struct_->low_price << " " 
                << temp_bardata_struct_->high_price << " " << temp_bardata_struct_->volume << " " << temp_bardata_struct_->trades << std::endl;
*/
    }

  }


  void ProcessEventsForBardataMode(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {


    std::string exchsymbol = HFSAT::BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(bse_tbt_data_common_struct->getContract());
    std::string shortcode = HFSAT::BSESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchsymbol);

    // int32_t token_ = bse_tbt_data_common_struct->token_;
    // std::string shortcode = HFSAT::BSESecurityDefinitions::GetShortcodeFromToken(token_);

    // std::cout << "token: " << token_
    //           << " shortcode: " << shortcode
    //           <<"\n";


    if ((exchsymbol == "INVALID") || (shortcode == "INVALID")) { return; }

    if ( is_bkx_last_week_ ) {
      if ( (shortcode.find("_BKX_") != string::npos) &&
           ((shortcode.find("_C0_") != string::npos) || (shortcode.find("_P0_") != string::npos)) && (shortcode.find("_W") == string::npos) ) {
        int expiry_date_tmp = HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
        //std::cout << "is_bkx_last_week_: " << is_bkx_last_week_ << " SHORTCODE: " << shortcode << " expiry_date_tmp: " << expiry_date_tmp << std::endl;
	//std::cout << "expiry_date_tmp: " << expiry_date_tmp << " bkx_last_week_expiry_: " << bkx_last_week_expiry_ << std::endl;
        if ( expiry_date_tmp == bkx_last_week_expiry_ ) {
           std::string shortcode_w = shortcode + "_W";  
           UpdateBardataStruct(shortcode_w, bse_tbt_data_common_struct);
        } 
      }
    }
    if ( is_bsx_last_week_ ) { 
      if ( (shortcode.find("_BSX_") != string::npos) && 
           ((shortcode.find("_C0_") != string::npos) || (shortcode.find("_P0_") != string::npos)) && (shortcode.find("_W") == string::npos)) {
        int expiry_date_tmp = HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
        //std::cout << "is_bsx_last_week_: " << is_bsx_last_week_ << " SHORTCODE: " << shortcode << " expiry_date_tmp: " << expiry_date_tmp << std::endl;
	//std::cout << "expiry_date_tmp: " << expiry_date_tmp << " bsx_last_week_expiry_: " << bsx_last_week_expiry_ << std::endl;
        if ( expiry_date_tmp == bsx_last_week_expiry_ ) {
           std::string shortcode_w = shortcode + "_W";  
           UpdateBardataStruct(shortcode_w, bse_tbt_data_common_struct);
        } 
      }
    }

    //std::cout << "ProcessEventsForBardataMode:shortcode: " << shortcode << " exchsymbol: " << exchsymbol << std::endl;
    UpdateBardataStruct(shortcode, bse_tbt_data_common_struct);

    // std::cout << bse_tbt_data_common_struct->getShortcode() << "\n";
    
  }
 public:

  void DumpBardata() {
    bardata_logger_thread_->logBardata(shortcode_to_bardata_map_);
    shortcode_to_bardata_map_.clear();
  }
  
  void UpdateProcessedBardataMap() {
    std::cout << "UpdateProcessedBardataMap(): " << shortcode_to_bardata_map_.size() << std::endl;
    for (auto map_itr : shortcode_to_bardata_map_) {
/*
      if (shortcode_to_control_lock_.end() == shortcode_to_control_lock_.find(map_itr.first)) {
        shortcode_to_control_lock_[map_itr.first] = new HFSAT::Lock();
      }
      // Lock shortcode control
      shortcode_to_control_lock_[map_itr.first]->LockMutex();
*/
      int32_t bardata_time = map_itr.second->bardata_time;
      shortcode_to_live_bardata_map_[map_itr.first][bardata_time] = map_itr.second;
/*
      if (map_itr.first.find("NIFTYBANK") != std::string::npos) {
        std::cout << "UpdateProcessedBardataMap: " << map_itr.first << " size_map_shortcode: " << shortcode_to_live_bardata_map_.size()
                  << " size_map_bardata: " << shortcode_to_live_bardata_map_[map_itr.first].size() << std::endl;
      }
*/
      // Time to release this shortcode lock
//      shortcode_to_control_lock_[map_itr.first]->UnlockMutex();
    }
  }

  void AddBardataToRecoveryMap() {
    std::cout << "AddBardataToRecoveryMap()" << std::endl;
    UpdateProcessedBardataMap();
    shortcode_to_bardata_map_.clear();
  }

  bool SendEmptyBardataUpdate(std::string& shortcode_, int32_t const& start_hhmm_, int32_t const& end_hhmm_, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    std::cout << "IsShortcodeAvailableForRecovery false: " << std::endl;
    EOBI_MDS::BSEBarDataCommonStruct temp_bardata_struct_;
    temp_bardata_struct_.bardata_time = -1;
    strcpy(temp_bardata_struct_.shortcode, shortcode_.c_str());
    temp_bardata_struct_.start_time = -1;
    temp_bardata_struct_.close_time = -1;
    temp_bardata_struct_.expiry = -1; 
    temp_bardata_struct_.open_price = -1;
    temp_bardata_struct_.close_price = -1;
    temp_bardata_struct_.low_price = -1;
    temp_bardata_struct_.high_price = -1;
    temp_bardata_struct_.volume = -1; 
    temp_bardata_struct_.trades = -1;
       
    DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING BARDATA RECOVERY REQUEST FOR : " << shortcode_
                                << " LIVE BARDATA : " << "1"
                                << " LIVE SIZE : " << sizeof(EOBI_MDS::BSEBarDataCommonStruct)
                                << " START_TIME: " << start_hhmm_ 
                                << " END_TIME: " << end_hhmm_
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    // Check whether the buffer storage can hold enough elements
    if ((int32_t)(4 + (sizeof(EOBI_MDS::BSEBarDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
      DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size " << sizeof(EOBI_MDS::BSEBarDataCommonStruct)
        << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    int32_t data_size = sizeof(EOBI_MDS::BSEBarDataCommonStruct);

    // Insert length to buffer
    memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

    DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    data_packet += 4;

    // Fill up the buffer with data
    memcpy((void*)data_packet, (void*)&(temp_bardata_struct_), sizeof(EOBI_MDS::BSEBarDataCommonStruct));
    data_packet += sizeof(EOBI_MDS::BSEBarDataCommonStruct);

    DBGLOG_CLASS_FUNC_LINE_INFO << "SENDING MAX SEQUENCE IN RECOVERY AS :" << end_hhmm_ << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    return true;
  }

  bool FetchProcessedBardataMap(std::string& temp_shortcode_, int32_t const& start_hhmm_, int32_t const& end_hhmm_, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    // Acquire lock to copy events
    std::string shortcode_;
    HFSAT::PerishableStringTokenizer::TrimString(temp_shortcode_.c_str(),shortcode_);
/*
    if (shortcode_to_control_lock_.end() == shortcode_to_control_lock_.find(shortcode_)) {
      shortcode_to_control_lock_[shortcode_] = new HFSAT::Lock();
    }
*/
    std::cout << "shortcode_:" << shortcode_ << " start_hhmm_: " << start_hhmm_ << " end_hhmm_: " << end_hhmm_
              << " shortcode_to_live_bardata_map_.size(): " << shortcode_to_live_bardata_map_.size()
              << " shortcode_to_live_bardata_map_.size():bardata: " << shortcode_to_live_bardata_map_[shortcode_].size() << std::endl;
    // Lock token control
//    shortcode_to_control_lock_[shortcode_]->LockMutex();
    if (shortcode_to_live_bardata_map_.end() != shortcode_to_live_bardata_map_.find(shortcode_)) {
      std::cout << "IsShortcodeAvailableForRecovery true: " << std::endl;
      // It's time to now construct buffer
      std::map<int32_t, EOBI_MDS::BSEBarDataCommonStruct*> live_bardata = shortcode_to_live_bardata_map_[shortcode_];

      int bardata_count = 0;
      for (auto& itr : live_bardata) {
        int32_t bardata_time = (itr.second)->bardata_time;
        if ((bardata_time >= start_hhmm_) && (bardata_time <= end_hhmm_)) {
          ++bardata_count;
        }
      }

      if ((live_bardata.size() == 0) || (bardata_count == 0)) {
        std::cout << "SendEmptyBardataUpdate:size: " << live_bardata.size() << " count: " << bardata_count << std::endl;
        return SendEmptyBardataUpdate(shortcode_, start_hhmm_, end_hhmm_, data_packet, MAX_RECOVERY_PACKET_SIZE);
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING BARDATA RECOVERY REQUEST FOR : " << shortcode_
                                  << " LIVE BARDATA : " << live_bardata.size()
                                  << " LIVE SIZE : " << live_bardata.size() * sizeof(EOBI_MDS::BSEBarDataCommonStruct)
                                  << " START_TIME: " << start_hhmm_
                                  << " END_TIME: " << end_hhmm_
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if (live_bardata.size() == 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size 0 "<< DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        return false;
      }
      // Check whether the buffer storage can hold enough elements
      if ((int32_t)(4 + (live_bardata.size() * sizeof(EOBI_MDS::BSEBarDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
        DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size " << live_bardata.size() * sizeof(EOBI_MDS::BSEBarDataCommonStruct)
          << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        return false;
      }

      int32_t data_size = bardata_count * sizeof(EOBI_MDS::BSEBarDataCommonStruct);

      // Insert length to buffer
      memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

      DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      data_packet += 4;

      int32_t max_bardata_time = 0;

      // Fill up the buffer with data
      for (auto& itr : live_bardata) {
        int32_t bardata_time = (itr.second)->bardata_time;
        if ((bardata_time >= start_hhmm_) && (bardata_time <= end_hhmm_)) {
          max_bardata_time = std::max(max_bardata_time, bardata_time);
          memcpy((void*)data_packet, (void*)&(*(itr.second)), sizeof(EOBI_MDS::BSEBarDataCommonStruct));
          data_packet += sizeof(NSE_MDS::NSEBarDataCommonStruct);
        }
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "SENDING MAX BARDATA TIME IN RECOVERY AS :" << max_bardata_time << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }
    else {
      return SendEmptyBardataUpdate(shortcode_, start_hhmm_, end_hhmm_, data_packet, MAX_RECOVERY_PACKET_SIZE);
    }

    // Time to release this shortcode lock
//    shortcode_to_control_lock_[shortcode_]->UnlockMutex();

    // Request was handled successfully
    return true;
  }

  void AddShortCodeForProcessing(std::string shortcode) {
    std::string exchange_symbol = GetExchangeSymbol(shortcode);
    std::string internal_symbol = BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
    if (internal_symbol == std::string("INVALID")) {
      std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
      return;
    }

    char segment = BSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
    int32_t token = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

    if (token != -1 /*Dummy Token*/) {
       //std::cout << "REQUESTING TBT DATA FOR TOKEN " << token << " FOR SHC : " << shortcode << std::endl;
       tokens_requested_raw_mode_.insert(token);
    } else {
      std::cerr << " Invalid Token for shortcode " << shortcode << std::endl;
    }
  }

 private:

  void OnLiveProductChange(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                           ExchSource_t exchange) {
    if ((daemon_mode_ == HFSAT::kComShm || daemon_mode_ == HFSAT::kProShm || daemon_mode_ == HFSAT::kConvertLogger) &&
        (exchange == kExchSourceBSE || exchange == kExchSourceBSE_FO || exchange == kExchSourceBSE_CD ||
         exchange == kExchSourceBSE_EQ)) {
      std::string exchange_symbol = GetExchangeSymbol(shortcode);
      std::string internal_symbol = BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
      if (internal_symbol == std::string("INVALID")) {
        std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
        return;
      }

      char segment = BSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
      int32_t token = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

      if (token != -1 /*Dummy Token*/) {
        if (action == LiveProductsChangeAction::kAdd) {
          tokens_requested_.insert(token);
        } else if (action == LiveProductsChangeAction::kRemove) {
          if (tokens_requested_.find(token) != tokens_requested_.end()) {
            tokens_requested_.erase(token);
          }
        }
      } else {
        std::cerr << " Invalid Token for shortcode " << shortcode << std::endl;
      }
    }
  }

  const char* GetExchangeSymbol(const std::string& shortcode) {
    const char* exchange_symbol = nullptr;
    try {
      exchange_symbol = ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode);

    } catch (...) {
      std::cerr << " ReqCW: BSETBTDataProcessor: Error in ExchangeSymbol Conversion for Shortcode : " << shortcode
                << std::endl;
    }

    return exchange_symbol;
  }

 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::FastMdConsumerMode_t daemon_mode_;


  // For Logger Mode//
  MDSLogger<EOBI_MDS::EOBICommonStruct>* data_logger_thread_;
  MDSLogger<EOBI_MDS::BSEBarDataCommonStruct>* bardata_logger_thread_;

  // For ComShm Mode
  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::Utils::BSEMDSShmInterface* bse_mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;
  EOBI_MDS::EOBICommonStruct* bse_mds_message_;
  MarketEventListener* market_event_listener_;

  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;

  bool is_logging_enabled_;

  // For DataMode
  HFSAT::MulticastSenderSocket* multicast_sender_socket_;

  EOBI_MDS::EOBICommonStruct dummy_struct;

  typedef void (BSETBTDataProcessor::*ProcessEventForModes)(
      EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct);
  ProcessEventForModes process_event_for_modes_;

  std::unordered_set<int32_t> tokens_requested_;
  std::unordered_set<int32_t> tokens_requested_raw_mode_;

  std::unordered_map<std::string, EOBI_MDS::BSEBarDataCommonStruct*> shortcode_to_bardata_map_;

  std::unordered_map<std::string, std::map<int32_t, EOBI_MDS::BSEBarDataCommonStruct*>> shortcode_to_live_bardata_map_;
  

  bool is_local_data_;

  uint32_t bardata_period_;

  int bkx_last_week_expiry_;
  int bsx_last_week_expiry_;
  bool is_bkx_last_week_;
  bool is_bsx_last_week_;

  BSETBTDataProcessor(HFSAT::DebugLogger& dbglogger, HFSAT::FastMdConsumerMode_t const& daemon_mode,
                      HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                      MarketEventListener* market_event_listener = nullptr)
      : dbglogger_(dbglogger),
        daemon_mode_(daemon_mode),
        data_logger_thread_(NULL),
        mds_shm_interface_(nullptr),
        bse_mds_shm_interface_(nullptr),
        generic_mds_message_(NULL),
        bse_mds_message_(NULL),
        market_event_listener_(market_event_listener),
        clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
        using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()),
        is_logging_enabled_(true), 
        multicast_sender_socket_(NULL),
        shortcode_to_bardata_map_(),
        shortcode_to_live_bardata_map_(),
        bardata_period_(60),
        bkx_last_week_expiry_(-1),
        bsx_last_week_expiry_(-1),
        is_bkx_last_week_(false),
        is_bsx_last_week_(false),
        initial_recovery_not_finished_(true),
        sec_to_mkt_seg_id_(),
	curr_sec_id_(),
	instr_summry_rcvd_(),
	security_in_recovery(),
        mkt_seg_id_to_seq_num_(),
	target_msg_seq_num_(),
        bse_daily_token_symbol_handler_(
            HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
        generic_logger_(nullptr) {
    message_buffer_.clear();
    price_buffer_.clear();

    gettimeofday(&logger_start_time_, NULL);
    // int curr_hhmm = DateTime::GetUTCHHMMFromTime(logger_start_time_.tv_sec);
      
    is_local_data_ = TradingLocationUtils::GetTradingLocationExch(kExchSourceBSE) ==
                     TradingLocationUtils::GetTradingLocationFromHostname();

    std::string bkx_mnt_shortcode = "BSE_BKX_C0_A";
    std::string bkx_weekly_shortcode = "BSE_BKX_C0_A_W";
    std::string bsx_mnt_shortcode = "BSE_BSX_C0_A";
    std::string bsx_weekly_shortcode = "BSE_BSX_C0_A_W";

    std::cout << "BSETBTDataProcessor:is_bkx_last_week_: " << is_bkx_last_week_ << " monthly: " << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bkx_mnt_shortcode) << " weekly: " << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bkx_weekly_shortcode) << std::endl;
    std::cout << "BSETBTDataProcessor:is_bsx_last_week_: " << is_bsx_last_week_ << " monthly: " << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bsx_mnt_shortcode) << " weekly: " << HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bsx_weekly_shortcode) << std::endl;

    if ( HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bkx_mnt_shortcode) == HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bkx_weekly_shortcode) ) {
      bkx_last_week_expiry_ = HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bkx_mnt_shortcode);
      is_bkx_last_week_ = true;
    }

    if ( HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bsx_mnt_shortcode) == HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bsx_weekly_shortcode) ) {
      bsx_last_week_expiry_ = HFSAT::BSESecurityDefinitions::GetExpiryFromShortCode(bsx_mnt_shortcode);
      is_bsx_last_week_ = true;
    }

    switch (daemon_mode_) {
      case HFSAT::kLogger: {
        data_logger_thread_ = new MDSLogger<EOBI_MDS::EOBICommonStruct>("BSETBT");
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEventsForLoggerMode;
      } break;

      case HFSAT::kBardataLogger: {
        std::cout << "CREATED MDSLogger\n";
        bardata_logger_thread_ = new MDSLogger<EOBI_MDS::BSEBarDataCommonStruct>("BARDATA");
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEventsForBardataMode;
        bardata_logger_thread_->EnableAffinity("BSEBardataLogger");
        bardata_logger_thread_->run();
      } break;

      case HFSAT::kBardataRecoveryHost: {
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEventsForBardataMode;

      } break;

      case HFSAT::kMcast: {
        HFSAT::NetworkAccountInfoManager network_account_info_manager;
        HFSAT::DataInfo data_info = network_account_info_manager.GetSrcDataInfo(HFSAT::kExchSourceBSE, "dummy_0");
        multicast_sender_socket_ = new HFSAT::MulticastSenderSocket(
            data_info.bcast_ip_, data_info.bcast_port_, HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                                            HFSAT::kExchSourceBSE, HFSAT::k_MktDataLive));

        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEvetnsForDataMode;
      } break;

      case HFSAT::kComShm: {
        generic_logger_ = &(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance());
        std::cout << "SHOULD NOT REACH HERE : " << std::endl;
        generic_mds_message_ = new HFSAT::MDS_MSG::GenericMDSMessage();
        generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::BSE;
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEventsForComShmMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
        is_logging_enabled_ = generic_logger_->IsLoggingEnabled();

      } break;

      case HFSAT::kProShm: {
        bse_mds_message_ = new EOBI_MDS::EOBICommonStruct();
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEventsForProShmMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        bse_mds_shm_interface_ = &HFSAT::Utils::BSEMDSShmInterface::GetUniqueInstance();
      } break;

      case HFSAT::kRaw: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "INITIATING THE ProcessEventsForRawMode " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        bse_mds_message_ = new EOBI_MDS::EOBICommonStruct();
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEventsForRawMode;

        if (nullptr == market_event_listener) {
          DBGLOG_CLASS_FUNC_LINE_FATAL << "MARKET LISTENER NULL FOR RAW MODE " << DBGLOG_ENDL_DUMP_AND_EXIT;
        }

      } break;

      case HFSAT::kConvertLogger: {
        bse_mds_message_ = new EOBI_MDS::EOBICommonStruct();
        process_event_for_modes_ = &BSETBTDataProcessor::ProcessEvetnsForConvertLoggerMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        bse_mds_shm_interface_ = &HFSAT::Utils::BSEMDSShmInterface::GetUniqueInstance();

        generic_logger_ = &(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance()); 
        generic_mds_message_ = new HFSAT::MDS_MSG::GenericMDSMessage();
        generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::BSE;
        generic_logger_->EnableLogging(); 
        is_logging_enabled_ = true;  
        generic_logger_->RunLoggerThread(); 

        std::cout << "GENERIC LOGGING : " << generic_logger_->IsLoggingEnabled() << std::endl;

      } break;

      default: {
        process_event_for_modes_ = NULL;

        DBGLOG_CLASS_FUNC_LINE_FATAL << "OOPS !!, THIS MODE IS NOT YET SUPPORTED BY THE PROCESSOR, MODE VALUE : "
                                     << (int32_t)daemon_mode << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        exit(-1);

      } break;
    }
  }

  ~BSETBTDataProcessor() {}

  BSETBTDataProcessor(BSETBTDataProcessor const& disabled_copy_constructor);

 public:
//
  uint32_t o_seq_num_;
  int64_t o_security_id_;
  uint32_t o_action_;
  uint8_t o_side_;
  int64_t o_price_;
  int32_t o_size_;
  uint64_t o_trd_reg_ts_;
  uint64_t o_priority_ts_;
  int64_t o_prev_price_;
  int32_t o_prev_size_;
  uint64_t o_prev_priority_ts_;
  bool o_intermediate_;
  int16_t o_synthetic_match_;
  int32_t current_mkt_seg_id_;        // Current market segment id extracted from the packet header
  bool initial_recovery_not_finished_;
  timeval logger_start_time_;

  std::unordered_map<int64_t, int32_t> sec_to_mkt_seg_id_;
  std::unordered_map<int32_t, int64_t> curr_sec_id_;
  std::unordered_map<int64_t, bool> instr_summry_rcvd_;
  std::map<int64_t, bool> security_in_recovery;
  std::unordered_map<int32_t, uint32_t>  mkt_seg_id_to_seq_num_;  // Mapping between market segment id and the latest message sequence number
  std::unordered_map<int64_t, uint32_t> target_msg_seq_num_;  // Mapping between security id and the message sequence number of instrument summary message
  HFSAT::Utils::BSEDailyTokenSymbolHandler& bse_daily_token_symbol_handler_;
  std::vector<std::pair<EOBI_MDS::EOBICommonStruct*, int64_t> > message_buffer_;
  std::vector<std::pair<uint64_t, uint64_t> > price_buffer_;
  HFSAT::Utils::CombinedSourceGenericLogger* generic_logger_;

//
//
  static BSETBTDataProcessor& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                                HFSAT::FastMdConsumerMode_t const& daemon_mode,
                                                HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                                                MarketEventListener* market_event_listener = nullptr) {
    static BSETBTDataProcessor unique_instance(dbglogger, daemon_mode, p_ccm_livesource, market_event_listener);
    return unique_instance;
  }

  void SetBardataPeriod(uint32_t bardata_period) {
    bardata_period_ = bardata_period;
  }
  
  bool IsProcessingSecurityForComShm(int32_t& token) {
    if (false == is_logging_enabled_) {
      if (tokens_requested_.find(token) == tokens_requested_.end()) return false;
    }
    return true;
  }

//It stores the current market data that we got during recovery.
//It also checks whether the thresholdrecovery time(1 min) w.r.t. current time and 
//if recovery is being carried out for more than one min then it stops the recovery
  bool CheckSequenceNum(EOBI_MDS::EOBICommonStruct* cstr) {
    EOBI_MDS::EOBICommonStruct* eobi_cstr_ = new EOBI_MDS::EOBICommonStruct(); 
    memcpy(eobi_cstr_, cstr, sizeof(EOBI_MDS::EOBICommonStruct));
  
    message_buffer_.push_back(std::make_pair(eobi_cstr_, o_security_id_));
    price_buffer_.push_back(std::make_pair(o_price_, o_prev_price_));
  
    timeval current_time_;
    gettimeofday(&current_time_, NULL);
  
    if (current_time_.tv_sec - logger_start_time_.tv_sec > 60) {
      for (auto it_ = security_in_recovery.begin(); it_ != security_in_recovery.end(); it_++) {
        //if (it_->second == true) { this will not process the message buffer of recoverd security
          EndRecovery(it_->first);
        //}
      }
      return true;
    }
    return false;
  }


  EOBI_MDS::EOBICommonStruct* GetBSECommonStructFromGenericPacket() {
    return &((generic_mds_message_->generic_data_).bse_data_);
  }

  void OnMarketEvent(EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct) {
    CALL_MEMBER_FN (*this, process_event_for_modes_)(bse_tbt_data_common_struct);
  }
};
}
}
