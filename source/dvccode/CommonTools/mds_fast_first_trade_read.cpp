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

class MDSLogReader {
 public:
  static void ReadMDSStructsGeneric(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::MDS_MSG::GenericMDSMessage next_event_;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.generic_data_.nse_data_.source_time;
        if(st > next_event_timestamp_.tv_sec )
		continue;
	if(et < next_event_timestamp_.tv_sec ) //time filter
        	break;
	if (next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSETrade){
	       std::cout << next_event_.ToStringLine();
       	       break;
	}
      }
      bulk_file_reader_.close();
    }
  }

  static void ReadMDSStructsNse(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.source_time;
        if(st > next_event_timestamp_.tv_sec )
                continue;
        if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
        if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade){
        	std::cout << next_event_.ToStringLine();
	       	break;
      	}
      }
      bulk_file_reader_.close();
    }
  }

};

int main(int argc, char** argv) {
  std::cout << sizeof(NSE_MDS::NSETBTDataCommonStructProShm) << std::endl;
  std::cout << sizeof(HFSAT::MultiShmMessage) << std::endl;
  std::cout << sizeof(HFSAT::GenericControlRequestStruct) << std::endl;
  std::cout << sizeof(HFSAT::GenericORSReplyStructLiveProShm) << std::endl;
  if (argc != 5) {
    std::cout
        << " USAGE: EXEC <exchange> <file_path> <start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec>"
        << std::endl;
    exit(0);
  }
  std::string exch = argv[1];
  int st = atol(argv[3]);
  int et = atol(argv[4]);
  std::ifstream input_file(argv[2]);
  std::string str;
  while(std::getline(input_file,str)){
  	HFSAT::BulkFileReader reader;
  	reader.open(str);
  	if (exch == "GENERIC")
    		MDSLogReader::ReadMDSStructsGeneric(reader, st, et);
  	else if (exch == "NSE")
    		MDSLogReader::ReadMDSStructsNse(reader, st, et);
  	else
    		std::cout << "Wrong exchange...!!" << exch << std::endl;
 }
 return 0;
}
