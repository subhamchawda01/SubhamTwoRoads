#pragma once
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <nsmsg.h>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"

// Returns signed 64 bit integer after adding proper masks
#define NS_RETURN_SIGNED_INT64(size)                                                \
  static const uint64_t sigmask = uint64_t(1) << ((8 * size) - 1);                  \
  static const uint64_t negmask = ~uint64_t(0) ^ ((uint64_t(1) << (8 * size)) - 1); \
  static const uint64_t posmask = (uint64_t(1) << uint64_t(8 * size)) - 1;          \
  if (res & sigmask) {                                                              \
    res = static_cast<int64_t>(res | negmask);                                      \
  } else {                                                                          \
    res = static_cast<int64_t>(res & posmask);                                      \
  }

namespace HFSAT {
#define FPGA_BOOK_LEVEL_UPDATE 97
#define FPGA_EXECUTION_SUMMARY 38
#define FPGA_TIME_MESSAGE 64
#define FIRST_BIT(n) (n & 1) > 0 ? 1 : 0
#define SECOND_BIT(n) (n & 2) > 0 ? 1 : 0
#define DEF_CME_FPGA_REFLOC_ "/spare/local/files/CMEMDP/cme_instrument_dict.txt"
#define DEF_CME_REFLOC_ "/spare/local/files/CMEMDP/cme-ref.txt"
#define POWER_OFFSET 10
/*
 * Reader class for FPGA
 * (reads and parses FPGA messages)
 * Little Endian Order being followed
 */
// TODO: Add error handling
class FPGAReader {
  nsmsg_t* ctx;          // nsmsg context
  nsmsg_socket_t* sock;  // nsmsg socket
  nsmsg_msg_t msg;       // present nsmsg message
  int current_index_;
  char* data;
  int data_length;
  int ref_price;  // Used in case price reference-delta model
  int price_exponent;
  timeval current_time_;

  std::tr1::unordered_map<int64_t, char*> idmap_;  // TODO - make this a vector

 public:
  FPGAReader(const char* subject_to_subscribe_, const char* nsmsg_interface_);

  char* getSecurityName(int sec_id);

  void LoadCmeFPGARef();

  void handle_error_if_any(long int rc, char* situation);

  void cleanUp();

  void releaseMessage();

  // TODO: Optimize this function (using bitmasks)
  // Returns integer value corresponding to bits start->end in src
  /*int getBitsValue ( int src, int start, int end )
  {
    int fact, fact2, res=0;
    int index=start;
    fact=1<<index;
    fact2=1;
    while(index<=end){
        if(src&fact)res+=fact2;
        fact<<=1;
        fact2<<=1;
        index++;
    }
    return res;
  }*/

  // Returns unsigned long value corresponding to first sz bytes starting from ptr
  uint64_t getBytesValue(char* ptr, int sz);

  // Returns signed long value corresponding to first sz bytes starting from ptr
  int64_t getSignedBytesValue(char* ptr, int sz);

  // Read new message from FPGA socket
  int readMessage();

  // Returns security ID corresponding to the present read message
  int getSecurityId();

  // Returns pointer to the present message's data payload
  char* getMessageBody();

  // Returns present message type
  int getMessageType();

  // Checks if we have completely read the present message (read is in past tense)
  bool parsedMessageFully();

  bool parseTimeMessage();

  // returns true on success, otherwise false
  bool parseCMEBookLevelUpdate(TradeType_t& _buysell_, int& level_, double& price_, int& size_, int& num_ords_,
                               bool& intermediate_);

  // returns true on success, otherwise false
  bool parseCMEExecutionSummary(double& price_, int& size_, TradeType_t& _buysell_);

  timeval getCurrentTime();
};
}
