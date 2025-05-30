// =====================================================================================
//
//       Filename:  offline_ose_itch_orderlvl_to_pricefeed_convertor.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/26/2013 11:30:32 AM
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
#include <stdlib.h>
#include <map>
#include <vector>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "infracore/DataDecoders/OSE/indexed_ose_order_book.hpp"
#include "infracore/DataDecoders/OSE/ose_converted_pf_listener.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#define DEBUG 0

namespace HFSAT {

class PriceFeedHandler : public OSEConvertedPFListener {
 public:
  BulkFileWriter bulk_file_writer_;

  PriceFeedHandler(std::string file_name_) {
    bulk_file_writer_.Open(file_name_);

    if (!bulk_file_writer_.is_open()) {
      std::cerr << " Could not open PriceFeed Output File : " << file_name_ << "\n";
      exit(-1);
    }
  }

  virtual void ProcessMarketUpdate(OSE_ITCH_MDS::OSEPFCommonStruct* ose_mds) {
    bulk_file_writer_.Write(ose_mds, sizeof(OSE_ITCH_MDS::OSEPFCommonStruct));
    bulk_file_writer_.CheckToFlushBuffer();
  }
};
}

void MakeOSEItchOrderBookForPFNew(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                                  std::string outfile_price_feed);

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << " Usage < exec > < order_lvl_data > < price-feed file > <trading-date> <shortcode>\n";
    exit(-1);
  }

  MakeOSEItchOrderBookForPFNew(argv[3], argv[4], argv[1], argv[2]);

  return 0;
}

std::string GetRefFilePath(std::string trading_date) {
  std::string file_name = std::string("/spare/local/files/OSE/ose-itch-ref_") + trading_date + std::string(".txt");
  return file_name;
}

double GetMinPxInc(std::string exch_symbol_, std::string trading_date) {
  std::fstream file(GetRefFilePath(trading_date), std::ofstream::in);
  if (!file || !file.is_open()) {
    fprintf(stderr, "Could not open file %s in OSE_OF_TO_PF_Converter::GetMinPxInc ",
            GetRefFilePath(trading_date).c_str());
    exit(-1);
  }

  while (file.good()) {
    char line[1024];
    file.getline(line, sizeof(line));

    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();

    if (tokens.size() < 1) continue;

    if (std::string(tokens[0]).find("#") != std::string::npos) continue;

    if (tokens.size() >= 2) {
      std::string symbol = tokens[0];

      if (symbol == exch_symbol_) {
        double min_px_inc = atof(tokens[2]);
        return min_px_inc;
      }

    } else {
      std::cerr << "Malformatted line in OSE Parese Ref File NumTokens: " << tokens.size() << " Expected: 3\n";
    }
  }
  file.close();
  return 0;
}

void MakeOSEItchOrderBookForPFNew(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                                  std::string outfile_price_feed) {
  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(infile_order_feed);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open OrderLevelData File : " << infile_order_feed << "\n";
    exit(-1);
  }

  size_t MDS_SIZE_ = sizeof(OSE_ITCH_MDS::OSECommonStruct);
  OSE_ITCH_MDS::OSECommonStruct event, next_event;
  size_t available_len_ = bulk_file_reader_.read(&event, MDS_SIZE_);

  std::string this_exch_symbol_(event.contract);

  double min_px_increment_ = GetMinPxInc(this_exch_symbol_, trading_date);
  std::cout << "Min Px Inc : " << min_px_increment_ << "\n";

  HFSAT::OSEOrderBookForPF ose_order_book_(this_exch_symbol_, min_px_increment_);
  HFSAT::PriceFeedHandler price_feed_handler_(outfile_price_feed);
  ose_order_book_.AddPriceFeedListener(&price_feed_handler_);

  while (true) {
    if (available_len_ < MDS_SIZE_) break;
    available_len_ = bulk_file_reader_.read(&next_event, MDS_SIZE_);

    // ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.dat);

    switch (next_event.msg_type) {
      // Order Add
      case OSE_ITCH_MDS::kOSEAdd: {
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.add.order_id, false,
                                     next_event.data.add.price, next_event.data.add.size, next_event.data.add.side, 0,
                                     0, next_event.data.add.priority);
        break;
      }

      // Order delete
      case OSE_ITCH_MDS::kOSEDelete: {
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.del.order_id, false, 0, 0,
                                     next_event.data.del.side, 0, 0, next_event.data.add.priority);
        break;
      }

      // Order Exec
      case OSE_ITCH_MDS::kOSEExec: {
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.exec.order_id, false, 0,
                                     next_event.data.exec.size_exec, next_event.data.exec.side,
                                     next_event.data.exec.size_exec, 0, 0);
        break;
      }

      // Order Exec with price data
      case OSE_ITCH_MDS::kOSEExecWithTrade: {
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.exec_info.order_id, false,
                                     next_event.data.exec_info.price, next_event.data.exec_info.size_exec,
                                     next_event.data.exec_info.side, next_event.data.exec_info.size_exec, 0, 0);
        break;
      }

      case OSE_ITCH_MDS::kOSETradingStatus: {
        std::string status(next_event.data.status.state);
        char state;
        if (status.find("ZARABA") == std::string::npos) {
          state = 'C';
        } else {
          state = 'O';
        }
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.exec_info.order_id, false,
                                     next_event.data.exec_info.price, next_event.data.exec_info.size_exec,
                                     next_event.data.exec_info.side, next_event.data.exec_info.size_exec, state, 0);
        break;
      }

      case OSE_ITCH_MDS::kOSEResetBegin: {
        // Only next_event.msg_type matters
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.exec_info.order_id, false,
                                     next_event.data.exec_info.price, next_event.data.exec_info.size_exec,
                                     next_event.data.exec_info.side, next_event.data.exec_info.size_exec, 0, 0);
      } break;
      case OSE_ITCH_MDS::kOSEResetEnd: {
        // Only next_event.msg_type matters
        ose_order_book_.ProcessOrder(next_event.time_, next_event.msg_type, next_event.data.exec_info.order_id, false,
                                     next_event.data.exec_info.price, next_event.data.exec_info.size_exec,
                                     next_event.data.exec_info.side, next_event.data.exec_info.size_exec, 0, 0);
      } break;

      default: { break; }
    }

    event = next_event;
  }

  bulk_file_reader_.close();
}
