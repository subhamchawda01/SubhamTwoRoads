// =====================================================================================
//
//       Filename:  nse_tbt_data_processor.hpp
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
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_livesource.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/Utils/nse_mds_shm_interface.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/Utils/lock.hpp"
#include "infracore/NSEMD/nse_tbt_templates.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/Listeners/live_products_manager_listener.hpp"
#include "infracore/Tools/live_products_manager.hpp"
#include "dvccode/Utils/data_daemon_config.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

#define MAX_BUFFERED_EVENTS_STORAGE 8192

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

namespace HFSAT {
namespace NSEMD {

class MarketEventListener {
 public:
  ~MarketEventListener() {}
  virtual void OnMarketEventDispatch(NSE_MDS::NSETBTDataCommonStructProShm* market_event) {
    std::cout << "SHOULDN'T POINT HERE: " << std::endl;
    std::exit(-1);
  }
};

class NSESegmentWiseHistoryManager {
 private:
  HFSAT::DebugLogger& dbglogger_;
  //  std::unordered_set<int32_t>& processing_tokens_;

  // For RecoveryMode
  std::map<int32_t, std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>*> token_to_live_orders_map_;
  std::map<int32_t, HFSAT::Lock*> token_to_control_lock_;

  NSESegmentWiseHistoryManager();

 public:
  NSESegmentWiseHistoryManager(HFSAT::DebugLogger& dbglogger)
      : dbglogger_(dbglogger), token_to_live_orders_map_(), token_to_control_lock_() {}

  bool IsTokenAvailableForRecovery(int32_t token) {
    return token_to_live_orders_map_.end() == token_to_live_orders_map_.find(token) ? false : true;
  }

  void UpdateProcessedLiveOrdersMap(int32_t token, NSE_MDS::NSETBTDataCommonStruct const& bufferd_event) {
    if (token_to_control_lock_.end() == token_to_control_lock_.find(token)) {
      token_to_control_lock_[token] = new HFSAT::Lock();
    }

    // Lock token control
    token_to_control_lock_[token]->LockMutex();

    std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>* live_orders_map = NULL;

    if (token_to_live_orders_map_.find(token) == token_to_live_orders_map_.end()) {
      std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>* new_live_orders_map =
          new std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct>();
      token_to_live_orders_map_[token] = new_live_orders_map;
    }

    live_orders_map = token_to_live_orders_map_[token];

    if (ORDER_MESSAGE_TYPE_NEW == bufferd_event.activity_type ||
        ORDER_MESSAGE_TYPE_MODIFY == bufferd_event.activity_type ||
        SPREAD_ORDER_MESSAGE_TYPE_NEW == bufferd_event.activity_type ||
        SPREAD_ORDER_MESSAGE_TYPE_MODIFY == bufferd_event.activity_type) {
      (*live_orders_map)[bufferd_event.data.nse_order.order_id] = bufferd_event;

    } else if (ORDER_MESSAGE_TYPE_DELETE == bufferd_event.activity_type) {
      if (live_orders_map->end() != live_orders_map->find(bufferd_event.data.nse_order.order_id)) {
        live_orders_map->erase(live_orders_map->find(bufferd_event.data.nse_order.order_id));
      }
    } else {  // Trade

      if (live_orders_map->end() != live_orders_map->find(bufferd_event.data.nse_trade.buy_order_id)) {
        NSE_MDS::NSETBTDataCommonStruct& this_event = (*live_orders_map)[bufferd_event.data.nse_trade.buy_order_id];
        this_event.data.nse_order.order_qty -= bufferd_event.data.nse_trade.trade_qty;

        if (this_event.data.nse_order.order_qty <= 0) {
          live_orders_map->erase(live_orders_map->find(bufferd_event.data.nse_trade.buy_order_id));
        }
      }

      if (live_orders_map->end() != live_orders_map->find(bufferd_event.data.nse_trade.sell_order_id)) {
        NSE_MDS::NSETBTDataCommonStruct& this_event = (*live_orders_map)[bufferd_event.data.nse_trade.sell_order_id];
        this_event.data.nse_order.order_qty -= bufferd_event.data.nse_trade.trade_qty;

        if (this_event.data.nse_order.order_qty <= 0) {
          live_orders_map->erase(live_orders_map->find(bufferd_event.data.nse_trade.sell_order_id));
        }
      }
    }

    // Time to release this token lock
    token_to_control_lock_[token]->UnlockMutex();
  }


