// =====================================================================================
//
//       Filename:  tmx_obf_live_test.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/05/2016 10:40:44 AM
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
#include <string>
#include "infracore/DataDecoders/TMXOBF/tmx_obf_defines.hpp"
#include "infracore/DataDecoders/TMXOBF/tmx_obf_decoder.hpp"
#include "infracore/DataDecoders/TMXOBF/tmx_obf_raw_live_data_source.hpp"

using namespace HFSAT::TMXOBF;

int main(int argc, char *argv[]) {
  //  HFSAT::TCPClientSocket * tcp_recovery_client_socket_ = new HFSAT::TCPClientSocket();
  //  tcp_recovery_client_socket_->Connect("142.201.223.8", 12070);
  ////  tcp_recovery_client_socket_->SetNonBlocking() ;
  //
  //  //Fill up the snapshot request struct with the required line-id
  //  ReplayRequest replay_request ;
  //  replay_request.line_id = 1 ;
  //  replay_request.first_msg_seq_num = 1 ;
  //  replay_request.last_msg_seq_num = 20016 ;
  //
  //  //Each request requires a header
  //  TMXOBFMessageHeader tmx_obf_msg_header;
  //  tmx_obf_msg_header.msg_type = 21586 ;
  //  int32_t total_msg_length = sizeof(ReplayRequest) + sizeof(TMXOBFMessageHeader);
  //  tmx_obf_msg_header.msg_length = hton16(total_msg_length);
  //
  //  char * send_buffer = new char[total_msg_length];
  //  memcpy((void*)send_buffer, (void*)&tmx_obf_msg_header, sizeof(TMXOBFMessageHeader));
  //  memcpy((void*)(send_buffer+sizeof(TMXOBFMessageHeader)), (void*)&replay_request, sizeof(ReplayRequest));
  //
  //  int32_t written_length = tcp_recovery_client_socket_->WriteN(total_msg_length,send_buffer);
  //
  //  std::cout << "Wrote : " << written_length << std::endl ;
  //
  //  char buffer [ 8192 ] ;
  //  int32_t read_length = tcp_recovery_client_socket_->ReadN(8192, buffer);
  //
  //  std::cout << "Read : " << read_length << " SysError : " << strerror(errno) << std::endl ;

  HFSAT::DebugLogger dbglogger(10240);
  dbglogger.OpenLogFile("/home/dvcinfra/decoder.log", std::ofstream::out);

  HFSAT::SimpleLiveDispatcher sld;
  HFSAT::TMXOBF::TMXOBFRawLiveDataSource tmx_obf_raw_live_data_source(dbglogger, &sld, HFSAT::kLogger);

  sld.RunLive();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
