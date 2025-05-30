/**
   \file Tools/mds_fast_first_trade_read.cpp

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
#include <fstream>

#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/multi_shm_messages.hpp"

std::map<int32_t, NSE_MDS::NSETBTDataCommonStruct> map1;
std::map<int32_t, NSE_MDS::NSETBTDataCommonStruct> map2;

class MDSLogReader {
 public:
  static void ReadMDSStructsGeneric(HFSAT::BulkFileReader& bulk_file_reader_,
                                    HFSAT::BulkFileReader& bulk_file_reader2_) {
    HFSAT::MDS_MSG::GenericMDSMessage next_event_;
    std::cout << "Reading file 1" << std::endl;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
        map1[next_event_.generic_data_.nse_data_.msg_seq] = next_event_.generic_data_.nse_data_;
      }
      bulk_file_reader_.close();
    }
    std::cout << "Reading file 2" << std::endl;
    if (bulk_file_reader2_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader2_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
        map2[next_event_.generic_data_.nse_data_.msg_seq] = next_event_.generic_data_.nse_data_;
      }
      bulk_file_reader2_.close();
    }

    std::cout << "FIRSTFILEMAPSIZE " << map1.size() << std::endl;
    int compared = 0, msg_comp = 0, trade_comp = 0;

    std::cout << "SECONDFILEMAPSIZE " << map2.size() << std::endl;
    std::cout << "COMPARE" << std::endl;
    for (auto it : map1) {
      if (map1[it.first].msg_type == NSE_MDS::MsgType::kNSETradeExecutionRange) continue;
      if (map2.find(it.first) != map2.end()) {
        compared++;
        if (map1[it.first].token != map2[it.first].token) {
          std::cout << "Token mismatch " << map1[it.first].token << " " << map2[it.first].token << std::endl;
          continue;
        }
        if (map1[it.first].msg_type != map2[it.first].msg_type) {
          std::cout << "msg_type mismatch " << it.first << " "
                    << static_cast<std::underlying_type<NSE_MDS::MsgType>::type>(map1[it.first].msg_type) << " "
                    << static_cast<std::underlying_type<NSE_MDS::MsgType>::type>(map2[it.first].msg_type) << std::endl;
          //<< map1[it.first].msg_type << " " << map2[it.first].msg_type << std::endl;
          continue;
        }
        if (map1[it.first].activity_type != map2[it.first].activity_type) {
          std::cout << "activity_type mismatch " << map1[it.first].activity_type << " " << map2[it.first].activity_type
                    << std::endl;
          continue;
        }
        if (map1[it.first].segment_type != map2[it.first].segment_type) {
          std::cout << "segment_type mismatch " << map1[it.first].segment_type << " " << map2[it.first].segment_type
                    << std::endl;
          continue;
        }

        if (map1[it.first].msg_type == NSE_MDS::MsgType::kNSETrade ||
            NSE_MDS::MsgType::kNSESpreadTrade == map1[it.first].msg_type) {
          trade_comp++;
          if (map1[it.first].data.nse_trade.buy_order_id != map2[it.first].data.nse_trade.buy_order_id) {
            std::cout << "buy order mismatch " << std::endl;
            continue;
          }
          if (map1[it.first].data.nse_trade.sell_order_id != map2[it.first].data.nse_trade.sell_order_id) {
            std::cout << "sell order mismatch " << std::endl;
            continue;
          }
          if (map1[it.first].data.nse_trade.trade_price != map2[it.first].data.nse_trade.trade_price) {
            std::cout << "tradeprice order mismatch " << std::endl;
            continue;
          }
          if (map1[it.first].data.nse_trade.trade_qty != map2[it.first].data.nse_trade.trade_qty) {
            std::cout << "qty mismatch Trade " << it.first << " " << map1[it.first].data.nse_trade.trade_qty << " "
                      << map2[it.first].data.nse_trade.trade_qty << std::endl;
            continue;
          }
        }

        else if (map1[it.first].msg_type == NSE_MDS::MsgType::kNSEOrderDelta) {
          msg_comp++;
          if (map1[it.first].data.nse_order.order_id != map2[it.first].data.nse_order.order_id) {
            std::cout << "2nd orderID mismatch " << std::endl;
            continue;
          }
          if (map1[it.first].data.nse_order.order_price != map2[it.first].data.nse_order.order_price) {
            std::cout << "order PRICE mismatch " << std::endl;
            continue;
          }
          if (map1[it.first].data.nse_order.order_qty != map2[it.first].data.nse_order.order_qty) {
            std::cout << "qty mismatch ORDER" << std::endl;
            continue;
          }
          if (map1[it.first].data.nse_order.buysell != map2[it.first].data.nse_order.buysell) {
            std::cout << "buysell mismatch " << map1[it.first].data.nse_order.buysell << " "
                      << map2[it.first].data.nse_order.buysell << std::endl;
            continue;
          }
        } else {
          //			std::cout << "Should not come here " << std::endl;
        }
      }
    }
    std::cout << "TOTAL packets compared " << compared << " Tradecompare: " << trade_comp << " MSgcomapre " << msg_comp
              << std::endl;
  }
};

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cout << " USAGE: EXEC <exchange> <file_path> <file_path2>" << std::endl;
    exit(0);
  }
  std::string exch = argv[1];
  HFSAT::BulkFileReader reader, reader2;
  reader.open(argv[2]);
  reader2.open(argv[3]);
  if (exch == "GENERIC")
    MDSLogReader::ReadMDSStructsGeneric(reader, reader2);
  else
    std::cout << "Wrong exchange...!!" << exch << std::endl;
  return 0;
}
