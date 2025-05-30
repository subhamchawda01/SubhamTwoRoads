// =====================================================================================
//
//       Filename:  generic_live_data_source.hpp
//
//    Description:  A Simple Utility Class Which Will Only Operate On Processing Live Data And Providing The Same To
//    CombinedSource ( Writer )
//
//        Version:  1.0
//        Created:  11/11/2014 07:25:47 AM
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

#include <errno.h>  //To get The description of the last system error stored

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Utils/nse_mds_shm_interface.hpp"
#include "dvccode/CommonDataStructures/enhanced_security_name_indexer.hpp"
#include "dvccode/Listeners/live_products_manager_listener.hpp"
#include "infracore/Tools/live_products_manager.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"

#define MAX_LIVE_SOCKETS_FD 1024

namespace HFSAT {
namespace MDSMessages {

template <class T>  // A Generilized Class
class MultiShmLiveDataSource : public HFSAT::SimpleExternalDataLiveListener, public LiveProductsManagerListener {
 private:
  DebugLogger& dbglogger_;
  HFSAT::Utils::UDPDirectMultipleZocket& udp_direct_multiple_zockets_;
  std::vector<HFSAT::MulticastReceiverSocket*> multicast_receiver_socket_vec_;  // Multicast Socket Reader
  std::vector<int32_t> socket_fd_to_actual_socket_index_map_;
  T next_event_;            // The Underlying Common Struct
  const int32_t pkt_size_;  // So That we don't keep calling sizeof

  // These are used for generic write
  HFSAT::Utils::NSEMDSShmInterface& nse_mds_shm_interface_;
  HFSAT::MDS_MSG::MDSMessageExchType source_md_exch_type_;
  NSE_MDS::NSETBTDataCommonStructProShm multishm_mds_message_;

  int32_t socket_read_size_;

  HFSAT::EnhancedSecurityNameIndexer requested_products_indexer_;

  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;

  bool is_overclocked_server_;
  bool is_midterm_legacy_server_;

 public:
  MultiShmLiveDataSource(DebugLogger& _dbglogger_, const HFSAT::MDS_MSG::MDSMessageExchType& _source_md_exch_type_)
      : dbglogger_(_dbglogger_),
        udp_direct_multiple_zockets_(HFSAT::Utils::UDPDirectMultipleZocket::GetUniqueInstance()),
        multicast_receiver_socket_vec_(),
        socket_fd_to_actual_socket_index_map_(),
        next_event_(),
        pkt_size_(sizeof(T)),
        nse_mds_shm_interface_(HFSAT::Utils::NSEMDSShmInterface::GetUniqueInstance()),
        source_md_exch_type_(_source_md_exch_type_),
        multishm_mds_message_(),
        socket_read_size_(-1),
        clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
        using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()),
        is_overclocked_server_(HFSAT::IsItOverClockedServer()),
        is_midterm_legacy_server_(HFSAT::IsItMidTermLegacyServer()) {
    std::cout << " MID TERM ? : " << is_midterm_legacy_server_ << std::endl;

    // Reset Memory
    memset((void*)&multishm_mds_message_, 0, sizeof(NSE_MDS::NSETBTDataCommonStructProShm));
    socket_fd_to_actual_socket_index_map_.resize(MAX_LIVE_SOCKETS_FD, -1);
  }

  void AddUDPDirectLiveSocket(const std::string& _mcast_ip_, const int32_t& _mcast_port_) {
    udp_direct_multiple_zockets_.CreateSocketAndAddToMuxer(_mcast_ip_, _mcast_port_, this, 'X', false, false, false);
  }

  // A call to include all live sockets
  void AddLiveSocket(const std::string& _mcast_ip_, const int32_t& _mcast_port_, const std::string& _interface_) {
    HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket =
        new HFSAT::MulticastReceiverSocket(_mcast_ip_, _mcast_port_, _interface_);
    multicast_receiver_socket_vec_.push_back(new_multicast_receiver_socket);

    // Check If the FD -> index of the socket can be stored directly or we have to increase the capacitiy
    if ((int32_t)socket_fd_to_actual_socket_index_map_.size() <
        new_multicast_receiver_socket->socket_file_descriptor()) {
      // <= is imporant since we are not utilising the [ 0 ] element, FD can't be zero,
      for (int32_t start_ptr = socket_fd_to_actual_socket_index_map_.size();
           start_ptr <= (new_multicast_receiver_socket->socket_file_descriptor()); start_ptr++) {
        socket_fd_to_actual_socket_index_map_.push_back(-1);
      }
    }

    // At this point the capacity has been set correctly for storing the FD -> socket_index
    socket_fd_to_actual_socket_index_map_[new_multicast_receiver_socket->socket_file_descriptor()] =
        (multicast_receiver_socket_vec_.size() - 1);
  }

