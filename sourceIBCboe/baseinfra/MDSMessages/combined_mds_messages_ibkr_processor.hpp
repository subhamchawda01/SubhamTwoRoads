// =====================================================================================
//
//       Filename:  combined_mds_messages_nse_source.cpp
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
#include "dvccode/Utils/cboe_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/cboe_refdata_loader.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/recovery_manager_config_parser.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"
#include "dvccode/IBUtils/contract_manager.hpp"

//#include "dvccode/Profiler/cpucycle_profiler.hpp"

namespace HFSAT {
namespace IBKR {

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

class CombinedMDSMessagesIBKRProcessor {
 protected:
  DebugLogger& dbglogger_;                       ///< error logger
  const SecurityNameIndexer& sec_name_indexer_;  ///< needed to filter securities of interest

  // Indication of whether the recovery has been complete or not and what was the last sequenced applied
  // can be switched to bool later
  int64_t security_id_to_recovery_complete_[DEF_MAX_SEC_ID];
  double security_id_to_inverse_price_mult_[DEF_MAX_SEC_ID];

  OrderGlobalListenerIBKR* p_order_global_listener_ibkr_;  ///< Listeners of NSE messages in LiveTrading of all types
  ExternalTimeListener* p_time_keeper_;

  // This classes are useful to access per security based refdata
  HFSAT::Utils::CBOEDailyTokenSymbolHandler& ibkr_daily_token_symbol_handler_;
  HFSAT::Utils::CBOERefDataLoader& ibkr_refdata_loader_;

  // This contains recovery packets received from RecoveryHost
  char* recovery_buffer;

  // It's goot to load and store this once as it's not going to be changed
  std::map<char, std::map<int32_t, CBOE_UDP_MDS::CBOERefData>*> segment_to_ibkr_ref_data_;
  std::map<char, std::unordered_map<std::string, int32_t> > segment_to_exchSymbol_secid_map_; //Here we use internal symbol instead of token unlike NSE
  TradeTimeManager& trade_time_manager_;

  // client side recovery purpose
  HFSAT::RecoveryManagerConfigParser recovery_manager_config_parser_;

