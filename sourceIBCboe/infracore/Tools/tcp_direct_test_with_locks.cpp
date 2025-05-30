// =====================================================================================
//
//       Filename:  tcp_direct_test.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/08/2018 06:57:45 AM
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

#include <cstdlib>
#include <iostream>
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/tcp_client_socket.hpp"

class SendPackets : public HFSAT::Thread {
 private:
  HFSAT::Utils::TCPDirectClientZocketMuxerRX &tcp_direct_rx_;

 public:
  SendPackets() : tcp_direct_rx_(HFSAT::Utils::TCPDirectClientZocketMuxerRX::GetUniqueInstance()) {}

  void thread_main() {
    setAffinityCore(19);

    //    char * rcv_buffer = NULL;
    //    int32_t rcv_length = 0 ;

    int32_t total_rcv_packets = 0;
    bool events_available = false;
    int32_t rcv_length = 0;

    while (true) {
      //      char * data_recv = tcp_direct_rx_.WaitForEvents(rcv_length, events_available);
      //
      //      if ( events_available ){
      //        total_rcv_packets++;
      //        std::cout << "LENGTH : " << rcv_length << " PACKET : " << total_rcv_packets << " DATA : " <<
      //        *(data_recv) << std::endl ;
      //      }
      //
      //      if( 100000 == total_rcv_packets )break;
    }
  }
};

int main(int argc, char *argv[]) {
  //  HFSAT::TCPClientSocket tcp_client_socket_;
  //  tcp_client_socket_.Connect("10.23.23.15", 36881);
  //
  //  int32_t count = 0 ;
  //  std::vector< uint32_t > values;
  //  uint64_t start_cycles = 0 ;
  //  uint64_t end_cycles = 0 ;
  //
  //  uint32_t pid = getpid();
  //
  //  CPUManager::setAffinity(21,pid);
  //  int32_t wrote = 0 ;
  //
  //  char send_buf[206];
  //
  //  while( true ) {
  //
  //    std::ostringstream tempstr;
  //    tempstr << "ABCDEFGHIJKL" << " PLUS " << count+1 ;
  //
  //    memset((void*)send_buf, 0, sizeof(send_buf));
  //    memcpy((void*)send_buf, (void*)tempstr.str().c_str(), tempstr.str().length());
  //
  //    start_cycles = HFSAT::GetCpucycleCountForTimeTick();
  //    wrote = tcp_client_socket_.WriteN(206, send_buf);
  //    end_cycles = HFSAT::GetCpucycleCountForTimeTick();
  //
  ////    std::cout << " WROTE : " << wrote << std::endl ;
  //
  //    HFSAT::usleep(2);
  //
  //    if(wrote>0)
  //    values.push_back(end_cycles-start_cycles);
  //
  ////    sleep (  );
  //    count++ ;
  //
  //    if ( count == 100000 )break;
  //  }
  //
  //  std::sort(values.begin(), values.end());
  //
  //  std::cout << "MEDIAN : " << values[values.size()/2] << " 95 :"  << values[values.size()*0.95] << " " <<
  //  values.size() << std::endl;
  //
  ////  send_packets.run();
  ////  send_packets.stop ();

  // TCP Direct

  HFSAT::Utils::TCPDirectClientZocketTX tcp_direct_client_zocket(nullptr, nullptr, "temp");
  tcp_direct_client_zocket.ConnectAndAddZocketToMuxer("10.23.23.15", 36881);

  SendPackets send_packets;
  send_packets.run();

  int32_t count = 0;
  std::vector<uint32_t> values;
  uint64_t start_cycles = 0;
  uint64_t end_cycles = 0;

  uint32_t pid = getpid();

  CPUManager::setAffinity(21, pid);
  int32_t wrote = 0;

  char send_buf[48];

  while (true) {
    std::ostringstream tempstr;
    tempstr << "ABCDEFGHIJKL"
            << " PLUS " << count + 1;

    memset((void *)send_buf, 0, sizeof(send_buf));
    memcpy((void *)send_buf, (void *)tempstr.str().c_str(), tempstr.str().length());

    start_cycles = HFSAT::GetCpucycleCountForTimeTick();
    wrote = tcp_direct_client_zocket.WriteN(48, send_buf);
    end_cycles = HFSAT::GetCpucycleCountForTimeTick();

    //    HFSAT::usleep(10);
    sleep(30);

    //    std::cout << " WROTE : " << wrote << std::endl ;

    //    HFSAT::usleep(1000000);
    //    tcp_direct_client_zocket.DoReactorPerform();

    if (wrote > 0) values.push_back(end_cycles - start_cycles);

    //    sleep (  );
    count++;

    if (count == 100000) break;
  }

  std::sort(values.begin(), values.end());

  std::cout << "MEDIAN : " << values[values.size() / 2] << " 95 :" << values[values.size() * 0.95] << " "
            << values.size() << std::endl;

  send_packets.stop();
  return EXIT_SUCCESS;
}
