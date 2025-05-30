// =====================================================================================
//
//       Filename:  read_aflash_raw_data.cpp
//
//    Description:  Reads Rawdump of alphaflash feed and displays its hex string and
//                  the parsed message
//
//        Version:  1.0
//        Created:  03/10/2014 10:38:06 AM
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

#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/af_msg_parser.hpp"

#define N 4000000

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("USAGE: %s <raw_data_filename> \nSize of TimeVal: %ld\t Int: %ld\n", argv[0], sizeof(timeval),
           sizeof(uint32_t));
    exit(0);
  }

  // sock -> SetNonBlocking();

  std::string fname(argv[1]);
  std::cerr << "File name: " << fname << std::endl;
  HFSAT::BulkFileReader bfr(N);
  bfr.open(fname);
  if (!bfr.is_open()) {
    std::cerr << "Cannot open file: " << argv[4] << std::endl;
  }
  char* msg_buf = new char[N];
  uint32_t msg_len = 1;
  timeval time_;
  char tmbuf[64], buf[64];

  AF_MSGSPECS::AF_MsgParser& msg_parser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance();

  while (msg_len > 0) {
    int lensize = bfr.read(&msg_len, sizeof(int));
    if (lensize < (int)sizeof(int)) break;

    bfr.read(&time_, sizeof(time_));
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", localtime(&time_.tv_sec));
    snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int)time_.tv_usec);
    bfr.read((void*)msg_buf, msg_len);

    std::ostringstream msghex_;
    for (unsigned int i = 0; i < msg_len; i++) {
      char hexbuf[5];
      sprintf(hexbuf, "%02x", (unsigned uint8_t)msg_buf[i]);
      msghex_ << hexbuf << " ";
    }
    std::cout << "TIME: " << buf << "\n";
    std::cout << "HEX: " << msghex_.str() << "\n";

    std::string msg_ = msg_parser_.msgParse(msg_buf);

    std::cout << msg_ << "\n\n";
  }
  bfr.close();
  delete msg_buf;
  return 0;
}
