#include <stdio.h>
#include <iostream>

#include "dvccode/CommonDataStructures/char16_gen_map.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " data_filename   data_out_directory  tradingdate_YYYYMMDD" << std::endl;
    exit(0);
  }

  std::string data_filename_ = argv[1];
  std::string data_out_directory_ = argv[2];
  data_out_directory_ += "/";

  int tradingdate_ = atoi(argv[3]);

  // get timeval corresponding to tradingdate_ midnight ( UTC )
  HFSAT::ttime_t base_tv_(HFSAT::DateTime::GetTimeMidnightUTC(tradingdate_), 0);

  // Open data_filename_ with BulkFileReader
  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(data_filename_);

  CME_MDS::CMECommonStruct next_event_;

  char char16_symbol_[16] = {0};
  HFSAT::Char16GenMap<HFSAT::BulkFileWriter*> symbol_to_writer_map_;

  while (bulk_file_reader_.is_open()) {
    // read every event in the file
    bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));

    // add base_tv_ to it's timeval
    next_event_.time_.tv_sec += base_tv_.tv_sec;
    next_event_.time_.tv_usec += base_tv_.tv_usec;

    // write to data_out_filename_
    switch (next_event_.msg_) {
      case CME_MDS::CME_DELTA: {
        memcpy(char16_symbol_, next_event_.data_.cme_dels_.contract_, CME_MDS_CONTRACT_TEXT_SIZE);

        HFSAT::BulkFileWriter* p_bulk_file_writer_ = NULL;
        bool is_there = symbol_to_writer_map_.GetIdFromChar16(char16_symbol_, p_bulk_file_writer_);
        if (!is_there) {  // first time we are seeing this symbol;
          std::string data_out_filename_ = data_out_directory_ + char16_symbol_;

          data_out_filename_ += "_";
          {
            std::stringstream ss;
            ss << tradingdate_;
            data_out_filename_ += ss.str();
          }

          p_bulk_file_writer_ = new HFSAT::BulkFileWriter(data_out_filename_);
          symbol_to_writer_map_.AddString(char16_symbol_, p_bulk_file_writer_);
        }

        p_bulk_file_writer_->Write(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        p_bulk_file_writer_->CheckToFlushBuffer();

      } break;
      case CME_MDS::CME_TRADE: {
        memcpy(char16_symbol_, next_event_.data_.cme_trds_.contract_, CME_MDS_CONTRACT_TEXT_SIZE);

        HFSAT::BulkFileWriter* p_bulk_file_writer_ = NULL;
        bool is_there = symbol_to_writer_map_.GetIdFromChar16(char16_symbol_, p_bulk_file_writer_);
        if (!is_there) {  // first time we are seeing this symbol;
          std::string data_out_filename_ = data_out_directory_ + char16_symbol_;
          data_out_filename_ += "_";
          {
            std::stringstream ss;
            ss << tradingdate_;
            data_out_filename_ += ss.str();
          }

          p_bulk_file_writer_ = new HFSAT::BulkFileWriter(data_out_filename_);
          symbol_to_writer_map_.AddString(char16_symbol_, p_bulk_file_writer_);
        }

        p_bulk_file_writer_->Write(&next_event_, sizeof(CME_MDS::CMECommonStruct));
        p_bulk_file_writer_->CheckToFlushBuffer();

      } break;
      default: { } break; }
  }

  bulk_file_reader_.close();

  const std::vector<const char*> keys_vec_ = symbol_to_writer_map_.GetKeys();
  for (auto i = 0u; i < keys_vec_.size(); i++) {
    HFSAT::BulkFileWriter** _temp_p_ = symbol_to_writer_map_.GetIdRefFromChar16(keys_vec_[i]);
    if (_temp_p_ != NULL) {
      (*_temp_p_)->Close();
      delete (*_temp_p_);
      *_temp_p_ = NULL;
    }
  }

  return 0;
}
