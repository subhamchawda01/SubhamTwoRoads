#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y%m%d", &tstruct);

  return buf;
}

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#define N 4000000

int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("USAGE: %s <ip> <port> <interface>\nSize of TimeVal: %ld\t Int: %ld\n", argv[0], sizeof(timeval),
           sizeof(uint32_t));
    exit(0);
  }

  HFSAT::MulticastReceiverSocket* sock = new HFSAT::MulticastReceiverSocket(argv[1], atoi(argv[2]), argv[3]);
  sock->setBufferSize(N);
  // sock -> SetNonBlocking();

  char fname[200];
  sprintf(fname, "/spare/local/MDSlogs/RawData/%s_%s_%s_%s.raw", argv[1], argv[2], argv[3], currentDateTime().c_str());
  std::cerr << "File name: " << fname << std::endl;
  HFSAT::BulkFileWriter bfw(fname, N);
  if (!bfw.is_open()) {
    std::cerr << "Cannot open file: " << argv[4] << std::endl;
  }
  char* msg_buf = new char[N];
  // char msg_len_string[10];
  uint32_t msg_len = 1;
  timeval time_;

  while (msg_len > 0) {
    msg_len = sock->ReadN(72000, (void*)msg_buf);
    gettimeofday(&time_, NULL);
    bfw.Write(&msg_len, sizeof(int));
    bfw.Write(&time_, sizeof(time_));
    // std::cerr<<"MSG-LEN: " << msg_len << std::endl;
    bfw.Write((void*)msg_buf, msg_len);
    bfw.DumpCurrentBuffer();
  }
  bfw.DumpCurrentBuffer();
  bfw.Close();
  return 0;
}
