// =====================================================================================
//
//       Filename:  csm_spreads_symbol_converter.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/22/2014 06:49:05 AM
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
#include "infracore/CSMD/csm_reference_data_defines.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << " USAGE: EXEC <in_file_path> date " << std::endl;
    exit(0);
  }

  HFSAT::BulkFileReader* bulk_file_reader_ = new HFSAT::BulkFileReader();
  bulk_file_reader_->open(argv[1]);

  HFSAT::BulkFileWriter* bulk_file_writer_ = NULL;
  //  HFSAT::BulkFileWriter * bulk_file_writer_ = new HFSAT::BulkFileWriter () ;
  //  bulk_file_writer_->Open( argv[2], std::ofstream::binary | std::ofstream::app | std::ofstream::ate );
  //
  if (bulk_file_reader_->is_open()) {
    CSM_MDS::CSMCommonStruct next_event_;
    CSM_MDS::CSMCommonStruct new_event_;

    while (true) {
      size_t available_length_ = bulk_file_reader_->read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));

      if (available_length_ < sizeof(next_event_)) break;

      // copy next event to new struct
      memset(&new_event_, 0, sizeof(CSM_MDS::CSMCommonStruct));
      memcpy((void*)&(new_event_), (void*)&(next_event_), sizeof(CSM_MDS::CSMCommonStruct));

      std::ostringstream new_name;
      std::string symbol_name = new_event_.contract_;

      while (symbol_name.find("_") != std::string::npos) {
        std::string name = symbol_name.substr(0, symbol_name.find("_"));
        std::string expiry_str = name.substr(2);

        const char month_code = HFSAT::CSMD::GetExpiryMonthCode(expiry_str.substr(4, 2));
        std::string year_code = expiry_str.substr(3, 1);
        new_name << "VX" << month_code << year_code << "_";

        symbol_name = symbol_name.substr(symbol_name.find("_") + 1);
      }

      std::string name = symbol_name;
      std::string expiry_str = name.substr(2);

      const char month_code = HFSAT::CSMD::GetExpiryMonthCode(expiry_str.substr(4, 2));
      std::string year_code = expiry_str.substr(3, 1);
      new_name << "VX" << month_code << year_code;

      if (!bulk_file_writer_) {
        std::string filename = new_name.str() + std::string("_") + std::string(argv[2]);

        bulk_file_writer_ = new HFSAT::BulkFileWriter();
        bulk_file_writer_->Open(filename.c_str(), std::ofstream::binary | std::ofstream::app | std::ofstream::ate);

        if (!bulk_file_writer_->is_open()) {
          std::cerr << " Failed To OPen File for Writing : " << new_name.str() << "\n";
          break;
        }
      }

      memset((void*)(new_event_.contract_), 0, 32);
      memcpy((void*)(new_event_.contract_), (void*)(new_name.str().c_str()), new_name.str().length());

      bulk_file_writer_->Write(&(new_event_), sizeof(CSM_MDS::CSMCommonStruct));
      bulk_file_writer_->CheckToFlushBuffer();
    }
  }

  if (bulk_file_reader_) {
    bulk_file_reader_->close();
    delete bulk_file_reader_;
    bulk_file_reader_ = NULL;
  }

  if (bulk_file_writer_) {
    bulk_file_writer_->Close();
    delete bulk_file_writer_;
    bulk_file_writer_ = NULL;
  }

  return 0;
}
