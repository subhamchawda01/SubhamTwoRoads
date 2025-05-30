// =====================================================================================
//
//       Filename:  multicast_forwarder.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/14/2018 05:33:28 AM
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
#include <cstdlib>
#include <set>

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#define MAX_PACKET_SIZE 1024

class MulticastForwarder : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::MulticastReceiverSocket mcast_receiver_socket_;
  char read_buffer[MAX_PACKET_SIZE];
  HFSAT::MulticastSenderSocket mcast_sender_socket_;

 public:
  MulticastForwarder(std::string mcast_ip, int32_t mcast_port, std::string interface, std::string mcast_out_ip,
                     int32_t mcast_out_port, std::string out_interface)
      : mcast_receiver_socket_(mcast_ip, mcast_port, interface),
        read_buffer(),
        mcast_sender_socket_(mcast_out_ip, mcast_out_port, out_interface) {
    mcast_receiver_socket_.SetNonBlocking();
    std::cout << "CREATED INCOMING SOCKET GROUP : " << mcast_ip << " " << mcast_port << " " << interface << " "
              << mcast_receiver_socket_.socket_file_descriptor() << std::endl;
    std::cout << "CREATED OUTGOING SOCKET GROUP : " << mcast_out_ip << " " << mcast_out_port << " " << out_interface
              << std::endl;
  }

  inline void ProcessAllEvents(int32_t socket_fd) {
    while (true) {
      int32_t mcast_read = mcast_receiver_socket_.ReadN(MAX_PACKET_SIZE, read_buffer);
      if (mcast_read < 1) return;

      int32_t written_length = mcast_sender_socket_.WriteN(mcast_read, read_buffer);
      std::cout << "READ : " << mcast_read << " FROM : " << socket_fd << " FORWARDED : " << written_length << std::endl;
    }
  }

  int32_t socket_file_descriptor() { return mcast_receiver_socket_.socket_file_descriptor(); }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "USAGE : <exec> <config>" << std::endl;
    std::exit(-1);
  }

  std::ifstream config_filestream;
  config_filestream.open(argv[1]);

  if (!config_filestream.is_open()) {
    std::cerr << "CAN'T OPEN CONFIG FILE FOR READING : " << argv[1] << std::endl;
  }

  std::set<std::string> unique_input_mcast_group_keys;
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher;

  char line_buffer[1024];
  while (config_filestream.good()) {
    config_filestream.getline(line_buffer, 1024);

    std::string line = line_buffer;
    if (line.find("#") != std::string::npos) continue;  // comments

    HFSAT::PerishableStringTokenizer pst(line_buffer, 1024);
    std::vector<char const *> const &tokens = pst.GetTokens();

    if (tokens.size() < 6) {
      std::cerr << "MALFORMED LINE : " << line << std::endl;
      continue;
    }

    std::ostringstream t_temp_oss;
    t_temp_oss << tokens[0] << "-" << tokens[1];

    if (unique_input_mcast_group_keys.find(t_temp_oss.str()) == unique_input_mcast_group_keys.end()) {
      std::cout << "FOUND A NEW MCAST GROUP FORWARD CONFIG : " << line << std::endl;

      MulticastForwarder *mcast_forward =
          new MulticastForwarder(tokens[0], atoi(tokens[1]), tokens[2], tokens[3], atoi(tokens[4]), tokens[5]);
      simple_live_dispatcher.AddSimpleExternalDataLiveListenerSocket(mcast_forward,
                                                                     mcast_forward->socket_file_descriptor(), true);
    }
  }

  config_filestream.close();

  simple_live_dispatcher.RunLive();
  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
