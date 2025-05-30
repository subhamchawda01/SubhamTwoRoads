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

enum class Index {
  Invalid = 0,
  NIFTYMIDSELECTOI,
  NIFTYFINSERVICEOI,
  NIFTYBANKOI,
  NIFTY50OI,
  NIFTYNXT50
};

HFSAT::BulkFileWriter writer;

template <class T>
class MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader &bulk_file_reader_, HFSAT::BulkFileWriter &bulk_file_writer,std::string& index_) {
    T next_event_;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
        if (available_len_ < sizeof(next_event_)) break;

        struct timeval tv;
        tv.tv_sec = (__time_t)next_event_.source_time.tv_sec;
        tv.tv_usec = (__suseconds_t)next_event_.source_time.tv_usec;

        HFSAT::MDS_MSG::GenericMDSMessage *converted_dotex_offline_msg = new HFSAT::MDS_MSG::GenericMDSMessage();
        converted_dotex_offline_msg->generic_data_.nse_data_.source_time = tv;
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.Token = next_event_.Token;
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.FillPrice = next_event_.FillPrice;
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.FillVolume = next_event_.FillVolume;
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.OpenInterest = next_event_.OpenInterest;
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.DayHiOI = next_event_.DayHiOI;
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.DayLoOI = next_event_.DayLoOI;
        converted_dotex_offline_msg->generic_data_.nse_data_.msg_type = NSE_MDS::MsgType::kNSEOpenInterestTick;
        converted_dotex_offline_msg->mds_msg_exch_ = HFSAT::MDS_MSG::MDSMessageExchType::NSE;

        if(strcmp(index_.c_str(),"NIFTYMIDSELECTOI") == 0)        
          converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.MarketType = static_cast<int16_t>(Index::NIFTYMIDSELECTOI);
        else if(strcmp(index_.c_str(),"NIFTYFINSERVICEOI") == 0)        
          converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.MarketType = static_cast<int16_t>(Index::NIFTYFINSERVICEOI);
        else if(strcmp(index_.c_str(),"NIFTYBANKOI") == 0)        
          converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.MarketType = static_cast<int16_t>(Index::NIFTYBANKOI);
        else if(strcmp(index_.c_str(),"NIFTY50OI") == 0)        
          converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.MarketType = static_cast<int16_t>(Index::NIFTY50OI);
        else if(strcmp(index_.c_str(),"NIFTYNXT50") == 0)        
          converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.MarketType = static_cast<int16_t>(Index::NIFTYNXT50);
        

        /*
        std::cout << "TOKEN " << next_event_.Token << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.Token << std::endl; std::cout <<
        "MarketType " << next_event_.MarketType << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.MarketType << std::endl; std::cout <<
        "FillPrice " << next_event_.FillPrice << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.FillPrice << std::endl; std::cout <<
        "FillVolume " << next_event_.FillVolume << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.FillVolume << std::endl; std::cout <<
        "OpenInterest " << next_event_.OpenInterest << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.OpenInterest << std::endl; std::cout <<
        "DayHiOI " << next_event_.DayHiOI << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.DayHiOI << std::endl; std::cout <<
        "DayLoOI " << next_event_.DayLoOI << " " <<
        converted_dotex_offline_msg->generic_data_.nse_data_.data.nse_oi_data.DayLoOI << std::endl; std::cout <<
        static_cast<int>(converted_dotex_offline_msg->generic_data_.nse_data_.msg_type) << std::endl;


        std::cout << "PRINTING STRUCT " << std::endl;
        std::cout << converted_dotex_offline_msg->ToString() << std::endl;
        std::cout << "PRINT STRUCT COMPLETED " << std::endl;
        */

        bulk_file_writer.Write(converted_dotex_offline_msg, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

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
  if (argc != 4) {
    std::cout << " USAGE: EXEC [NIFTYMIDSELECTOI,NIFTYFINSERVICEOI,NIFTYBANKOI,NIFTY50OI,NIFTYNXT50] <unconverted_file_path> <converted_file_path>" << std::endl;
    exit(0);
  }
  std::string index_ = argv[1];

  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);
  writer.Open(argv[3]);

  MDSLogReader<NSE_MDS::ST_TICKET_INDEX_INFO>::ReadMDSStructs(reader, writer,index_);

  writer.Close();
  reader.close();

  std::cout << argv[2] << "Conversion Completed\n";

  return 0;
}