  bool FetchProcessedLiveOrdersMap(int32_t token, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    // Acquire lock to copy events
    if (token_to_control_lock_.end() == token_to_control_lock_.find(token)) {
      token_to_control_lock_[token] = new HFSAT::Lock();
    }

    // Lock token control
    token_to_control_lock_[token]->LockMutex();
    // It's time to now construct buffer
    std::map<uint64_t, NSE_MDS::NSETBTDataCommonStruct> live_orders = *(token_to_live_orders_map_[token]);
    token_to_control_lock_[token]->UnlockMutex();

    DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING RECOVERY REQUEST FOR : " << token
                                << " LIVE ORDERS : " << live_orders.size()
                                << " LIVE SIZE : " << live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct)
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    if (live_orders.size() == 0) {
      DBGLOG_CLASS_FUNC_LINE_INFO << "Live Order Size 0 "<< DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }
    // Check whether the buffer storage can hold enough elements
    if ((int32_t)(4 + (live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
      DBGLOG_CLASS_FUNC_LINE_INFO << "Live Order Size " << live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct) 
          << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    int32_t data_size = live_orders.size() * sizeof(NSE_MDS::NSETBTDataCommonStruct);

    // Insert length to buffer
    memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

    DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    data_packet += 4;

    uint32_t max_seq = 0;

    double best_bid_price = 0;
    double best_ask_price = 5000000000;
    uint64_t best_bid_order_id = 0;
    uint64_t best_ask_order_id = 0;

    // Fill up the buffer with data
    for (auto& itr : live_orders) {
      max_seq = std::max(max_seq, (itr.second).msg_seq);
      memcpy((void*)data_packet, (void*)&(itr.second), sizeof(NSE_MDS::NSETBTDataCommonStruct));
      data_packet += sizeof(NSE_MDS::NSETBTDataCommonStruct);

      if (HFSAT::kTradeTypeBuy == itr.second.data.nse_order.buysell &&
          best_bid_price < itr.second.data.nse_order.order_price) {
        best_bid_price = itr.second.data.nse_order.order_price;
        best_bid_order_id = itr.second.data.nse_order.order_id;
      }

      if (HFSAT::kTradeTypeSell == itr.second.data.nse_order.buysell &&
          best_ask_price > itr.second.data.nse_order.order_price) {
        best_ask_price = itr.second.data.nse_order.order_price;
        best_ask_order_id = itr.second.data.nse_order.order_id;
      }

      if (0.0 != best_bid_price && 0.0 != best_ask_price && best_bid_price >= best_ask_price) {
        DBGLOG_CLASS_FUNC_LINE_INFO << " BOOK HAS CROSSED AT THIS POINT WITH BEST BID PROCE : " << best_bid_price
                                    << " ORDER ID : " << best_bid_order_id << " ASK PRICE :" << best_ask_price
                                    << " ORDER ID : " << best_ask_order_id << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      }
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "SENDING MAX SEQUENCE IN RECOVERY AS :" << max_seq << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    // Request was handled successfully
    return true;
  }
};

class NSETBTDataProcessor : public HFSAT::LiveProductsManagerListener {
 private:

  void ProcessEventsForLoggerMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    data_logger_thread_->log(*nse_tbt_data_common_struct);
  }

  void UpdateBardataStruct( std::string& shortcode , NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct ) {
    if (shortcode_to_bardata_map_.find(shortcode) == shortcode_to_bardata_map_.end()) {
      shortcode_to_bardata_map_[shortcode] = new NSE_MDS::NSEBarDataCommonStruct();
      NSE_MDS::NSEBarDataCommonStruct* temp_bardata_struct_ = shortcode_to_bardata_map_[shortcode];
      int bardata_time_ = nse_tbt_data_common_struct->source_time.tv_sec - (nse_tbt_data_common_struct->source_time.tv_sec % bardata_period_);
      int expiry_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode);

      temp_bardata_struct_->bardata_time = bardata_time_;
      strcpy(temp_bardata_struct_->shortcode, shortcode.c_str());
      //temp_bardata_struct_->shortcode = shortcode.c_str();
      temp_bardata_struct_->start_time = nse_tbt_data_common_struct->source_time.tv_sec;
      temp_bardata_struct_->close_time = nse_tbt_data_common_struct->source_time.tv_sec;
      temp_bardata_struct_->expiry = expiry_;
      if (nse_tbt_data_common_struct->segment_type == 'S') {
        temp_bardata_struct_->open_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
        temp_bardata_struct_->close_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
        temp_bardata_struct_->low_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
        temp_bardata_struct_->high_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
        temp_bardata_struct_->volume = 0;
      } else {
        temp_bardata_struct_->open_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        temp_bardata_struct_->close_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        temp_bardata_struct_->low_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        temp_bardata_struct_->high_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        temp_bardata_struct_->volume = nse_tbt_data_common_struct->data.nse_trade.trade_qty;
      }

      temp_bardata_struct_->trades = 1;
    } else {
      NSE_MDS::NSEBarDataCommonStruct* temp_bardata_struct_ = shortcode_to_bardata_map_[shortcode];
  
      temp_bardata_struct_->close_time = nse_tbt_data_common_struct->source_time.tv_sec;
      if (nse_tbt_data_common_struct->segment_type == 'S') {
        temp_bardata_struct_->close_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
        if (temp_bardata_struct_->low_price > (nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100))
          temp_bardata_struct_->low_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
        if (temp_bardata_struct_->high_price < (nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100))
          temp_bardata_struct_->high_price = nse_tbt_data_common_struct->data.nse_spot_index.IndexValue / (double)100;
      } else {
        temp_bardata_struct_->close_time = nse_tbt_data_common_struct->source_time.tv_sec;
        temp_bardata_struct_->close_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        if (temp_bardata_struct_->low_price > (nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100))
          temp_bardata_struct_->low_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        if (temp_bardata_struct_->high_price < (nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100))
          temp_bardata_struct_->high_price = nse_tbt_data_common_struct->data.nse_trade.trade_price / (double)100;
        temp_bardata_struct_->volume += nse_tbt_data_common_struct->data.nse_trade.trade_qty;
        temp_bardata_struct_->trades += 1;
      }
    } 
  }