 public:
  CombinedMDSMessagesIBKRProcessor(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        p_time_keeper_(NULL),
        ibkr_daily_token_symbol_handler_(HFSAT::Utils::CBOEDailyTokenSymbolHandler::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        ibkr_refdata_loader_(HFSAT::Utils::CBOERefDataLoader::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        recovery_buffer(new char[MAX_RECOVERY_PACKET_SIZE]),
        segment_to_ibkr_ref_data_(),
        segment_to_exchSymbol_secid_map_(),
        trade_time_manager_(TradeTimeManager::GetUniqueInstance(
            _sec_name_indexer_, HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        recovery_manager_config_parser_() {
    // Note, We are here taking advantage of the fact that -1 will be all FF... so we are not setting it per byte basis
    // but rather
    // total number of elements only

    // TODO - MULTISHM
    memset((void*)security_id_to_recovery_complete_, -1, DEF_MAX_SEC_ID*sizeof(int64_t));
    memset((void*)security_id_to_inverse_price_mult_, 0, DEF_MAX_SEC_ID*sizeof(int32_t));

    timeval tv;
    gettimeofday(&tv, NULL);
    int curr_hhmm = DateTime::GetUTCHHMMFromTime(tv.tv_sec);
    for (unsigned int sec_id = 0; sec_id < _sec_name_indexer_.NumSecurityId(); sec_id++) {
      // If current time is less than the start time for this sec_id, then don't go for its recovery
      int start_time = trade_time_manager_.GetStartTime(sec_id);
      int start_hhmm = (start_time / 3600) * 100 + (start_time % 3600) / 60;
      if (curr_hhmm < start_hhmm) {
        security_id_to_recovery_complete_[sec_id] = 0;
      }
    }

    if (IsItSimulationServer()) {
      memset((void*)security_id_to_recovery_complete_, 0, DEF_MAX_SEC_ID*sizeof(int64_t));
    }

    // Reset the buffer
    memset((void*)recovery_buffer, 0, MAX_RECOVERY_PACKET_SIZE);

    // Initialize
    segment_to_ibkr_ref_data_[CBOE_FO_SEGMENT_MARKING] = new std::map<int32_t, CBOE_UDP_MDS::CBOERefData>();
    segment_to_ibkr_ref_data_[CBOE_IX_SEGMENT_MARKING] = new std::map<int32_t, CBOE_UDP_MDS::CBOERefData>();
    segment_to_ibkr_ref_data_[CBOE_COMBO_SEGMENT_MARKING] = new std::map<int32_t, CBOE_UDP_MDS::CBOERefData>();


    std::map<int32_t, CBOE_UDP_MDS::CBOERefData>& fo_ibkr_ref_data = *(segment_to_ibkr_ref_data_[CBOE_FO_SEGMENT_MARKING]);
    std::map<int32_t, CBOE_UDP_MDS::CBOERefData>& ix_ibkr_ref_data = *(segment_to_ibkr_ref_data_[CBOE_IX_SEGMENT_MARKING]);
    std::map<int32_t, CBOE_UDP_MDS::CBOERefData>& co_ibkr_ref_data = *(segment_to_ibkr_ref_data_[CBOE_COMBO_SEGMENT_MARKING]);


    for (auto& itr : ibkr_refdata_loader_.GetCBOERefData(CBOE_FO_SEGMENT_MARKING)) {
      // std::cout<<itr.first<<" "<<std::string((itr.second).option_type)<<"\n";
      std::ostringstream internal_symbol_str;
      fo_ibkr_ref_data[itr.first] = itr.second;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "CBOE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << (itr.second).expiry;

      } else {
        internal_symbol_str << "CBOE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << (itr.second).expiry;
      }
      std::string exchange_symbol =
          CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

      // std::cout<<exchange_symbol<<"Internal Symbol"<<internal_symbol_str.str()<<"\n";

      int32_t security_id = -1;
      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        // std::cout<<"Security Id:"<<security_id<<"\n";
        segment_to_exchSymbol_secid_map_[CBOE_FO_SEGMENT_MARKING].insert(std::make_pair(exchange_symbol, security_id));
        if (security_id >= 0) {
          // security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier); //Sanskar:Do not know what will be the price_multiplier for each security??
        }
      }
    }

    for (auto& itr : ibkr_refdata_loader_.GetCBOERefData(CBOE_IX_SEGMENT_MARKING)) {
//      std::cout<<itr.first<<" "<<std::string((itr.second).option_type) << " " << (itr.second).symbol <<"\n";
      std::ostringstream internal_symbol_str;
      ix_ibkr_ref_data[itr.first] = itr.second;

      if (std::string("IX") == std::string((itr.second).option_type)) {
        internal_symbol_str << (itr.second).symbol;
      } else {
        internal_symbol_str << "CBOE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << (itr.second).expiry;
      }
      std::string exchange_symbol =
          CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

      // std::cout<<exchange_symbol<<"Internal Symbol"<<internal_symbol_str.str()<<"\n";

      int32_t security_id = -1;
      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
//        std::cout<<"Security Id:"<<security_id<<"\n";
        segment_to_exchSymbol_secid_map_[CBOE_IX_SEGMENT_MARKING].insert(std::make_pair(exchange_symbol, security_id));
        if (security_id >= 0) {
          // security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier); //Sanskar:Do not know what will be the price_multiplier for each security??
        }
      }
      // std::cout<<"CombinedMDSMessagesIBKRProcessor"<<exchange_symbol<<" "<<security_id<<"\n";
    }

    for (auto& itr : ibkr_refdata_loader_.GetCBOERefData(CBOE_COMBO_SEGMENT_MARKING)) {
    //  std::cout<<itr.first<<" "<<std::string((itr.second).option_type) << " " << (itr.second).symbol <<std::endl;
      std::ostringstream internal_symbol_str;
      co_ibkr_ref_data[itr.first] = itr.second;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "CBOE_SPXW_COMBO_" << (itr.second).symbol;
      } else {
        internal_symbol_str << "CBOE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << (itr.second).expiry;
      }
      std::string exchange_symbol =
          CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

      // std::cout<<exchange_symbol<<"Internal Symbol"<<internal_symbol_str.str()<<std::endl;

      int32_t security_id = -1;
      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
       std::cout<<"Security Id:"<<security_id<<"\n";
        segment_to_exchSymbol_secid_map_[CBOE_COMBO_SEGMENT_MARKING].insert(std::make_pair(exchange_symbol, security_id));
        if (security_id >= 0) {
          // security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier); //Sanskar:Do not know what will be the price_multiplier for each security??
        }
      }
      // std::cout<<"CombinedMDSMessagesIBKRProcessor"<<exchange_symbol<<" "<<security_id<<"\n";
    }
    /*HFSAT::CpucycleProfiler::SetUniqueInstance(4);
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "OnOrderAdd");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(2, "OnOrderModify");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(3, "OnOrderDelete");
    HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(4, "OnTrade");*/
  }

