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
#include <sys/stat.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/multi_shm_messages.hpp"


void GenericReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {

  HFSAT::MDS_MSG::GenericMDSMessage next_event_;
  int64_t trade_qty_ = 0;
  std::string symbol_ = "";
  if (bulk_file_reader_.is_open()) {
    while (true) {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
      if (available_len_ < sizeof(next_event_)) break;
      switch (next_event_.mds_msg_exch_) {
        case HFSAT::MDS_MSG::NSE: {
          switch(next_event_.generic_data_.nse_data_.msg_type) {
            case NSE_MDS::MsgType::kNSETrade: {
              trade_qty_ += next_event_.generic_data_.nse_data_.data.nse_trade.trade_qty;
              symbol_ = HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
                                      next_event_.generic_data_.nse_data_.token, next_event_.generic_data_.nse_data_.segment_type);
              break;
            }
            default: { } break;
          }
          break;
        }
        case HFSAT::MDS_MSG::BSE: {
          break;
        }
        default: { } break;
      }
    } 
  }
  bulk_file_reader_.close();
  std::cout << "SYMBOL: " << symbol_ << " TOTAL_TRADE_QTY: " << trade_qty_ << std::endl; 
}

void NSEReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {

  NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
  int64_t trade_qty_ = 0;
  std::string symbol_ = "";
  if (bulk_file_reader_.is_open()) {
    while (true) {
      size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
      if (available_len_ < sizeof(next_event_)) break;
        switch(next_event_.msg_type) {
          case NSE_MDS::MsgType::kNSETrade: {
            trade_qty_ += next_event_.data.nse_dotex_trade.trade_quantity;
            symbol_ = next_event_.symbol;
            break;
          }
          default: { } break;
        }
    }
  }
  bulk_file_reader_.close();
  std::cout << "SYMBOL: " << symbol_ << " TOTAL_TRADE_QTY: " << trade_qty_ << std::endl; 
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << " USAGE: EXEC <GENERIC/NSE> " << std::endl;
    exit(0);
  }
  std::string exch = argv[1];
  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);

  if (exch == "GENERIC")
    GenericReadMDSStructs(reader);
  else if (exch == "NSE")
    NSEReadMDSStructs(reader);
  else
    std::cout << "Wrong exchange...!!" << exch << std::endl;
  
  return 0;
}