  void ProcessEventsForBardataMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    std::string exchsymbol = HFSAT::NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(nse_tbt_data_common_struct->getContract());
    std::string shortcode = HFSAT::NSESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchsymbol);
    if ((exchsymbol == "INVALID") || (shortcode == "INVALID")) { return; }
    // for CM products
    //std::string shortcode(nse_tbt_data_common_struct->getContract());
    //std::cout << "ProcessEventsForBardataMode:shortcode: " << shortcode << std::endl;

    std::string shortcode_w = HFSAT::NSESecurityDefinitions::GetWeeklyShortCodeFromSymbol(exchsymbol);
    if (shortcode.compare(shortcode_w) != 0) {
      UpdateBardataStruct(shortcode_w, nse_tbt_data_common_struct);
    }
/*
    if ( is_nifty_last_week_ ) {
      if ( ((shortcode.find("_NIFTY_") != string::npos)) &&
           ((shortcode.find("_C0_") != string::npos) || (shortcode.find("_P0_") != string::npos)) && (shortcode.find("_W") == string::npos) ) {
        int expiry_date_tmp = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
        if (( expiry_date_tmp == fin_last_week_expiry_ ) || ( expiry_date_tmp == nifty_last_week_expiry_ ) || ( expiry_date_tmp == midcp_last_week_expiry_ )) {
           std::string shortcode_w = shortcode + "_W";  
           UpdateBardataStruct(shortcode_w, nse_tbt_data_common_struct);
        } 
      }
    } 
    if ( is_banknifty_last_week_ ) {
      if ( ( (shortcode.find("_BANKNIFTY_") != string::npos)) &&
           ((shortcode.find("_C0_") != string::npos) || (shortcode.find("_P0_") != string::npos)) && (shortcode.find("_W") == string::npos) ) {
        int expiry_date_tmp = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
        if (( expiry_date_tmp == banknifty_last_week_expiry_ )) {
           std::string shortcode_w = shortcode + "_W";  
           UpdateBardataStruct(shortcode_w, nse_tbt_data_common_struct);
        } 
      }
    } 
    if ( is_fin_last_week_ ) { 
      if ( (shortcode.find("_FINNIFTY_") != string::npos) && 
           ((shortcode.find("_C0_") != string::npos) || (shortcode.find("_P0_") != string::npos)) && (shortcode.find("_W") == string::npos)) {
        int expiry_date_tmp = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
        if (( expiry_date_tmp == fin_last_week_expiry_ ) || ( expiry_date_tmp == nifty_last_week_expiry_ ) || ( expiry_date_tmp == midcp_last_week_expiry_ )) {
           std::string shortcode_w = shortcode + "_W";  
           UpdateBardataStruct(shortcode_w, nse_tbt_data_common_struct);
        } 
      }
    } 
    if ( is_midcp_last_week_ ) { 
      if ( (shortcode.find("_MIDCPNIFTY_") != string::npos) && 
           ((shortcode.find("_C0_") != string::npos) || (shortcode.find("_P0_") != string::npos)) && (shortcode.find("_W") == string::npos)) {
        int expiry_date_tmp = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode);
        if (( expiry_date_tmp == fin_last_week_expiry_ ) || ( expiry_date_tmp == nifty_last_week_expiry_ ) || ( expiry_date_tmp == midcp_last_week_expiry_ )) {
           std::string shortcode_w = shortcode + "_W";  
           UpdateBardataStruct(shortcode_w, nse_tbt_data_common_struct);
        } 
      }
    } 
*/      
    UpdateBardataStruct(shortcode, nse_tbt_data_common_struct);
  }

