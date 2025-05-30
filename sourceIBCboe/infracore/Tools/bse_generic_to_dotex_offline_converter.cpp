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
      HFSAT::MDS_MSG::GenericMDSMessage generic_msg;
      size_t available_len_ = bulk_file_reader->read(&generic_msg, sizeof(generic_msg));
      if (available_len_ < sizeof(generic_msg)) {  // check if reading complete structs
        std::cerr << "Incorrect len read: expected " << sizeof(generic_msg) << " read len: " << available_len_
                  << std::endl;
        break;
      }
      EOBI_MDS::EOBICommonStruct* next_event_;
      next_event_ = &generic_msg.generic_data_.bse_data_;

      bulk_file_writer.Write(next_event_, sizeof(EOBI_MDS::EOBICommonStruct));
      bulk_file_writer.CheckToFlushBuffer();
    }
    // always close the writer, the buffered data need to be flushed
    bulk_file_reader->close();
    bulk_file_writer.Close();
  }
  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
