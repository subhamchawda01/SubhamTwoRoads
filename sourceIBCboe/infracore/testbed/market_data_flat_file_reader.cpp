// =====================================================================================
//
//       Filename:  market_data_flat_file_generator.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  03/20/2013 03:31:44 PM
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

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "infracore/MarketAdapter/eurex_price_level_market_view_manager.hpp"

enum ExchangeA_MsgType { NewLevel = 0, DeleteLevel, ModifyLevel };

struct ExchangeA_MD {
  uint16_t seqno_;
  char contract_[4];
  uint8_t level_;
  double price_;
  uint16_t size_;
  char side_;
  ExchangeA_MsgType msg_;

  std::string ToString() {
    std::ostringstream str_;

    str_ << "================= ExchangeA_MD ==========================\n";

    str_ << "Contract : " << contract_ << "\n";
    str_ << "Level    : " << (int)level_ << "\n";
    str_ << "Seq      : " << seqno_ << "\n";
    str_ << "Price    : " << price_ << "\n";
    str_ << "Size     : " << size_ << "\n";
    str_ << "Side     : " << side_ << "\n";
    str_ << "Type     : " << (int)msg_ << "\n\n\n";

    return str_.str();
  }
};

class MDSLogReader {
 public:
  static void ReadFlatFile(std::string filename_) {
    HFSAT::BulkFileReader file_reader_;

    file_reader_.open(filename_.c_str());

    while (true) {
      ExchangeA_MD this_msg_;

      memset(&(this_msg_), 0, sizeof(ExchangeA_MD));
      size_t read_size_ = file_reader_.read(&(this_msg_), sizeof(ExchangeA_MD));

      if (read_size_ < sizeof(ExchangeA_MD)) break;

      std::cout << this_msg_.ToString();
    }

    file_reader_.close();
  }
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << " USAGE: EXEC <file_path> " << std::endl;
    exit(0);
  }

  MDSLogReader::ReadFlatFile(argv[1]);

  return 0;
}
