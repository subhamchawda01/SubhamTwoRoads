/**
   \file Tools/mds_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <stdlib.h>
#include <signal.h>

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"

/**
 * This file is intended to simply dump all messages received on a bcast ip:port
 */

HFSAT::MulticastReceiverSocket* rcv_soc;
HFSAT::BulkFileWriter* writer;

/// signal handler
void sighandler(int signum) {
  std::cout << "received signal\n";
  writer->Close();
  rcv_soc->Close();
  exit(0);
}

int main(int argc, char** argv) {
  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  if (argc != 6) {
    std::cout << "usage: " << argv[0] << " <output-file> <write-len-flag options 0 or 1> <mcast_ip> <mcast_port> "
              << "<network_interface like eth5 or bond0> \n";
    exit(1);
  }
  bool writeLen = atoi(argv[2]) == 1;
  writer = new HFSAT::BulkFileWriter(argv[1]);

  rcv_soc = new HFSAT::MulticastReceiverSocket(argv[3], atoi(argv[4]), argv[5]);

  char buf[1024];
  int max_len = 1024;
  while (true) {
    int len_read = rcv_soc->ReadN(max_len, buf);
    if (len_read == 0) break;
    if (writeLen) {
      writer->Write(&len_read, sizeof(int));
    }
    writer->Write(buf, len_read);
    writer->CheckToFlushBuffer();
  }

  return 0;
}
