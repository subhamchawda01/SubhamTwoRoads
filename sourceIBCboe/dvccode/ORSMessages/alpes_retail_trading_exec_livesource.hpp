// =====================================================================================
//
//       Filename:  alpes_retail_trading_exec_livesource.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/16/2014 01:54:26 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/LiveSources/retail_trade_listener.hpp"

#define MAX_BUFFER_SIZE 8192

namespace HFSAT {

namespace ORSMessages {

struct ReverseData {
  unsigned int ors_assigned_client_id_;
  char product_i_m_trading_[16];
  char pad[4];
};

class AlpesRetailTradingExecLivesource : public SimpleExternalDataLiveListener {
 private:
  int32_t read_offset_;

  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;

  std::map<std::string, std::vector<FPOrderExecutedListener*> > fporder_executed_listener_vec_map_;  ///< map (vector)
  /// from security_id
  /// to vector of
  /// listeners to ORS
  /// messages of type
  /// kORRType_Exec
  ExternalTimeListener* p_time_keeper_;  ///< only meant for Watch
  HFSAT::TCPClientSocket tcp_client_socket_;
  RETAIL_MDS::RETAILCommonStruct next_event_;
  int ors_struct_pkt_size_;
  char data_buffer_[MAX_BUFFER_SIZE];
  int date_;

  AlpesRetailTradingExecLivesource(DebugLogger& _dbglogger_, SecurityNameIndexer& _sec_name_indexer_,
                                   const std::string& _server_ip_, const int _server_port_)
      : read_offset_(0),
        dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        fporder_executed_listener_vec_map_(),
        p_time_keeper_(NULL),
        tcp_client_socket_(),
        next_event_(),
        ors_struct_pkt_size_(sizeof(RETAIL_MDS::RETAILCommonStruct)),
        date_(HFSAT::DateTime::GetCurrentIsoDateUTC())

  {
    DBGLOG_CLASS_FUNC << "sizeof RETAIL_MDS::RETAILCommonStruct: " << ors_struct_pkt_size_ << DBGLOG_ENDL_FLUSH;

    DBGLOG_CLASS_FUNC << " Connecting TO : " << _server_ip_ << " Port : " << _server_port_ << DBGLOG_ENDL_FLUSH;

    tcp_client_socket_.Connect(_server_ip_, _server_port_);

    if (-1 == tcp_client_socket_.socket_file_descriptor()) {  // Ensure We are connected to FIX server //ravi

      DBGLOG_CLASS_FUNC << "FAILED TO CONNECT TO FIX SERVER : " << _server_ip_ << " PORT : " << _server_port_ << "\n";
      exit(-1);
    }

    memset((void*)data_buffer_, 0, MAX_BUFFER_SIZE);
  }

 public:
  ~AlpesRetailTradingExecLivesource() {}

  // In this implementation there is really no need to call SetUniqueInstance
  // We are still persisting with this artifact since this allows us in future
  // to add logic to check that we dn't have multiple livesource objects for the
  // same ip port. Currently there is no such check.
  static AlpesRetailTradingExecLivesource* SetUniqueInstance(DebugLogger& _dbglogger_,
                                                             SecurityNameIndexer& _sec_name_indexer_,
                                                             const std::string& _server_ip_, const int _server_port_) {
    return GetUniqueInstance(_dbglogger_, _sec_name_indexer_, _server_ip_, _server_port_);
  }

  static AlpesRetailTradingExecLivesource* GetUniqueInstance(DebugLogger& _dbglogger_,
                                                             SecurityNameIndexer& _sec_name_indexer_,
                                                             const std::string& _server_ip_, const int _server_port_) {
    static AlpesRetailTradingExecLivesource* p_unique_instance_ = nullptr;
    if (p_unique_instance_ == nullptr) {
      p_unique_instance_ =
          new AlpesRetailTradingExecLivesource(_dbglogger_, _sec_name_indexer_, _server_ip_, _server_port_);
    }
    return p_unique_instance_;
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }
  inline int socket_file_descriptor() const { return tcp_client_socket_.socket_file_descriptor(); }

