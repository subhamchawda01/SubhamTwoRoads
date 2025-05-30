// =====================================================================================
//
//       Filename:  test_tmx_decoder.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/02/2016 03:37:47 AM
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
#include "dvccode/CDef/debug_logger.hpp"
#include "infracore/DataDecoders/TMXOBF/tmx_obf_decoder.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

int main(int argc, char *argv[]) {
  HFSAT::BulkFileReader bulk_file_reader;
  bulk_file_reader.open(argv[1]);

  HFSAT::DebugLogger dbglogger(10240);
  dbglogger.OpenLogFile("/home/ravi/decoder.log", std::ofstream::out);

  HFSAT::TMXOBF::TMXOBFDecoder &decoder = HFSAT::TMXOBF::TMXOBFDecoder::GetUniqueInstance(dbglogger);

  bool dummy = false;
  uint32_t dummy_int = 0;
  int8_t line = 1;

  while (true) {
    char addr[16];
    int32_t port;
    int32_t rlength;
    struct timeval tv;
    char buffer[8192];

    int32_t size_read = bulk_file_reader.read(addr, 16);
    if (size_read < 16) break;

    size_read = bulk_file_reader.read(&port, 4);
    if (size_read < 4) break;

    size_read = bulk_file_reader.read(&rlength, 4);
    if (size_read < 4) break;

    size_read = bulk_file_reader.read(&tv, sizeof(struct timeval));
    if (size_read < (int32_t)sizeof(struct timeval)) break;

    size_read = bulk_file_reader.read(buffer, rlength);
    if (size_read < rlength) break;

    //    HFSAT::TMXOBF::TMXOBFFrameHeader * frame_header = (HFSAT::TMXOBF::TMXOBFFrameHeader*)buffer ;
    //    dbglogger << frame_header->ToString () ;

    //    std::cout << "IP : " << addr << " Port : " << port << " Length : " << rlength << " SeqNum : " <<
    //    ntoh32(*(uint32_t*)(buffer)) << " Line : " << (int32_t)(*(uint8_t*)(buffer+4)) << " NumMsg : " <<
    //    (int32_t)(*(uint8_t*)(buffer+5)) << " Length : " << (int32_t)ntoh16((*(uint16_t*)(buffer+6))) << std::endl ;

    //    decoder.DecodeUDPPacket(buffer + 8, rlength - 8, *(uint32_t *)(buffer), line, dummy, dummy_int);
  }

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
