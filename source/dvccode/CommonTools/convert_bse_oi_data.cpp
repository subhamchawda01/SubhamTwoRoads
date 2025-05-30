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

#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/multi_shm_messages.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"


HFSAT::BulkFileWriter bulk_file_writer;

template <class T>
class MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader &bulk_file_reader_) {
    if (bulk_file_reader_.is_open()) {
      while (true) {

        EOBI_MDS::EOBICommonStruct bse_data_;
        size_t available_len_ = bulk_file_reader_.read(&bse_data_, sizeof(bse_data_));

        if (available_len_ < sizeof(bse_data_)) {  // check if reading complete structs
           std::cerr << "Incorrect len read: expected " << sizeof(bse_data_) << " read len: " << available_len_
                   << std::endl;
           break;
         }   

        HFSAT::MDS_MSG::GenericMDSMessage generic_msg;
        generic_msg.generic_data_.bse_data_ = bse_data_;
        generic_msg.mds_msg_exch_ = HFSAT::MDS_MSG::MDSMessageExchType::BSE;

        bulk_file_writer.Write(&generic_msg, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        bulk_file_writer.CheckToFlushBuffer();
      }
    }
  }

  static void ReadRealDataMDSStructs(HFSAT::BulkFileReader &bulk_file_reader_);
};

int main(int argc, char **argv) {
  std::cout << sizeof(NSE_MDS::NSETBTDataCommonStructProShm) << std::endl;
  std::cout << sizeof(HFSAT::MultiShmMessage) << std::endl;
  std::cout << sizeof(HFSAT::GenericControlRequestStruct) << std::endl;
  std::cout << sizeof(HFSAT::GenericORSReplyStructLiveProShm) << std::endl;
  std::cout << sizeof(HFSAT::MDS_MSG::GenericMDSMessage) << std::endl;
  if (argc != 3) {
    std::cout << " USAGE: EXEC <unconverted_file_path> <converted_file_path>" << std::endl;
    exit(0);
  }

  HFSAT::BulkFileReader reader;
  reader.open(argv[1]);
  bulk_file_writer.Open(argv[2]);

  MDSLogReader<EOBI_MDS::EOBICommonStruct>::ReadMDSStructs(reader);

  bulk_file_writer.Close();
  reader.close();

  std::cout << argv[2] << "Conversion Completed\n";

  return 0;
}