  void ProcessEventsForComShmMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    if (tokens_requested_.find(nse_tbt_data_common_struct->token) != tokens_requested_.end()) {
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_, false);
    }
    if (true == is_logging_enabled_) {
      generic_logger_->Log(*generic_mds_message_);
    }
  }

  void ProcessEventsForProShmMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    nse_mds_message_->source_time = nse_tbt_data_common_struct->source_time;
    nse_mds_message_->token = nse_tbt_data_common_struct->token;
    nse_mds_message_->msg_seq = nse_tbt_data_common_struct->msg_seq;

    switch (nse_tbt_data_common_struct->msg_type) {
      case NSE_MDS::MsgType::kInvalid: {
        nse_mds_message_->msg_type = kNSEMsgTypeInvalid;
      } break;
      case NSE_MDS::MsgType::kNSEOrderDelta: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSEOrderDelta;
        memcpy(&((nse_mds_message_->data).nse_order), &((nse_tbt_data_common_struct->data).nse_order),
               sizeof(NSE_MDS::NSETBTBookDelta));
      } break;
      case NSE_MDS::MsgType::kNSETrade: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSETrade;
        memcpy(&((nse_mds_message_->data).nse_trade), &((nse_tbt_data_common_struct->data).nse_trade),
               sizeof(NSE_MDS::NSETBTTrade));
      } break;
      case NSE_MDS::MsgType::kNSETradeDelta: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSETradeDelta;
      } break;
      case NSE_MDS::MsgType::kNSEOrderSpreadDelta: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSEOrderSpreadDelta;
      } break;
      case NSE_MDS::MsgType::kNSESpreadTrade: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSESpreadTrade;
      } break;
      case NSE_MDS::MsgType::kNSETradeExecutionRange: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSETradeExecutionRange;
        memcpy(&((nse_mds_message_->data).nse_trade_range), &((nse_tbt_data_common_struct->data).nse_trade_range),
               sizeof(NSE_MDS::NSETBTTradeExecRange));
      } break;
      case NSE_MDS::MsgType::kNSESpotIndexUpdate: {
        nse_mds_message_->msg_type = kNSEMsgTypekNSESpotIndexUpdate;
        memcpy(&((nse_mds_message_->data).nse_spot_index), &((nse_tbt_data_common_struct->data).nse_spot_index),
               sizeof(NSE_MDS::NSETBTSpotIndex));
      } break;
       case NSE_MDS::MsgType::kNSEOpenInterestTick: {
         nse_mds_message_->msg_type = KNSEMsgTypekNSEOpenInterestTick;
        memcpy(&((nse_mds_message_->data).nse_oi_data), &((nse_tbt_data_common_struct->data).nse_oi_data),
               sizeof(NSE_MDS::NSETBTOpenInterestTick));
      } break;
      default: { nse_mds_message_->msg_type = kNSEMsgTypeInvalid; } break;
    }

    nse_mds_message_->activity_type = nse_tbt_data_common_struct->activity_type;
    nse_mds_message_->segment_type = nse_tbt_data_common_struct->segment_type;

    nse_mds_shm_interface_->WriteGenericStruct(nse_mds_message_);
  }

  void ProcessEventsForRawMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    // Below check is removed as it already done in /infracore/NSEMD/nse_tbt_data_processor.hpp nse_tbt_raw_data_decoder.hpp
    // if (tokens_requested_raw_mode_.find(nse_tbt_data_common_struct->token) != tokens_requested_raw_mode_.end()) {
    //   std::cout << "ProcessEventsForRawMode TOKEN FOUND" << std::endl;
      nse_mds_message_->source_time = nse_tbt_data_common_struct->source_time;
      nse_mds_message_->token = nse_tbt_data_common_struct->token;
      nse_mds_message_->msg_seq = nse_tbt_data_common_struct->msg_seq;
      switch (nse_tbt_data_common_struct->msg_type) {
        case NSE_MDS::MsgType::kInvalid: {
          nse_mds_message_->msg_type = kNSEMsgTypeInvalid;
        } break;
        case NSE_MDS::MsgType::kNSEOrderDelta: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSEOrderDelta;
          memcpy(&((nse_mds_message_->data).nse_order), &((nse_tbt_data_common_struct->data).nse_order),
                 sizeof(NSE_MDS::NSETBTBookDelta));
        } break;
        case NSE_MDS::MsgType::kNSETrade: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSETrade;
          memcpy(&((nse_mds_message_->data).nse_trade), &((nse_tbt_data_common_struct->data).nse_trade),
                 sizeof(NSE_MDS::NSETBTTrade));
        } break;
        case NSE_MDS::MsgType::kNSETradeDelta: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSETradeDelta;
        } break;
        case NSE_MDS::MsgType::kNSEOrderSpreadDelta: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSEOrderSpreadDelta;
        } break;
        case NSE_MDS::MsgType::kNSESpreadTrade: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSESpreadTrade;
        } break;
        case NSE_MDS::MsgType::kNSETradeExecutionRange: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSETradeExecutionRange;
          memcpy(&((nse_mds_message_->data).nse_trade_range), &((nse_tbt_data_common_struct->data).nse_trade_range),
                 sizeof(NSE_MDS::NSETBTTradeExecRange));
        } break;
        case NSE_MDS::MsgType::kNSESpotIndexUpdate: {
          nse_mds_message_->msg_type = kNSEMsgTypekNSESpotIndexUpdate;
          memcpy(&((nse_mds_message_->data).nse_spot_index), &((nse_tbt_data_common_struct->data).nse_spot_index),
                 sizeof(NSE_MDS::NSETBTSpotIndex));
        } break;
        case NSE_MDS::MsgType::kNSEOpenInterestTick: {
    	//   dbglogger_ << "kNSEOpenInterestTick "<< nse_tbt_data_common_struct->token << " " << (nse_tbt_data_common_struct->data).nse_oi_data.OpenInterest << "\n";
           nse_mds_message_->msg_type = KNSEMsgTypekNSEOpenInterestTick;
          memcpy(&((nse_mds_message_->data).nse_oi_data), &((nse_tbt_data_common_struct->data).nse_oi_data),
                 sizeof(NSE_MDS::NSETBTOpenInterestTick));
        } break;
        default: { nse_mds_message_->msg_type = kNSEMsgTypeInvalid; } break;
      }

      nse_mds_message_->activity_type = nse_tbt_data_common_struct->activity_type;
      nse_mds_message_->segment_type = nse_tbt_data_common_struct->segment_type;

      market_event_listener_->OnMarketEventDispatch(nse_mds_message_);
    
  }

  void ProcessEvetnsForConvertLoggerMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    ProcessEventsForProShmMode(nse_tbt_data_common_struct);
    // Log it

    memcpy((void*)(&(generic_mds_message_->generic_data_).nse_data_), (void*)nse_tbt_data_common_struct,
           sizeof(NSE_MDS::NSETBTDataCommonStruct));
    generic_logger_->Log(*generic_mds_message_);
  }

  void ProcessEvetnsForDataMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    multicast_sender_socket_->WriteN(sizeof(NSE_MDS::NSETBTDataCommonStruct), (void*)nse_tbt_data_common_struct);
  }

 public:
  void DumpBardata() {
    bardata_logger_thread_->logBardata(shortcode_to_bardata_map_);
    DBGLOG_CLASS_FUNC_LINE_INFO << "Bardata_map_size : " << shortcode_to_bardata_map_.size() << DBGLOG_ENDL_NOFLUSH;
    shortcode_to_bardata_map_.clear();
  }

  void UpdateProcessedBardataMap() {
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

      // Time to release this shortcode lock
//      shortcode_to_control_lock_[map_itr.first]->UnlockMutex();
    }
  }

  void AddBardataToRecoveryMap() {
    UpdateProcessedBardataMap();
    DBGLOG_CLASS_FUNC_LINE_INFO << "Bardata_map_size : " << shortcode_to_bardata_map_.size() << DBGLOG_ENDL_NOFLUSH;
    shortcode_to_bardata_map_.clear();
  }

  bool IsTokenAvailableForRecovery(int32_t token, char segment) {
    return (segment_to_history_manager_[segment]->IsTokenAvailableForRecovery(token));
  }

  bool SendEmptyBardataUpdate(std::string& shortcode_, int32_t const& start_hhmm_, int32_t const& end_hhmm_, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    NSE_MDS::NSEBarDataCommonStruct temp_bardata_struct_;
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
                                << " LIVE SIZE : " << sizeof(NSE_MDS::NSEBarDataCommonStruct)
                                << " START_TIME: " << start_hhmm_ 
                                << " END_TIME: " << end_hhmm_
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    // Check whether the buffer storage can hold enough elements
    if ((int32_t)(4 + (sizeof(NSE_MDS::NSEBarDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
      DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size " << sizeof(NSE_MDS::NSEBarDataCommonStruct) 
        << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    int32_t data_size = sizeof(NSE_MDS::NSEBarDataCommonStruct);
  
    // Insert length to buffer
    memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

    DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    data_packet += 4;

    // Fill up the buffer with data
    memcpy((void*)data_packet, (void*)&(temp_bardata_struct_), sizeof(NSE_MDS::NSEBarDataCommonStruct));
    data_packet += sizeof(NSE_MDS::NSEBarDataCommonStruct);

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
    DBGLOG_CLASS_FUNC_LINE_INFO << "shortcode_:" << shortcode_ << " start_hhmm_: " << start_hhmm_ << " end_hhmm_: " << end_hhmm_
                                << " shortcode_to_live_bardata_map_[shortcode_].size(): " << shortcode_to_live_bardata_map_[shortcode_].size()
			        << DBGLOG_ENDL_NOFLUSH;	
    // Lock token control
//    shortcode_to_control_lock_[shortcode_]->LockMutex();
    if (shortcode_to_live_bardata_map_.end() != shortcode_to_live_bardata_map_.find(shortcode_)) {
      // It's time to now construct buffer
      std::map<int32_t, NSE_MDS::NSEBarDataCommonStruct*> live_bardata = shortcode_to_live_bardata_map_[shortcode_];
  
      int bardata_count = 0;
      for (auto& itr : live_bardata) {
        int32_t bardata_time = (itr.second)->bardata_time;
        if ((bardata_time >= start_hhmm_) && (bardata_time <= end_hhmm_)) {
          ++bardata_count;
        }
      }

      if ((live_bardata.size() == 0) || (bardata_count == 0)) {
        return SendEmptyBardataUpdate(shortcode_, start_hhmm_, end_hhmm_, data_packet, MAX_RECOVERY_PACKET_SIZE);
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING BARDATA RECOVERY REQUEST FOR : " << shortcode_
                                  << " LIVE BARDATA : " << live_bardata.size()
                                  << " LIVE SIZE : " << live_bardata.size() * sizeof(NSE_MDS::NSEBarDataCommonStruct)
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
      if ((int32_t)(4 + (live_bardata.size() * sizeof(NSE_MDS::NSEBarDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
        DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size " << live_bardata.size() * sizeof(NSE_MDS::NSEBarDataCommonStruct) 
          << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        return false;
      }

      int32_t data_size = bardata_count * sizeof(NSE_MDS::NSEBarDataCommonStruct);
  
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
          memcpy((void*)data_packet, (void*)&(*(itr.second)), sizeof(NSE_MDS::NSEBarDataCommonStruct));
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

  bool FetchProcessedLiveOrdersMap(int32_t token, char* data_packet, int32_t const& MAX_BUFFER_SIZE, char segment) {
    return (segment_to_history_manager_[segment]->FetchProcessedLiveOrdersMap(token, data_packet, MAX_BUFFER_SIZE));
  }

  void AddShortCodeForProcessing(std::string shortcode) {
    std::string exchange_symbol = GetExchangeSymbol(shortcode);
    std::string internal_symbol = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
    if (internal_symbol == std::string("INVALID")) {
      std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
      return;
    }

    char segment = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
    int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

    if (token != -1 /*Dummy Token*/) {
      tokens_requested_raw_mode_.insert(token);
    } else {
      std::cerr << " Invalid Token for shortcode " << shortcode << std::endl;
    }
  }

 private:
#define NSE_SOURCES_PATH "/spare/local/files/NSE/nse_recovery_datasources.txt"

  void ApplyAddTSFilterForProcessingData() {
    ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())
        .LoadNSESecurityDefinitions();

    // Load Addts File and Only add tokens for those products for recovery management

    std::ifstream addtsfs;
    addtsfs.open(NSE_SOURCES_PATH);

    if (false == addtsfs.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "UNABLE TO OPEN NSE RECOVERY SOURCES FILE : " << NSE_SOURCES_PATH
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return;
    }

    char buffer[1024];
    while (addtsfs.good()) {
      addtsfs.getline(buffer, 1024);
      std::string line = buffer;

      if (line.length() == 0) continue;
      if (std::string::npos != line.find("#")) continue;

      std::string shortcode = line;

      if (false ==
          HFSAT::SecurityDefinitions::CheckIfContractSpecExists(shortcode, HFSAT::DateTime::GetCurrentIsoDateLocal()))
        continue;

      std::string exchange_symbol = GetExchangeSymbol(shortcode);
      std::string internal_symbol = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);

      if (internal_symbol == std::string("INVALID")) {
        std::cerr << "ApplyAddTSFilterForProcessingData Error: Invalid Internal Symbol for " << exchange_symbol
                  << std::endl;
        continue;
      }

      char segment = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
      int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

      if (-1 == token) {
        std::cerr << "ApplyAddTSFilterForProcessingData Error: Invalid Token : " << token << " For Symbol "
                  << exchange_symbol << std::endl;
        continue;
      }

      processing_tokens_.insert(token);
    }

    addtsfs.close();
  }

  void ProcessEventsForRecoveryHostMode(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "New Packet arrived..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;
    static bool are_we_processing_all = (0 == processing_tokens_.size());

    if (false == are_we_processing_all) {
      if (processing_tokens_.end() == processing_tokens_.find(nse_tbt_data_common_struct->token)) return;
    }

    if (nse_tbt_data_common_struct->msg_type == NSE_MDS::MsgType::kNSETradeExecutionRange) return;

    // Only process delta events, trades don't need to be re-sent / recovered
    if (ORDER_MESSAGE_TYPE_NEW != nse_tbt_data_common_struct->activity_type &&
        ORDER_MESSAGE_TYPE_MODIFY != nse_tbt_data_common_struct->activity_type &&
        ORDER_MESSAGE_TYPE_DELETE != nse_tbt_data_common_struct->activity_type &&
        TRADE_MESSAGE_TYPE_TRADE != nse_tbt_data_common_struct->activity_type)
      return;

    segment_to_history_manager_[nse_tbt_data_common_struct->segment_type]->UpdateProcessedLiveOrdersMap(
        nse_tbt_data_common_struct->token, *nse_tbt_data_common_struct);
  }

  void OnLiveProductChange(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                           ExchSource_t exchange) {
    std::cout << "ReqCW: NSETBTHandler: " << action << " " << shortcode << std::endl;
    if ((daemon_mode_ == HFSAT::kComShm || daemon_mode_ == HFSAT::kProShm || daemon_mode_ == HFSAT::kConvertLogger) &&
        (exchange == kExchSourceNSE || exchange == kExchSourceNSE_FO || exchange == kExchSourceNSE_CD ||
         exchange == kExchSourceNSE_EQ)) {
      std::string exchange_symbol = GetExchangeSymbol(shortcode);
      std::string internal_symbol = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
      if (internal_symbol == std::string("INVALID")) {
        std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
        return;
      }
      char segment = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shortcode);
      int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);
      if (token != -1 /*Dummy Token*/) {
        if (action == LiveProductsChangeAction::kAdd) {
          std::cout << "Token " << token << " added for " << shortcode << std::endl;
          tokens_requested_.insert(token);
        } else if (action == LiveProductsChangeAction::kRemove) {
          if (tokens_requested_.find(token) != tokens_requested_.end()) {
            std::cout << "Token " << token << " erased for " << shortcode << std::endl;
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
      std::cerr << " ReqCW: NSETBTDataProcessor: Error in ExchangeSymbol Conversion for Shortcode : " << shortcode
                << std::endl;
    }

    return exchange_symbol;
  }

 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::FastMdConsumerMode_t daemon_mode_;
  std::unordered_set<int32_t> processing_tokens_;

  // For Logger Mode//
  MDSLogger<NSE_MDS::NSETBTDataCommonStruct>* data_logger_thread_;
  MDSLogger<NSE_MDS::NSEBarDataCommonStruct>* bardata_logger_thread_;

  // For ComShm Mode
  HFSAT::Utils::MDSShmInterface* mds_shm_interface_;
  HFSAT::Utils::NSEMDSShmInterface* nse_mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage* generic_mds_message_;
  NSE_MDS::NSETBTDataCommonStructProShm* nse_mds_message_;
  MarketEventListener* market_event_listener_;
  bool is_logging_enabled_;

  // For DataMode
  HFSAT::MulticastSenderSocket* multicast_sender_socket_;

  // For RecoveryMode
  std::map<char, NSESegmentWiseHistoryManager*> segment_to_history_manager_;
  bool is_hybrid_exch_;

  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_;

  NSE_MDS::NSETBTDataCommonStruct dummy_struct;

  typedef void (NSETBTDataProcessor::*ProcessEventForModes)(
      NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct);
  ProcessEventForModes process_event_for_modes_;

  std::unordered_set<int32_t> tokens_requested_;
  std::unordered_set<int32_t> tokens_requested_raw_mode_;

  HFSAT::Utils::CombinedSourceGenericLogger* generic_logger_;

  std::unordered_map<std::string,NSE_MDS::NSEBarDataCommonStruct*> shortcode_to_bardata_map_; 

  std::unordered_map<std::string, std::map<int32_t, NSE_MDS::NSEBarDataCommonStruct*>> shortcode_to_live_bardata_map_;

  std::unordered_map<std::string, HFSAT::Lock*> shortcode_to_control_lock_;

  bool is_local_data_;

  uint32_t bardata_period_;

  int nifty_last_week_expiry_;

  int banknifty_last_week_expiry_;

  int fin_last_week_expiry_;

  int midcp_last_week_expiry_;

  bool is_nifty_last_week_;

  bool is_banknifty_last_week_;

  bool is_fin_last_week_;

  bool is_midcp_last_week_;

  NSETBTDataProcessor(HFSAT::DebugLogger& dbglogger, HFSAT::FastMdConsumerMode_t const& daemon_mode,
                      HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                      MarketEventListener* market_event_listener = nullptr, bool is_hybrid_exch = false)
      : dbglogger_(dbglogger),
        daemon_mode_(daemon_mode),
        processing_tokens_(),
        data_logger_thread_(NULL),
        bardata_logger_thread_(NULL),
        mds_shm_interface_(nullptr),
        nse_mds_shm_interface_(nullptr),
        generic_mds_message_(NULL),
        nse_mds_message_(NULL),
        market_event_listener_(market_event_listener),
        is_logging_enabled_(true),
        multicast_sender_socket_(NULL),
        segment_to_history_manager_(),
        is_hybrid_exch_(is_hybrid_exch),
        nse_daily_token_symbol_handler_(
            HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
        generic_logger_(nullptr),
        shortcode_to_bardata_map_(),
        shortcode_to_live_bardata_map_(), 
        shortcode_to_control_lock_(),
        bardata_period_(60),
        nifty_last_week_expiry_(-1),
        fin_last_week_expiry_(-1),
        midcp_last_week_expiry_(-1),
        is_nifty_last_week_(false),
        is_banknifty_last_week_(false),
        is_fin_last_week_(false),
        is_midcp_last_week_(false) {
    is_local_data_ = TradingLocationUtils::GetTradingLocationExch(kExchSourceNSE) ==
                     TradingLocationUtils::GetTradingLocationFromHostname();

    std::string nifty_mnt_shortcode = "NSE_NIFTY_C0_A";
    std::string nifty_weekly_shortcode = "NSE_NIFTY_C0_A_W";
    std::string banknifty_mnt_shortcode = "NSE_BANKNIFTY_C0_A";
    std::string banknifty_weekly_shortcode = "NSE_BANKNIFTY_C0_A_W";
    std::string fin_mnt_shortcode = "NSE_FINNIFTY_C0_A";
    std::string fin_weekly_shortcode = "NSE_FINNIFTY_C0_A_W";
    std::string midcp_mnt_shortcode = "NSE_MIDCPNIFTY_C0_A";
    std::string midcp_weekly_shortcode = "NSE_MIDCPNIFTY_C0_A_W";

    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal()).LoadNSESecurityDefinitions();
    if ( HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(nifty_mnt_shortcode) == HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(nifty_weekly_shortcode) ) {
      nifty_last_week_expiry_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(nifty_mnt_shortcode);
      is_nifty_last_week_ = true;
    }
    if ( HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(banknifty_mnt_shortcode) == HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(banknifty_weekly_shortcode) ) {
      banknifty_last_week_expiry_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(banknifty_mnt_shortcode);
      is_banknifty_last_week_ = true;
    }
    if ( HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(fin_mnt_shortcode) == HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(fin_weekly_shortcode) ) {
      fin_last_week_expiry_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(fin_mnt_shortcode);
      is_fin_last_week_ = true;
    }
    if ( HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(midcp_mnt_shortcode) == HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(midcp_weekly_shortcode) ) {
      midcp_last_week_expiry_ = HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(midcp_mnt_shortcode);
      is_midcp_last_week_ = true;
    }
    std::cout<<"last week bools N: BN: FN: MN:" <<is_nifty_last_week_<<" "<<is_banknifty_last_week_<<" "<<is_fin_last_week_<<" "<<is_midcp_last_week_<<std::endl;
    
    switch (daemon_mode_) {
      case HFSAT::kLogger: {
        data_logger_thread_ = new MDSLogger<NSE_MDS::NSETBTDataCommonStruct>("NSETBT");
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForLoggerMode;
      } break;

      case HFSAT::kBardataLogger: {
        bardata_logger_thread_ = new MDSLogger<NSE_MDS::NSEBarDataCommonStruct>("BARDATA");
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForBardataMode;
        bardata_logger_thread_->EnableAffinity("NSEBardataLogger");
        bardata_logger_thread_->run();
      } break;

      case HFSAT::kBardataRecoveryHost: {
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForBardataMode;

      } break;

      case HFSAT::kMcast: {
        HFSAT::NetworkAccountInfoManager network_account_info_manager;
        HFSAT::DataInfo data_info = network_account_info_manager.GetSrcDataInfo(HFSAT::kExchSourceNSE, "dummy_0");
        multicast_sender_socket_ =
            new HFSAT::MulticastSenderSocket(data_info.bcast_ip_, data_info.bcast_port_,
                                             HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                                 HFSAT::kExchSourceNSE, HFSAT::k_MktDataLive));

        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEvetnsForDataMode;
      } break;

      case HFSAT::kComShm: {
        generic_logger_ = &(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance());
        std::cout << "SHOULD NOT REACH HERE : " << std::endl;
        generic_mds_message_ = new HFSAT::MDS_MSG::GenericMDSMessage();
        generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NSE;
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForComShmMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
        is_logging_enabled_ = generic_logger_->IsLoggingEnabled();

      } break;

      case HFSAT::kProShm: {
        nse_mds_message_ = new NSE_MDS::NSETBTDataCommonStructProShm();
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForProShmMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        nse_mds_shm_interface_ = &HFSAT::Utils::NSEMDSShmInterface::GetUniqueInstance();
      } break;

      case HFSAT::kRaw: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "INITIATING THE ProcessEventsForRawMode " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        nse_mds_message_ = new NSE_MDS::NSETBTDataCommonStructProShm();
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForRawMode;

        if (nullptr == market_event_listener) {
          DBGLOG_CLASS_FUNC_LINE_FATAL << "MARKET LISTENER NULL FOR RAW MODE " << DBGLOG_ENDL_DUMP_AND_EXIT;
        }

      } break;

      case HFSAT::kConvertLogger: {
        //        std::cout << "INITIALIZED FOR LOGGER MODE" << std::endl;
        nse_mds_message_ = new NSE_MDS::NSETBTDataCommonStructProShm();
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEvetnsForConvertLoggerMode;
        HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
        nse_mds_shm_interface_ = &HFSAT::Utils::NSEMDSShmInterface::GetUniqueInstance();

        generic_logger_ = &(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance());
        generic_mds_message_ = new HFSAT::MDS_MSG::GenericMDSMessage();
        generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NSE;
        generic_logger_->EnableLogging();
        is_logging_enabled_ = true;

        if(!is_hybrid_exch_){
          generic_logger_->RunLoggerThread();
        }

        std::cout << "GENERIC LOGGING : " << generic_logger_->IsLoggingEnabled() << std::endl;

      } break;

      case HFSAT::kRecoveryHost: {
        process_event_for_modes_ = &NSETBTDataProcessor::ProcessEventsForRecoveryHostMode;
        segment_to_history_manager_[NSE_EQ_SEGMENT_MARKING] = new NSESegmentWiseHistoryManager(dbglogger_);
        segment_to_history_manager_[NSE_CD_SEGMENT_MARKING] = new NSESegmentWiseHistoryManager(dbglogger_);
        segment_to_history_manager_[NSE_FO_SEGMENT_MARKING] = new NSESegmentWiseHistoryManager(dbglogger_);

        ApplyAddTSFilterForProcessingData();

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

  ~NSETBTDataProcessor() {}

  NSETBTDataProcessor(NSETBTDataProcessor const& disabled_copy_constructor);

 public:
  static NSETBTDataProcessor& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                                HFSAT::FastMdConsumerMode_t const& daemon_mode,
                                                HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                                                MarketEventListener* market_event_listener = nullptr, bool is_hybrid_exch = false) {
    static NSETBTDataProcessor unique_instance(dbglogger, daemon_mode, p_ccm_livesource, market_event_listener, is_hybrid_exch);
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

  NSE_MDS::NSETBTDataCommonStruct* GetNSECommonStructFromGenericPacket() {
    return &((generic_mds_message_->generic_data_).nse_data_);
  }

  inline void OnMarketEvent(NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct) {
    CALL_MEMBER_FN (*this, process_event_for_modes_)(nse_tbt_data_common_struct);
  }

};
}  // namespace NSEMD
}  // namespace HFSAT
