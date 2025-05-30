/**
    \file dvccode/ORSMessages/control_message_livesource.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"

namespace HFSAT {

/// @brief class to listen to live UDP from OrderRoutingServer, when it is a separate Process
class ControlMessageLiveSource : public SimpleExternalDataLiveListener {
 protected:
  DebugLogger& dbglogger_;
  /// should be deprecated
  std::map<std::string, std::vector<ControlMessageListener*> >
      string2_control_message_listener_vec_map_; /**< map (vector) from shortcode to vector of listeners to
                                                    ControlMessage */
  std::map<int, std::vector<ControlMessageListener*> > int2_control_message_listener_vec_map_;

  ExternalTimeListener* p_time_keeper_;

  MulticastReceiverSocket multicast_receiver_socket_;

  GenericControlRequestStruct control_request_;
  const int GenericControlRequestStructLen_;

 public:
  /**
   * @param _dbglogger_ for logging errors
   * @param md_udp_ip the braodcast ip
   * @param md_udp_port the braodcast port
   */
  ControlMessageLiveSource(DebugLogger& _dbglogger_, const std::string& md_udp_ip, const int md_udp_port,
                           std::string iface_)
      : dbglogger_(_dbglogger_),
        string2_control_message_listener_vec_map_(),
        int2_control_message_listener_vec_map_(),
        p_time_keeper_(NULL),
        multicast_receiver_socket_(md_udp_ip, md_udp_port, iface_),
        control_request_(),
        GenericControlRequestStructLen_(sizeof(GenericControlRequestStruct)) {
    multicast_receiver_socket_.SetNonBlocking();
  }

  /// should be deprecated
  inline void AddControlMessageListener(const std::string& _shortcode_, ControlMessageListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(string2_control_message_listener_vec_map_[_shortcode_], _new_listener_);
  }
  inline void RemoveControlMessageListener(const std::string& _shortcode_, ControlMessageListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(string2_control_message_listener_vec_map_[_shortcode_], _new_listener_);
  }

  /// listener functions for ints
  inline void AddControlMessageListener(const int trader_id_, ControlMessageListener* _new_listener_) {
    if (int2_control_message_listener_vec_map_.find(trader_id_) == int2_control_message_listener_vec_map_.end()) {
      int2_control_message_listener_vec_map_[trader_id_] = std::vector<ControlMessageListener*>();
    }
    VectorUtils::UniqueVectorAdd(int2_control_message_listener_vec_map_[trader_id_], _new_listener_);
  }
  inline void RemoveControlMessageListener(const int trader_id_, ControlMessageListener* _new_listener_) {
    if (int2_control_message_listener_vec_map_.find(trader_id_) == int2_control_message_listener_vec_map_.end()) {
      return;
    }
    VectorUtils::UniqueVectorRemove(int2_control_message_listener_vec_map_[trader_id_], _new_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline int socket_file_descriptor() const { return multicast_receiver_socket_.socket_file_descriptor(); }

  inline void ProcessAllEvents(int this_socket_fd_) {
    while (true) {
      int num_bytes = multicast_receiver_socket_.ReadN(GenericControlRequestStructLen_, &control_request_);

      if (num_bytes < GenericControlRequestStructLen_) return;  // returning to signal no data

      /// Semantics - control message should be associated with either a symbol or a trader id.
      p_time_keeper_->OnTimeReceived(control_request_.time_set_by_frontend_);

      if (control_request_.symbol_ &&
          string2_control_message_listener_vec_map_.find(control_request_.symbol_) !=
              string2_control_message_listener_vec_map_.end()) {
        std::vector<ControlMessageListener*>& listener_vec =
            string2_control_message_listener_vec_map_[control_request_.symbol_];
        for (size_t i = 0; i < listener_vec.size(); i++)
          listener_vec[i]->OnControlUpdate(control_request_.control_message_, control_request_.symbol_,
                                           control_request_.trader_id_);
      } else if (control_request_.trader_id_ > 0 &&
                 int2_control_message_listener_vec_map_.find(control_request_.trader_id_) !=
                     int2_control_message_listener_vec_map_.end()) {
        std::vector<ControlMessageListener*>& listener_vec =
            int2_control_message_listener_vec_map_[control_request_.trader_id_];
        for (size_t i = 0; i < listener_vec.size(); i++)
          listener_vec[i]->OnControlUpdate(control_request_.control_message_, control_request_.symbol_,
                                           control_request_.trader_id_);
      }
    }
  }

 private:
};
}
