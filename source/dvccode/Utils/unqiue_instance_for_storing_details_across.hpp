// =====================================================================================
// 
//       Filename:  unqiue_instance_for_storing_details_across.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  01/18/2023 08:52:15 AM
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
#pragma once

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <signal.h>

namespace HFSAT {

class DetailsDumping {
 int token;
 long long msg_seq;
 timespec timeval;
 public:
  static DetailsDumping& GetUniqueInstance();
  void updateToken(int token_){ token = token_;}
  void updateSeq(long long msg_seq_) {msg_seq = msg_seq_; }
  void updateTime(timespec timval_) {timeval = timval_;}
  
  int getToken(){return token;}
  long long getSeq(){return msg_seq;}
  timespec getTime(){return timeval;}

 private:
  DetailsDumping();
  ~DetailsDumping();


  static DetailsDumping* unique_ptr;
};
}

