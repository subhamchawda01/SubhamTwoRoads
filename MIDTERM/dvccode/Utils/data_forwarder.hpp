// =====================================================================================
//
//       Filename:  data_forwarder.hpp
//
//    Description:  A SIMPLE CLASS TO LISTEN ON NUMBER OF MULTICAST CHANNELS AND FORWARDS THE SAME ON GIVEN LIST OF
//    MULTICAST OUTGOING CHANNELS
//
//        Version:  1.0
//        Created:  10/21/2014 01:23:54 PM
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

#include <iostream>
#include <fstream>

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define BUFFER_SIZE 16384

namespace HFSAT {
namespace Utils {

class DataForwarder : public SimpleExternalDataLiveListener {
 private:
  // Event Trigger Class, Takes a bunch of sockets and notifies when events are available
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  // A list of incoming traffic sockets
  std::vector<HFSAT::MulticastReceiverSocket*> list_of_inbound_multicast_socket_subscription_;
  std::vector<uint32_t> list_of_inbound_multicast_channel_ports_;

  // A list of sockets where we want to send data out
  std::vector<HFSAT::MulticastSenderSocket*> list_of_outbound_multicast_socket_subscription_;

  char read_buffer[BUFFER_SIZE];
  int32_t read_size_;

  // Only Allowing A unique instance
  DataForwarder()
      : simple_live_dispatcher_(),
        list_of_inbound_multicast_socket_subscription_(),
        list_of_inbound_multicast_channel_ports_(),
        list_of_outbound_multicast_socket_subscription_(),
        read_buffer(),
        read_size_(0)

  {
    memset((void*)read_buffer, 0, BUFFER_SIZE);
  }

  DataForwarder(const DataForwarder& _disabled_copy_constructor_);

 public:
  // Static Class scope unique instance
  static DataForwarder& GetUniqueInstance() {
    static DataForwarder unique_instance_;
    return unique_instance_;
  }

  // Caller can handover the multicast socket channel information
  void AddMulticastChannelForInputSubscription(const std::string& _multicast_ip_, const int32_t& _multicast_port_,
                                               const std::string& _interface_) {
    bool create_new_socket = true;

    for (uint32_t ports_counter = 0; ports_counter < list_of_inbound_multicast_channel_ports_.size(); ports_counter++) {
      // Check if we can simple call join on the address, if the ports are same
      if ((uint32_t)_multicast_port_ == list_of_inbound_multicast_channel_ports_[ports_counter]) {
        list_of_inbound_multicast_socket_subscription_[ports_counter]->McastJoin(_multicast_ip_, _interface_);

        create_new_socket = false;
        break;
      }
    }

    // Create a new socket and add to wathlist
    if (true == create_new_socket) {
      list_of_inbound_multicast_channel_ports_.push_back(_multicast_port_);
      list_of_inbound_multicast_socket_subscription_.push_back(
          new HFSAT::MulticastReceiverSocket(_multicast_ip_, _multicast_port_, _interface_));

      // By default treat it as a primary socket, hence last arg is true
      simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
          this,
          list_of_inbound_multicast_socket_subscription_[list_of_inbound_multicast_socket_subscription_.size() - 1]
              ->socket_file_descriptor(),
          true);
    }
  }

  // Caller will assign the multicast sending groups
  void AddMulticastChannelForOutput(const std::string& _multicast_ip_, const int32_t& _multicast_port_,
                                    const std::string& _interface_) {
    list_of_outbound_multicast_socket_subscription_.push_back(
        new HFSAT::MulticastSenderSocket(_multicast_ip_, _multicast_port_, _interface_));
  }

  void Start() { simple_live_dispatcher_.RunLive(); }

  void CleanUp() {}

  void ProcessAllEvents(int32_t _socket_fd_with_data_) {
    for (uint32_t socket_counter = 0; socket_counter < list_of_inbound_multicast_socket_subscription_.size();
         socket_counter++) {
      if (_socket_fd_with_data_ ==
          list_of_inbound_multicast_socket_subscription_[socket_counter]->socket_file_descriptor()) {
        read_size_ = list_of_inbound_multicast_socket_subscription_[socket_counter]->ReadN(BUFFER_SIZE, read_buffer);

        if (read_size_ <= 0) {
          std::cerr << "SOCKET READ RETURNED AN ERROR : " << read_size_ << " " << strerror(errno) << "\n";
          exit(-1);
        }

        for (uint32_t outgoing_socket_counter = 0;
             outgoing_socket_counter < list_of_outbound_multicast_socket_subscription_.size();
             outgoing_socket_counter++) {
          list_of_outbound_multicast_socket_subscription_[outgoing_socket_counter]->WriteN(read_size_, read_buffer);
        }

        break;
      }
    }
  }
};
}
}
