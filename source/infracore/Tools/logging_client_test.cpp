// =====================================================================================
//
//       Filename:  loggin_client_test.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/31/2014 01:43:41 PM
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
#include <string>
#include <sstream>
#include <signal.h>
#include <ctime>
#include <sys/syscall.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"

HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_ptr = NULL;

void termination_handler(int signum) {
  if (NULL != client_logging_segment_initializer_ptr) {
    client_logging_segment_initializer_ptr->CleanUp();
  }

  exit(-1);
}

int main(int argc, char** argv) {
  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGKILL, termination_handler);

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  dbglogger_.OpenLogFile("/home/ravi/test", std::ofstream::out);

  HFSAT::BulkFileWriter trades_writer_(256 * 1024 * 1024);
  trades_writer_.Open("/home/dvcinfra/testtrades1.log", std::ios::app);

  HFSAT::Utils::ClientLoggingSegmentInitializer client_logging_segment_initializer(dbglogger_, 40011, "/home/ravi/",
                                                                                   argv[1]);
  client_logging_segment_initializer_ptr = &client_logging_segment_initializer;

  int threadId = getpid();
  CPUManager::setAffinity(4, threadId);

  struct timeval start_time;
  struct timeval end_time;

  HFSAT::CDef::LogBuffer log_buffer;
  memset((void*)&log_buffer, 0, sizeof(HFSAT::CDef::LogBuffer));

  log_buffer.content_type_ = HFSAT::CDef::QueryTrade;
  log_buffer.buffer_data_.query_trade_.watch_tv_sec_ = 1407355500;
  log_buffer.buffer_data_.query_trade_.watch_tv_usec_ = 572778;
  memcpy((void*)log_buffer.buffer_data_.query_trade_.security_name_, "ESM4", 4);
  log_buffer.buffer_data_.query_trade_.trade_size_ = 1;
  log_buffer.buffer_data_.query_trade_.trade_price_ = 191700.0000000;
  log_buffer.buffer_data_.query_trade_.new_position_ = 0;
  log_buffer.buffer_data_.query_trade_.open_unrealized_pnl_ = 0;
  log_buffer.buffer_data_.query_trade_.total_pnl_ = 450;
  log_buffer.buffer_data_.query_trade_.bestbid_size_ = 30;
  log_buffer.buffer_data_.query_trade_.bestbid_price_ = 191700;
  log_buffer.buffer_data_.query_trade_.bestask_price_ = 191725;
  log_buffer.buffer_data_.query_trade_.bestask_size_ = 40;
  log_buffer.buffer_data_.query_trade_.mult_risk_ = 0;
  log_buffer.buffer_data_.query_trade_.mult_base_pnl_ = 0;
  log_buffer.buffer_data_.query_trade_.open_or_flat_ = 'F';
  log_buffer.buffer_data_.query_trade_.trade_type_ = 'B';

  HFSAT::CDef::LogBuffer ors_log_buffer;
  memset((void*)&ors_log_buffer, 0, sizeof(HFSAT::CDef::LogBuffer));

  ors_log_buffer.content_type_ = HFSAT::CDef::ORSTrade;
  memcpy((void*)ors_log_buffer.buffer_data_.ors_trade_.symbol_, "ESM4", 4);
  ors_log_buffer.buffer_data_.ors_trade_.trade_type_ = 0;
  ors_log_buffer.buffer_data_.ors_trade_.size_executed_ = 1;
  ors_log_buffer.buffer_data_.ors_trade_.price_ = 191700.0000000;

  std::string temp = "";

  gettimeofday(&start_time, NULL);

  for (int i = 0; i < 10000; i++) {
    ors_log_buffer.buffer_data_.ors_trade_.saos_ = i;
    //    log_buffer.buffer_data_.query_trade_.mult_risk_ = i ;
    client_logging_segment_initializer.Log(&ors_log_buffer);
    HFSAT::usleep(10);

    //    temp = log_buffer.ToString () ;
    //    trades_writer_ << log_buffer.ToString () << "\n" ;
    //    trades_writer_.CheckToFlushBuffer () ;
  }

  gettimeofday(&end_time, NULL);

  trades_writer_.Close();

  std::cout << " TIME TAKEN : "
            << (end_time.tv_sec * 1000000 + end_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec)
            << "\n";

  //  while ( true ) {
  //
  //    client_logging_segment_initializer.Log ( & log_buffer ) ;
  //
  //    sleep ( 1 ) ;
  //
  //  }

  return 0;
}
