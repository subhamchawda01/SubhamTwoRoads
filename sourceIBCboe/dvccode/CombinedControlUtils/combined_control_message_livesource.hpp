/**
    \file dvccode/ORSMessages/combined_control_message_livesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/CombinedControlUtils/combined_control_messages.hpp"
#include "dvccode/CombinedControlUtils/combined_control_message_listener.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"

namespace HFSAT {

/// @brief class to listen to live UDP from a user as to send commands to combined writer
class CombinedControlMessageLiveSource : public SimpleExternalDataLiveListener {
 protected:
  DebugLogger& dbglogger_;
  /// should be deprecated
  std::vector<CombinedControlMessageListener*> combined_control_message_listeners_;
//  HFSAT::Utils::UDPDirectMultipleZocket& udp_direct_multiple_zockets_;
  MulticastReceiverSocket* multicast_receiver_socket_;

  CombinedControlMessage combined_control_request_;
  const int CombinedControlMessageLen;

 public:
  /**
   * @param _dbglogger_ for logging errors
   * @param md_udp_ip the braodcast ip
   * @param md_udp_port the braodcast port
   */
  CombinedControlMessageLiveSource(DebugLogger& _dbglogger_, const std::string& md_udp_ip, const int md_udp_port,
                                   std::string iface_, IPCMode ipcm = MULTICAST,
                                   HFSAT::FastMdConsumerMode_t _mode_ = HFSAT::kLiveConsumer,
                                   bool using_udp_direct = false)
      : dbglogger_(_dbglogger_),
        combined_control_message_listeners_(),
        multicast_receiver_socket_(NULL),
        combined_control_request_(),
        CombinedControlMessageLen(sizeof(CombinedControlMessage)) {
    if (false == using_udp_direct) {
      multicast_receiver_socket_ = new MulticastReceiverSocket(md_udp_ip, md_udp_port, iface_);
      multicast_receiver_socket_->SetNonBlocking();
    } else {
//        udp_direct_multiple_zockets_(hfsat::utils::udpdirectmultiplezocket::getuniqueinstance()),
//        udp_direct_multiple_zockets_.CreateSocketAndAddToMuxer(md_udp_ip, md_udp_port, this, 'X', false);
    }
  }

  /// should be deprecated
  inline void AddCombinedControlMessageListener(CombinedControlMessageListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(combined_control_message_listeners_, _new_listener_);
  }
  inline void RemoveCombinedControlMessageListener(CombinedControlMessageListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(combined_control_message_listeners_, _new_listener_);
  }

  inline int socket_file_descriptor() const { return multicast_receiver_socket_->socket_file_descriptor(); }

  inline void ProcessAllEvents(int this_socket_fd_) {
    while (true) {
      int num_bytes = multicast_receiver_socket_->ReadN(CombinedControlMessageLen, &combined_control_request_);

      if (num_bytes < CombinedControlMessageLen) return;  // returning to signal no data

      for (auto listener_ : combined_control_message_listeners_)
        listener_->OnCombinedControlMessageReceived(combined_control_request_);
    }
  }

  void ProcessEventsFromUDPDirectRead(char const* msg_ptr, int32_t msg_length, char seg_type, bool is_trade_exec_fd, 
                                      bool is_spot_index_fd, bool is_oi_data, uint32_t& udp_msg_seq_no) {
    std::cout << " CombinedControlMessageLiveSource : " << msg_length << " " << CombinedControlMessageLen << std::endl;
    if (msg_length < CombinedControlMessageLen) return;

    for (auto listener_ : combined_control_message_listeners_)
      listener_->OnCombinedControlMessageReceived(*((CombinedControlMessage*)msg_ptr));
  }

 private:
};
}
