// =====================================================================================
//
//       Filename:  fpga_data_logger.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/30/2014 12:39:20 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

// This exec reads data from FPGA and directly logs into files in binary format.
// Very similar to mktDD which logs data reading from multicast over UDP

#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>

#include "baseinfra/FPGA/fpga_reader.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

int main(int argc, char* argv[]) {
  // create an instance of fpga reader
  HFSAT::FPGAReader* fpga_reader_ = new HFSAT::FPGAReader("*", "pcie://pcie0");

  // stores cme fpga delta and trade msgs
  CME_MDS::CMEFPGACommonStruct fpga_half_book_;

  fpga_half_book_.data_.cme_dels_[0].type_ = '0';  // 0-> bid
  fpga_half_book_.data_.cme_dels_[1].type_ = '1';  //  1->ask

  // create a logger thread and run it
  MDSLogger<CME_MDS::CMEFPGACommonStruct>* mdsLogger =
      new MDSLogger<CME_MDS::CMEFPGACommonStruct>("CME_FPGA");  // log data in /spare/local/MDSlogs/CME_FPGA/
  mdsLogger->run();                                             // start logger thread

  int result, security_id_, level_, size_, num_ords_, msg_type_;
  bool intermediate_;
  double price_;
  HFSAT::TradeType_t _buysell_;
  bool new_update_bid_ = false, new_update_ask_ = false;
  std::cout << "Reading fpga msgs now: " << std::endl;

  // Read msgs from FPGA throughout the day
  while (true) {
    // Make FPGA API call and process messages
    result = fpga_reader_->readMessage();
    if (result <= 0) {  // No data
      continue;         // data not available at this instance, will retry again
    }

    // security id as provided in fpga messages
    security_id_ = fpga_reader_->getSecurityId();

    char* sec_name = NULL;
    // ignore this message if it corresponds to random security
    if ((security_id_ <= 0) ||
        ((sec_name = fpga_reader_->getSecurityName(security_id_)) == NULL))  // Check if ref for security id available
    {
      // Security not found..not sure why.
      fpga_reader_->releaseMessage();  // Deallocate message
      continue;
    }

    // denotes if its book update or delta msg
    msg_type_ = fpga_reader_->getMessageType();

    // copy the security name
    strcpy(fpga_half_book_.contract_, sec_name);
    gettimeofday(&(fpga_half_book_.time_), NULL);

    // book update received
    if (msg_type_ == FPGA_BOOK_LEVEL_UPDATE) {
      fpga_half_book_.msg_ = CME_MDS::CME_DELTA;
      // Resetting values on both sides of book
      new_update_bid_ = new_update_ask_ = false;
      for (int ctr = 0; ctr < 5; ctr++) {
        fpga_half_book_.data_.cme_dels_[0].num_orders_[ctr] = 0;
        fpga_half_book_.data_.cme_dels_[0].sizes_[ctr] = -1;
        fpga_half_book_.data_.cme_dels_[0].prices_[ctr] = 0;

        fpga_half_book_.data_.cme_dels_[1].num_orders_[ctr] = 0;
        fpga_half_book_.data_.cme_dels_[1].sizes_[ctr] = -1;
        fpga_half_book_.data_.cme_dels_[1].prices_[ctr] = 0;
      }

      // process messages packet by packet unless entire message is parsed
      while (!fpga_reader_->parsedMessageFully()) {
        // parse a particular book update
        if (fpga_reader_->parseCMEBookLevelUpdate(_buysell_, level_, price_, size_, num_ords_, intermediate_)) {
          if (level_ >= 5) continue;  // not sure why >5 level updates are not needed
          if (_buysell_ == HFSAT::kTradeTypeBuy) {
            new_update_bid_ = true;
            fpga_half_book_.data_.cme_dels_[0].num_orders_[level_] = num_ords_;
            fpga_half_book_.data_.cme_dels_[0].sizes_[level_] = size_;
            fpga_half_book_.data_.cme_dels_[0].prices_[level_] = price_;
          } else {
            new_update_ask_ = true;
            fpga_half_book_.data_.cme_dels_[1].num_orders_[level_] = num_ords_;
            fpga_half_book_.data_.cme_dels_[1].sizes_[level_] = size_;
            fpga_half_book_.data_.cme_dels_[1].prices_[level_] = price_;
          }
        }
      }
      // message is parsed and structs are populated
      // log the message into file
      if (new_update_bid_ || new_update_ask_) {
        mdsLogger->log(fpga_half_book_);
      }
    }
    // received a trade msg
    else if (msg_type_ == FPGA_EXECUTION_SUMMARY) {
      fpga_half_book_.msg_ = CME_MDS::CME_TRADE;
      if (fpga_reader_->parseCMEExecutionSummary(price_, size_, _buysell_)) {
        fpga_half_book_.data_.cme_trds_.price_ = price_;
        fpga_half_book_.data_.cme_trds_.size_ = size_;
        fpga_half_book_.data_.cme_trds_.type_ = (_buysell_ == HFSAT::kTradeTypeBuy) ? '0' : '1';

        // message is parsed and structs are populated
        // log the message into file
        mdsLogger->log(fpga_half_book_);
      }
    }
    fpga_reader_->releaseMessage();  // Deallocate message
  }
  return 0;
}
