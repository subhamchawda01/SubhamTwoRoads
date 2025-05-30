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

    str_ << "===========================================\n";

    str_ << "Contract : " << contract_ << "\n";
    str_ << "Level : " << (int)level_ << "\n";
    str_ << "Seq : " << seqno_ << "\n";
    str_ << "Price : " << price_ << "\n";
    str_ << "Size : " << size_ << "\n";
    str_ << "Side : " << side_ << "\n";
    str_ << "Type : " << (int)msg_ << "\n";

    str_ << "==========================================\n\n\n";

    return str_.str();
  }
};

class MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
    EUREX_MDS::EUREXCommonStruct next_event_;

    HFSAT::BulkFileWriter file_writer_;

    file_writer_.Open("/home/ravi/ExchangeA_MD.bin");

    if (!file_writer_.is_open()) {
      std::cerr << " Can't open output file to write \n";
    }

    unsigned int seq_ = 0;

    double normalize_price_ = 0.0;

    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;

        if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) continue;

        if (next_event_.data_.eurex_dels_.level_ > 10 || next_event_.data_.eurex_dels_.level_ < 1) continue;

        if (normalize_price_ == 0) {
          normalize_price_ = next_event_.data_.eurex_dels_.price_;
        }

        ExchangeA_MD this_msg_;

        seq_++;

        if (seq_ > 1000) {
          file_writer_.DumpCurrentBuffer();
          file_writer_.Close();
          break;
        }

        memset(&(this_msg_), 0, sizeof(ExchangeA_MD));
        strcpy(this_msg_.contract_, "Y");

        int price_diff_ = (int)((next_event_.data_.eurex_dels_.price_ * 100 - normalize_price_ * 100));

        this_msg_.price_ = 100 + (price_diff_);

        //            this_msg_.price_ = next_event_.data_.eurex_dels_.price_;
        this_msg_.size_ = next_event_.data_.eurex_dels_.size_;
        this_msg_.level_ = (int)next_event_.data_.eurex_dels_.level_;
        this_msg_.side_ = (next_event_.data_.eurex_dels_.type_ == '1') ? 'S' : 'B';
        this_msg_.seqno_ = seq_;

        switch (next_event_.data_.eurex_dels_.action_) {
          case '1': {
            this_msg_.msg_ = NewLevel;

          } break;

          case '2': {
            this_msg_.msg_ = ModifyLevel;

          } break;

          case '3': {
            this_msg_.msg_ = DeleteLevel;

          } break;

          default: { } break; }

        if (seq_ == 140 || seq_ == 170 || seq_ == 232 || seq_ == 431 || seq_ == 467 || seq_ == 561 || seq_ == 754 ||
            seq_ == 876 || seq_ == 973 || seq_ == 974) {
          strcpy(this_msg_.contract_, "ZZ");
          this_msg_.price_ = 120 + (price_diff_);
          this_msg_.msg_ = NewLevel;
        }

        file_writer_.Write(&(this_msg_), sizeof(ExchangeA_MD));
        file_writer_.CheckToFlushBuffer();
      }

      bulk_file_reader_.close();
    }

    file_writer_.Close();
  }

  static void ReadFlatFile() {
    HFSAT::BulkFileReader file_reader_;

    file_reader_.open("/home/ravi/ExchangeA_MD.bin");

    while (true) {
      ExchangeA_MD this_msg_;

      memset(&(this_msg_), 0, sizeof(ExchangeA_MD));
      size_t read_size_ = file_reader_.read(&(this_msg_), sizeof(ExchangeA_MD));

      if (read_size_ < sizeof(ExchangeA_MD)) break;

      std::cout << this_msg_.ToString();
    }

    file_reader_.close();
  }

  static void ConvertFlatBackToEUREX() {
    HFSAT::BulkFileReader file_reader_;
    HFSAT::BulkFileWriter exch_file_writer_;

    file_reader_.open("/home/ravi/ExchangeA_MD.bin");
    exch_file_writer_.Open("/home/ravi/exchMD.bin");

    while (true) {
      ExchangeA_MD this_msg_;

      memset(&(this_msg_), 0, sizeof(ExchangeA_MD));
      size_t read_size_ = file_reader_.read(&(this_msg_), sizeof(ExchangeA_MD));

      if (read_size_ < sizeof(ExchangeA_MD)) break;

      EUREX_MDS::EUREXCommonStruct exch_common_struct_;
      memset(&(exch_common_struct_), 0, sizeof(EUREX_MDS::EUREXCommonStruct));

      exch_common_struct_.msg_ = EUREX_MDS::EUREX_DELTA;
      strcpy(exch_common_struct_.data_.eurex_dels_.contract_, "Y");
      gettimeofday(&(exch_common_struct_.time_), NULL);
      exch_common_struct_.data_.eurex_dels_.price_ = this_msg_.price_;
      exch_common_struct_.data_.eurex_dels_.size_ = this_msg_.size_;
      exch_common_struct_.data_.eurex_dels_.type_ = (this_msg_.side_ == 'B') ? '2' : '1';
      exch_common_struct_.data_.eurex_dels_.level_ = (this_msg_.level_);

      if (this_msg_.msg_ == 0) exch_common_struct_.data_.eurex_dels_.action_ = '1';

      if (this_msg_.msg_ == 1) exch_common_struct_.data_.eurex_dels_.action_ = '3';

      if (this_msg_.msg_ == 2) exch_common_struct_.data_.eurex_dels_.action_ = '2';

      exch_file_writer_.Write(&(exch_common_struct_), sizeof(EUREX_MDS::EUREXCommonStruct));
      exch_file_writer_.CheckToFlushBuffer();
    }

    file_reader_.close();
    exch_file_writer_.Close();
  }
};

int main(int argc, char** argv) {
  if (argc != 3 && argc != 5) {
    std::cout
        << " USAGE: EXEC <exchange> <file_path> <start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec>"
        << std::endl;
    exit(0);
  }

  std::string exch = argv[1];

  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);

  MDSLogReader::ReadMDSStructs(reader);
  //  MDSLogReader::ReadFlatFile () ;
  MDSLogReader::ConvertFlatBackToEUREX();

  return 0;
}