  ~CombinedMDSMessagesIBKRProcessor() {
    // std::cout << HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();
  }

  void ResetIBKRRecoveryIndex(){
      for (unsigned int sec_id = 0; sec_id < sec_name_indexer_.NumSecurityId(); sec_id++) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "DISABLING RECOVERY MANAGER FOR SecId: "<< sec_id << DBGLOG_ENDL_NOFLUSH;
        security_id_to_recovery_complete_[sec_id] = 0;
      }
  }

  inline void SetOrderGlobalListenerIBKR(OrderGlobalListenerIBKR* p_new_listener_) {
    p_order_global_listener_ibkr_ = p_new_listener_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  // Interface to recover live orders for the given security, Applies packets upto given sequence number
  /*bool RecoverLiveOrders(int32_t const& token, uint32_t const& current_seq_num, int32_t const& sec_id, char segment) {
    DBGLOG_CLASS_FUNC_LINE_INFO << "STARTING CONNECTION TO RECOVERY MANAGER..." << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    std::string this_exch_string = "NSE";

    switch (segment) {
      case NSE_FO_SEGMENT_MARKING: {
        this_exch_string = "NSE_FO";
      } break;
      case NSE_EQ_SEGMENT_MARKING: {
        this_exch_string = "NSE_EQ";
      } break;
      case NSE_CD_SEGMENT_MARKING: {
        this_exch_string = "NSE_CD";
      } break;
      default: { this_exch_string = "NSE"; } break;
    }

    if ("NSE" != this_exch_string) {
      recovery_manager_config_parser_.Initialise(this_exch_string);
    } else {
      recovery_manager_config_parser_.Initialise();
    }

    // hard coded the exchange src value
    int32_t int_exchange_src = HFSAT::exchange_name_str_to_int_mapping[this_exch_string];
    std::string recovery_ip = recovery_manager_config_parser_.GetRecoveryHostIP(int_exchange_src);

    // For NSE always recover from IND12 by default
    if (std::string("127.0.0.1") == recovery_ip) {
      recovery_ip = "10.23.115.62";  // IND12
    }

    int recovery_port = recovery_manager_config_parser_.GetRecoveryHostClientPort(int_exchange_src);

    DBGLOG_CLASS_FUNC_LINE_INFO << "TRYING TO CONNECT TO RECOVERY HOST AT " << recovery_ip << " X " << recovery_port
                                << "..." << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    if (segment_to_nse_ref_data_.end() == segment_to_nse_ref_data_.find(segment)) return false;

    std::map<int32_t, NSE_UDP_MDS::NSERefData>& nse_ref_data = *(segment_to_nse_ref_data_[segment]);

    HFSAT::TCPClientSocket tcp_client_socket;
    tcp_client_socket.Connect(recovery_ip, recovery_port);

    if (-1 == tcp_client_socket.socket_file_descriptor()) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO CONNECT TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      exit(-1);
    }

    char request_message[RECOVERY_REQUEST_MESSAGE_LENGTH];
    memset(request_message, '\0', RECOVERY_REQUEST_MESSAGE_LENGTH);

    char* tmp_writer_ptr = request_message;
    // fill-in the exchange name
    memcpy((void*)(tmp_writer_ptr), (void*)(&int_exchange_src), sizeof(int32_t));
    tmp_writer_ptr += RECOVERY_REQUEST_EXCHANGE_SRC_LENGTH;

    // fill-in the dummy flags (we aren't using these currently)
    tmp_writer_ptr += RECOVERY_REQUEST_DUMMY_FLAGS_LENGTH;

    // fill in the message body
    tmp_writer_ptr[0] = segment;
    memcpy((void*)(tmp_writer_ptr + 1), (void*)&token, sizeof(int32_t));

    int32_t written_length = tcp_client_socket.WriteN(RECOVERY_REQUEST_MESSAGE_LENGTH, request_message);

    if (written_length < (int32_t)RECOVERY_REQUEST_MESSAGE_LENGTH) {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO SEND MESSAGE TO RECOVERY HOST, SYSTEMERROR : " << strerror(errno)
                                   << " WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      tcp_client_socket.Close();
      exit(-1);
    }

    int32_t packet_length = 0;

    // First 4 bytes will indicate length of the actual data
    while (packet_length < 4) {
      int32_t read_length =
          tcp_client_socket.ReadN(MAX_RECOVERY_PACKET_SIZE - packet_length, recovery_buffer + packet_length);
      DBGLOG_CLASS_FUNC_LINE_INFO << "READ RETURNED WITH : " << read_length << " PACKET LENGTH : " << packet_length
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if (read_length <= 0) {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO READ FROM RECOVERY HOST, WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        tcp_client_socket.Close();
        exit(-1);
      }

      packet_length += read_length;
    }

    int32_t data_length = *((int32_t*)((char*)(recovery_buffer)));

    // This means server is not ready for recovery
    if (-2 == data_length) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << " RETURNED WITH-2 " << DBGLOG_ENDL_FLUSH;
      tcp_client_socket.Close();
      return false;
    }

    // Check if we have read all what was sent and expected
    while (packet_length < data_length + 4) {
      int32_t read_length =
          tcp_client_socket.ReadN(MAX_RECOVERY_PACKET_SIZE - packet_length, recovery_buffer + packet_length);

      DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY READ RETURNED WITH : " << read_length
                                  << " PACKET LENGTH : " << packet_length << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if (read_length <= 0) {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO READ FROM RECOVERY HOST, WILL EXIT... " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        tcp_client_socket.Close();
        exit(-1);
      }

      packet_length += read_length;
    }

    int64_t max_seq_number = 0;
    char* msg_ptr = recovery_buffer + 4;

    // Packets are not in order of sequence number, hence let's first check what's the max sequence number
    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(NSE_MDS::NSETBTDataCommonStruct);
         event_counter++) {
      max_seq_number = std::max((uint32_t)max_seq_number, ((NSE_MDS::NSETBTDataCommonStruct*)(msg_ptr))->msg_seq);
      msg_ptr += sizeof(NSE_MDS::NSETBTDataCommonStruct);
    }

    // Well, recovery has failed, It's possible we just missed it by a seq or two but we'll need to go for another round
    // for this
    // security, We'll allow a lag upto 50 packets
    if (max_seq_number < (int64_t)current_seq_num - 50) {
      tcp_client_socket.Close();
      return false;
    }

    msg_ptr = recovery_buffer + 4;

    // Timestamp is not marked for recovery packets via combined source, It's fine to have same timestamp of all packets
    // I think ?
    struct timeval event_time;
    gettimeofday(&event_time, NULL);

    DBGLOG_CLASS_FUNC_LINE_INFO << "RECOVERY SUCCESSFUL..., TOTAL PACKETS RECEIVED : "
                                << data_length / sizeof(NSE_MDS::NSETBTDataCommonStruct)
                                << " SYNCED UPTO SEQ : " << max_seq_number
                                << " APPLYING PACKETS TO BOOK NOW UPTO REQUESTED SEQ.. " << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    for (uint32_t event_counter = 0; event_counter < data_length / sizeof(NSE_MDS::NSETBTDataCommonStruct);
         event_counter++) {
      NSE_MDS::NSETBTDataCommonStruct* event = (NSE_MDS::NSETBTDataCommonStruct*)(msg_ptr);

      // We are done here with recovery
      //      if (event->msg_seq >= current_seq_num) break;
      if (event->msg_seq >= current_seq_num)
        continue;  // Since We are getting buffer of recovery packets which is sorted based on OrderIds and not seq

      // DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET DATA : " << event->ToString() << DBGLOG_ENDL_NOFLUSH;
      p_time_keeper_->OnTimeReceived(event_time);

      //      DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY PACKET, SEC ID :" << sec_id << " PRICE : "
      //                                  << (event->data).nse_order.order_price /
      //                                  (double)nse_ref_data[token].price_multiplier
      //                                  << " QTY : " << (event->data).nse_order.order_qty << DBGLOG_ENDL_NOFLUSH;
      //      DBGLOG_DUMP;

      // All orders will be passed as OrderAdd only since they are coming from recovery as live orders
      // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
      p_order_global_listener_nse_->OnOrderAdd(
          sec_id, (event->data).nse_order.buysell, (event->data).nse_order.order_id,
          (event->data).nse_order.order_price / (double)nse_ref_data[token].price_multiplier,
          (event->data).nse_order.order_qty, false);

      // HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);

      msg_ptr += sizeof(NSE_MDS::NSETBTDataCommonStruct);
    }

    // Recovery Done For This Security Id
    security_id_to_recovery_complete_[sec_id] = max_seq_number;
    DBGLOG_CLASS_FUNC_LINE_INFO << " RECOVERY COMPLETE, CLOSING SOCKET SEC ID :" << sec_id << " "
                                << tcp_client_socket.socket_file_descriptor() << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    tcp_client_socket.Close();

    return true;
  }
  Sanskar: Do not know about recovery yet.
  */
  inline void AddComboProductToSegmentMap(const std::string internal_symbol_str){
    if(internal_symbol_str.empty()){
      std::cerr<<"Requested internal symbol is empty please try with a valid internal symbol"<<std::endl;
      return;
    }
    int32_t security_id = -1;
    std::string exchange_symbol =
          CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str);
    
    if (std::string("INVALID") != exchange_symbol) {
      security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
//        std::cout<<"Security Id:"<<security_id<<"\n";
      segment_to_exchSymbol_secid_map_[CBOE_COMBO_SEGMENT_MARKING].insert(std::make_pair(exchange_symbol, security_id));
      if (security_id >= 0) {
        // security_id_to_inverse_price_mult_[security_id] = (1.0 / (double)(itr.second).price_multiplier); //Sanskar:Do not know what will be the price_multiplier for each security??
      }
    }
  }
  inline void ProcessIBKREvent(IBL1UpdateTick* next_event_) {

//    std::cout << "ProcessIBKREvent " << std::endl; 
//    std::cout << next_event_->ToString() << std::endl;
    // This was a hack here
    // if(next_event_->ib_update_type == IBUpdateType::IB_GREEKS_UPDATE ){
    //   next_event_->ib_update_type = IBUpdateType::IB_TRADE;
    //   next_event_->trade_price = 6075;
    //   std::memcpy(next_event_->symbol, "CBOE_SPX", sizeof("CBOE_SPX"));
    //   std::cout<<"Changed the greek update\n";
      // std::cout<<next_event_->ToString()<<"\n";
    // }
    // DBGLOG_CLASS_FUNC_LINE_INFO << "Ibkr processor received data" << DBGLOG_ENDL_NOFLUSH;
    //   DBGLOG_DUMP;
    // There will be a few instruments for which datasymbol/exchange won't be defined, TODO : use bool to reduce latency
    char segment_type=CBOE_FO_SEGMENT_MARKING; //For now we want only this segment
    const std::string exchange_symbol =
          CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(next_event_->symbol);
//    std::cout << " Exchange Symbol : " << exchange_symbol << " " << next_event_->symbol << std::endl;
    //It is a hack here
    // if(exchange_symbol == "CBOE_IDX416904") segment_type = CBOE_IX_SEGMENT_MARKING;
    if(exchange_symbol.substr(0, 8) == "CBOE_IDX") segment_type = CBOE_IX_SEGMENT_MARKING;
    else if(exchange_symbol.substr(0,6) == "CBOE_C") segment_type = CBOE_COMBO_SEGMENT_MARKING;

    // if(segment_type == CBOE_COMBO_SEGMENT_MARKING){
    //   std::cout<<"Before\n";
    //   std::cout<<next_event_->ToString()<<"\n";
    // }
    // DBGLOG_CLASS_FUNC_LINE_INFO << "The events info are, internal symbol:"<<next_event_->symbol<<" ,exchange Symbol:"<< exchange_symbol <<",segment type:"<< segment_type << DBGLOG_ENDL_NOFLUSH;
    //   DBGLOG_DUMP;

      // if(std::strncmp(next_event_->symbol,"CBOE_SPXW_COMBO_0",17)==0){

      //   std::cout<<next_event_->ToString()<<std::endl;
      //   std::cout<<segment_type<<std::endl;
      //   std::cout<<segment_to_exchSymbol_secid_map_[segment_type].size()<<std::endl;

      // }
//    std::cout << " Symbol : " << exchange_symbol << " " << segment_type << std::endl;
    if (segment_to_exchSymbol_secid_map_[segment_type].find(exchange_symbol) ==
        segment_to_exchSymbol_secid_map_[segment_type].end())
      return;
    //Sanskar:Assuming ticker_id and token are same 



    int32_t security_id = segment_to_exchSymbol_secid_map_[segment_type][exchange_symbol];
//std::cout << " Security Id : " << security_id << " For Shortcode : " << exchange_symbol <<std::endl;

    // DBGLOG_CLASS_FUNC_LINE_INFO << "The event's security is:"<< security_id << DBGLOG_ENDL_NOFLUSH;
    //   DBGLOG_DUMP;
    
    if (security_id < 0) return;

    // if(segment_type == CBOE_COMBO_SEGMENT_MARKING){
    //   std::cout<<"After\n";
    //   std::cout<<next_event_->ToString()<<"\n";
    // }
    // Check if we have ever gone for recovery, If not we'll assume we have missed things and now we'll want to recover
    // all live orders
    //Sanskar: DO not know about recovery yet.
    /*if (-1 == security_id_to_recovery_complete_[security_id]) {
      struct timeval current_time;
      gettimeofday(&current_time, NULL);

      DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << " TOKEN : " << next_event_->token
                                  << " SYMBOL : " << sec_name_indexer_.GetSecurityNameFromId(security_id)
                                  << " DATA SYMBOL : " << sec_name_indexer_.GetShortcodeFromId(security_id)
                                  << " IS GOING FOR RECOVERY NOW... CURRENT SEQUENCE : " << next_event_->msg_seq
                                  << " @ " << current_time.tv_sec << "." << current_time.tv_usec << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // We need to intentionally make client slow if we failed to recover,
      // It's very likely combinedwriter is leading the tbt recovery manager and this sleep is
      // required to make sure we don't push too many TCP connection requests
      HFSAT::usleep(100);

      // It's possible that the recovery host is not yet uptodate, We might have to call it sometime later
      if (true == RecoverLiveOrders(next_event_->token, next_event_->msg_seq, security_id, next_event_->segment_type)) {
        gettimeofday(&current_time, NULL);
        DBGLOG_CLASS_FUNC_LINE_INFO << "SECURITY : " << security_id << "RECOVERY COMPLETED SUCCESSFULLY... "
                                    << " @ " << current_time.tv_sec << "." << current_time.tv_usec
                                    << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

      } else {
        gettimeofday(&current_time, NULL);
        DBGLOG_CLASS_FUNC_LINE_ERROR << "SECURITY : " << security_id
                                     << " FAILED TO RECOVER... WILL ATTEMPT NEXT TIME.. "
                                     << " @ " << current_time.tv_sec << "." << current_time.tv_usec
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        return;  // We'll wait for next turn
      }
    }*/
  //  DBGLOG_CLASS_FUNC_LINE_INFO << "Ib_Update_Type :" << next_event_->ib_update_type
  //                                      << DBGLOG_ENDL_NOFLUSH;
  //         DBGLOG_DUMP;
    switch (next_event_->ib_update_type) {
      case IBUpdateType::IB_TICK_UPDATE: {
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Side : " << next_event_->side
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        if ('A' == next_event_->side) {         // Ask update
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->time);

          p_order_global_listener_ibkr_->Ib_Tick_Update(
              security_id, HFSAT::TradeType_t::kTradeTypeSell,
              next_event_->ask_price,
              next_event_->ask_size,false);
        } else if ('B' == next_event_->side) {  // Bid update
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->time);

          p_order_global_listener_ibkr_->Ib_Tick_Update(
              security_id, HFSAT::TradeType_t::kTradeTypeBuy,
              next_event_->bid_price,
              next_event_->bid_size,false);
          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
        } else {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TICK UPDATE RECEVIED IN DATA : " << next_event_->side
                                       << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

      } break;

      case IBUpdateType::IB_TICK_SIZE_ONLY_UPDATE: {
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Side : " << next_event_->side
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        if ('S' == next_event_->side) {         // Ask size only update
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->time);

          p_order_global_listener_ibkr_->Ib_Tick_Size_Only_Update(
              security_id, HFSAT::TradeType_t::kTradeTypeSell,
              next_event_->ask_size,false);
        } else if ('B' == next_event_->side) {  // Bid size only update
          // Update watch
          p_time_keeper_->OnTimeReceived(next_event_->time);

          p_order_global_listener_ibkr_->Ib_Tick_Size_Only_Update(
              security_id, HFSAT::TradeType_t::kTradeTypeBuy,
              next_event_->bid_size,false);
          // HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
        } else {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TICK SIZE ONLY UPDATE RECEVIED IN DATA : " << next_event_->side
                                       << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

      } break;

      case IBUpdateType::IB_TRADE: {
        // DBGLOG_CLASS_FUNC_LINE_INFO << "Ib_Update_Type :" << next_event_->ib_update_type
        //                                << DBGLOG_ENDL_NOFLUSH;
        //   DBGLOG_DUMP;
        p_time_keeper_->OnTimeReceived(next_event_->time);

        /*int32_t bid_size_remaining = 0;
        int32_t ask_size_remaining = 0;*/

        // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);
        p_order_global_listener_ibkr_->Ib_Trade(
            security_id,next_event_->trade_price,next_event_->trade_size,TradeType_t::kTradeTypeNoInfo);
        // HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
      } break;

      case IBUpdateType::IB_GREEKS_UPDATE: {
      } break;

      case IBUpdateType::IB_INFO_UPDATE: {
      } break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED TYPE OF MESSAGE RECEVIED : " << (int32_t)(next_event_->ib_update_type)
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      } break;
    }
  }
};
}
}
