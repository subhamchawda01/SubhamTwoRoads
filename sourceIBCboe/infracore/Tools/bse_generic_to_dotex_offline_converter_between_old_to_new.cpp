// =====================================================================================
//
//       Filename:  bse_generic_to_dotex_offline_converter.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/07/2015 08:55:25 AM
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/CDef/mds_messages.hpp"

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage : " << argv[0] << " < InputFile >"
              << " < Date >"
              << " < OUTPUT FILE>" << std::endl;
    exit(-1);
  }
  HFSAT::BulkFileReader* bulk_file_reader = new HFSAT::BulkFileReader();
  bulk_file_reader->open(argv[1]);
  HFSAT::BulkFileWriter bulk_file_writer;
  bulk_file_writer.Open(argv[3]);

  // allocate memory to result struct once
  if (!bulk_file_reader->is_open()) {
    std::cerr << "Failed To Open DataFile : " << argv[1] << " For Reading" << std::endl;
    exit(-1);
  } else {
    while (true) {
      // expect generic logged data as input which has BSE struct
      EOBI_MDS::EOBICommonStructOld_v2 generic_msg;
      size_t available_len_ = bulk_file_reader->read(&generic_msg, sizeof(generic_msg));
      if (available_len_ < sizeof(generic_msg)) {  // check if reading complete structs
        std::cerr << "Incorrect len read: expected " << sizeof(generic_msg) << " read len: " << available_len_
                  << std::endl;
        break;
      }
      // std::cout << "GENERIC 1" << generic_msg.ToString()<< std::endl;
      EOBI_MDS::EOBICommonStruct* next_event_ = new EOBI_MDS::EOBICommonStruct();;
      next_event_->source_time = generic_msg.source_time;
      next_event_->token_ = generic_msg.token_;
      next_event_->msg_ = generic_msg.msg_;
      next_event_->order_.msg_seq_num_ = generic_msg.data_.order_.msg_seq_num_;
      next_event_->order_.side = generic_msg.data_.order_.side;
      next_event_->order_.priority_ts = generic_msg.data_.order_.priority_ts;
      next_event_->order_.action_ = generic_msg.data_.order_.action_;
      next_event_->order_.price = generic_msg.data_.order_.price;
      next_event_->order_.size = generic_msg.data_.order_.size;
      next_event_->order_.prev_price = generic_msg.data_.order_.prev_price;
      next_event_->order_.prev_size = generic_msg.data_.order_.prev_size;
      next_event_->order_.prev_priority_ts = generic_msg.data_.order_.prev_priority_ts;
      next_event_->order_.intermediate_ = generic_msg.data_.order_.intermediate_;
      next_event_->order_.trd_reg_ts = generic_msg.data_.order_.trd_reg_ts;
      next_event_->segment_type = generic_msg.segment_type;
      // std::cout << "TMPGENERIC 2" << next_event_->ToString() << std::endl;
      bulk_file_writer.Write(next_event_, sizeof(EOBI_MDS::EOBICommonStruct));
      bulk_file_writer.CheckToFlushBuffer();
    }
    // always close the writer, the buffered data need to be flushed
    bulk_file_reader->close();
    bulk_file_writer.Close();
  }
  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
