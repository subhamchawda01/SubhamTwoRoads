#include <time.h>
#include <iostream>
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"

int main(int argc, char** argv) {
  std::string mcast_ip_ = "225.2.2.1";
  ;
  int mcast_port_ = 17117;
  int position = 0;

  if (argc < 4) {
    std::cerr << " Usage: " << argv[0] << " <ip> <port> <global_position>" << std::endl;
    exit(0);
  }

  mcast_ip_ = argv[1];
  mcast_port_ = atoi(argv[2]);
  position = atoi(argv[3]);

  HFSAT::MulticastSenderSocket abc_(mcast_ip_, mcast_port_);

  HFSAT::GenericORSReplyStruct gors;
  strncpy(gors.symbol_, "FESX201109", 16);
  gors.price_ = 2935;
  gors.time_set_by_server_ = HFSAT::GetTimeOfDay();
  gors.orr_type_ = HFSAT::kORRType_Exec;
  gors.server_assigned_message_sequence_ = 1;
  gors.server_assigned_client_id_ = 2;
  gors.size_remaining_ = 0;
  gors.buysell_ = HFSAT::kTradeTypeBuy;
  gors.server_assigned_order_sequence_ = 2;
  gors.client_assigned_order_sequence_ = 1;
  gors.size_executed_ = 5;
  gors.client_position_ = 5;
  gors.global_position_ = position;
  gors.int_price_ = 2935;

  abc_.WriteN(sizeof(gors), &gors);

  return 0;
}
