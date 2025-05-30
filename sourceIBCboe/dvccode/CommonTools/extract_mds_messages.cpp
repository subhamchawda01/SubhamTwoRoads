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
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/multi_shm_messages.hpp"

template <class T>
class MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et, HFSAT::BulkFileWriter &bulk_file_writer) {
    T next_event_;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
        if (available_len_ < sizeof(next_event_)) break;
            if(et < next_event_.source_time.tv_sec   || next_event_.source_time.tv_sec < st) continue;
            bulk_file_writer.Write(&next_event_, sizeof(T)); 
	    bulk_file_writer.CheckToFlushBuffer();
      }
    }
  }

  static void ReadRealDataMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_);
};

int main(int argc, char** argv) {
  std::cout << sizeof(NSE_MDS::NSETBTDataCommonStructProShm) << std::endl;
  std::cout << sizeof(HFSAT::MultiShmMessage) << std::endl;
  std::cout << sizeof(HFSAT::GenericControlRequestStruct) << std::endl;
  std::cout << sizeof(HFSAT::GenericORSReplyStructLiveProShm) << std::endl;
  if (argc != 6 ) {
    std::cout
        << " USAGE: EXEC <exchange> <file_path> <start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec> <output file>"
        << std::endl;
    exit(0);
  }
  std::string exch = argv[1];

  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);
  HFSAT::BulkFileWriter writer;
  writer.Open(argv[5]);
  int st = atol(argv[3]);
  int et = atol(argv[4]);

  if (exch == "NSE") {
    MDSLogReader<NSE_MDS::NSEDotexOfflineCommonStruct>::ReadMDSStructs(reader, st, et, writer);
  } else {
    std::cout << "Wrong exchange...!!" << exch << std::endl;
  }
  writer.Close();
  reader.close();
  return 0;
}