  inline void InitializeReverseCommunication(const unsigned int& _client_id_, const std::string& _shortcode_) {
    ReverseData reverse_data;
    memset((void*)&(reverse_data), 0, sizeof(ReverseData));
    reverse_data.ors_assigned_client_id_ = _client_id_;
    const char* secname_ = _shortcode_.c_str();
    memcpy((void*)(reverse_data.product_i_m_trading_), (void*)(secname_), 16);

    int write_len = tcp_client_socket_.WriteN(sizeof(ReverseData), (void*)&(reverse_data));

    if (write_len < (int)sizeof(ReverseData)) {
      dbglogger_ << " Failed To Write Data, Written : " << write_len << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }

  inline void AddFPOrderExecutedListener(const std::string& _secname_, FPOrderExecutedListener* _new_listener_) {
    DBGLOG_CLASS_FUNC << _secname_ << DBGLOG_ENDL_FLUSH;
    if (VectorUtils::UniqueVectorAdd(fporder_executed_listener_vec_map_[_secname_], _new_listener_)) {
      DBGLOG_CLASS_FUNC << "New FPOrderExecutedListener added for " << _secname_ << ". Total # of Listeners "
                        << fporder_executed_listener_vec_map_[_secname_].size() << DBGLOG_ENDL_FLUSH;
    }
  }

  inline int ProcessORSReply(char* retail_msg_char_buf_, const int& this_msg_length_) {
    int32_t length_to_be_processed_ = this_msg_length_;
    const char* msg_ptr = retail_msg_char_buf_;

    DBGLOG_CLASS_FUNC << "length_to_be_processed_: " << length_to_be_processed_
                      << " ors_struct_pkt_size_: " << ors_struct_pkt_size_ << DBGLOG_ENDL_FLUSH;

    while (length_to_be_processed_ > 0) {
      if (length_to_be_processed_ < ors_struct_pkt_size_) {  // Incomplete Message

        memcpy((void*)data_buffer_, (void*)msg_ptr, length_to_be_processed_);
        return length_to_be_processed_;
      }

      // Update Single Packet
      memcpy((void*)&(next_event_), (void*)msg_ptr, length_to_be_processed_);
      msg_ptr += ors_struct_pkt_size_;
      length_to_be_processed_ -= ors_struct_pkt_size_;

      DBGLOG_CLASS_FUNC << "length_to_be_processed_: " << length_to_be_processed_
                        << " ors_struct_pkt_size_: " << ors_struct_pkt_size_ << DBGLOG_ENDL_FLUSH;

      DBGLOG_CLASS_FUNC << next_event_.ToString() << DBGLOG_ENDL_FLUSH;

      switch (next_event_.msg_) {
        case RETAIL_MDS::RETAIL_TRADE: {
          p_time_keeper_->OnTimeReceived(HFSAT::GetTimeOfDay());
          if (next_event_.data_.retail_trds_.trd_qty_ > 0) {
            std::string t_symbol_str_ = std::string(next_event_.data_.retail_trds_.contract_);
            if (fporder_executed_listener_vec_map_.find(t_symbol_str_) != fporder_executed_listener_vec_map_.end()) {
              std::vector<FPOrderExecutedListener*>& order_executed_listener_vec_ =
                  fporder_executed_listener_vec_map_[t_symbol_str_];
              DBGLOG_CLASS_FUNC << "Order Executed for " << t_symbol_str_
                                << ". Distributing to listeners : " << order_executed_listener_vec_.size()
                                << DBGLOG_ENDL_FLUSH;

              for (size_t i = 0; i < order_executed_listener_vec_.size(); i++) {
                order_executed_listener_vec_[i]->FPOrderExecuted(
                    next_event_.data_.retail_trds_.contract_, next_event_.data_.retail_trds_.trd_px_,
                    next_event_.data_.retail_trds_.agg_side_ == 'B' ? kTradeTypeBuy : kTradeTypeSell,
                    next_event_.data_.retail_trds_.trd_qty_);
              }
            }
          }
        } break;

        default: { } break; }
    }
    return 0;  // Message was processed completely
  }

  inline void ProcessAllEvents(int this_socket_fd_) {
    int read_size = tcp_client_socket_.ReadN(MAX_BUFFER_SIZE - read_offset_, data_buffer_ + read_offset_);

    if (read_size <= 0) {
      DBGLOG_CLASS_FUNC << " READ ERROR ON FIX SERVER SOCKET \n ";
      exit(-1);
    }

    read_offset_ = ProcessORSReply(data_buffer_, read_offset_ + read_size);
  }
};
}
}