  // Fill Up All the socket fd so the caller can add the same to it's event dispatch loop
  void GetLiveSocketsFdList(std::vector<int32_t>& _all_active_socket_fd_vec_) {
    for (uint32_t socket_counter = 0; socket_counter < multicast_receiver_socket_vec_.size(); socket_counter++) {
      _all_active_socket_fd_vec_.push_back(multicast_receiver_socket_vec_[socket_counter]->socket_file_descriptor());
      multicast_receiver_socket_vec_[socket_counter]->SetNonBlocking();
    }
  }

  // Overridden Callback - A listenr of the event dispatcher
  inline void ProcessAllEvents(int32_t _socket_fd_with_data_) {
    while (true) {
      socket_read_size_ =
          multicast_receiver_socket_vec_[socket_fd_to_actual_socket_index_map_[_socket_fd_with_data_]]->ReadN(
              pkt_size_, (void*)&next_event_);

      if (pkt_size_ > socket_read_size_) {
        return;
      }

      // Process the Data Based on Underlying Exchange
      switch (source_md_exch_type_) {
        case HFSAT::MDS_MSG::CONTROL: {
          // Reset Memory
          memset((void*)&multishm_mds_message_, 0, sizeof(NSE_MDS::NSETBTDataCommonStructProShm));
          multishm_mds_message_.msg_type = kMsgTypeControl;

          struct timeval tv;
          gettimeofday(&tv, NULL);
          multishm_mds_message_.source_time = tv;

          multishm_mds_message_.msg_seq = next_event_.trader_id_;
          multishm_mds_message_.data.control_msg.message_code_ = next_event_.control_message_.message_code_;
          multishm_mds_message_.data.control_msg.intval_1_ = next_event_.control_message_.intval_1_;
          memcpy((void*)multishm_mds_message_.data.control_msg.strval_1_, (void*)next_event_.control_message_.strval_1_,
                 sizeof(multishm_mds_message_.data.control_msg.strval_1_));

        } break;
        default: {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED DATA SOURCE TYPE : " << source_md_exch_type_
                                       << " DISCARDING PACKET " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        } break;
      }

      nse_mds_shm_interface_.WriteGenericStruct(&multishm_mds_message_);
    }
  }

  void ProcessEventsFromUDPDirectRead(char const* msg_ptr, int32_t msg_length, char seg_type, bool is_trade_exec_fd,
                                      bool is_spot_index_fd, bool is_oi_index_fd, uint32_t& udp_msg_seq_no) {
    if (msg_length < pkt_size_) return;

    memcpy((void*)&next_event_, (void*)msg_ptr, msg_length);

    // Process the Data Based on Underlying Exchange
    switch (source_md_exch_type_) {
      case HFSAT::MDS_MSG::CONTROL: {
        // Reset Memory
        memset((void*)&multishm_mds_message_, 0, sizeof(NSE_MDS::NSETBTDataCommonStructProShm));
        multishm_mds_message_.msg_type = kMsgTypeControl;

        struct timeval tv;
        gettimeofday(&tv, NULL);
        multishm_mds_message_.source_time = tv;

        multishm_mds_message_.msg_seq = next_event_.trader_id_;
        multishm_mds_message_.data.control_msg.message_code_ = next_event_.control_message_.message_code_;
        multishm_mds_message_.data.control_msg.intval_1_ = next_event_.control_message_.intval_1_;
        memcpy((void*)multishm_mds_message_.data.control_msg.strval_1_, (void*)next_event_.control_message_.strval_1_,
               sizeof(multishm_mds_message_.data.control_msg.strval_1_));

      } break;
      default: {
        DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED DATA SOURCE TYPE : " << source_md_exch_type_
                                     << " DISCARDING PACKET " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      } break;
    }

    nse_mds_shm_interface_.WriteGenericStruct(&multishm_mds_message_);
  }

  void OnLiveProductChange(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                           ExchSource_t exchange) override {
    if (action == LiveProductsChangeAction::kAdd) {
      std::cout << "GenericRavLiveDataSource Added " << shortcode << std::endl;
      requested_products_indexer_.AddString(exchange_symbol.c_str(), shortcode);
    } else if (action == LiveProductsChangeAction::kRemove) {
      std::cout << "GenericRavLiveDataSource Removed " << shortcode << std::endl;
      requested_products_indexer_.RemoveString(exchange_symbol.c_str(), shortcode);
    }
  }

  void CleanUp() {
    for (uint32_t socket_counter = 0; socket_counter < multicast_receiver_socket_vec_.size(); socket_counter++) {
      if (NULL != multicast_receiver_socket_vec_[socket_counter]) {
        delete multicast_receiver_socket_vec_[socket_counter];
        multicast_receiver_socket_vec_[socket_counter] = NULL;
      }
    }
  }
};
}
}
