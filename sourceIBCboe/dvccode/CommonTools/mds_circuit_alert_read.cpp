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
#include <cmath>
#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/multi_shm_messages.hpp"

std::string sym="";
class MDSLogReader {
 public:
  static int ReadMDSStructsGeneric(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::MDS_MSG::GenericMDSMessage next_event_;
    int trade_price = -1;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
	if (next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSETrade){
		sym=HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
            next_event_.generic_data_.nse_data_.token, next_event_.generic_data_.nse_data_.segment_type);
	       trade_price = next_event_.generic_data_.nse_data_.data.nse_trade.trade_price;
	}
      }
      bulk_file_reader_.close();
    }
  return trade_price;
  }

};

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout
        << " USAGE: EXEC <exchange> <file_path> <start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec>"
        << std::endl;
    exit(0);
  }
  std::string exch = argv[1];
  if (exch != "GENERIC"){
	  std::cout << "Wrong exchange...!!" << exch << std::endl;
          return 0;
	}
  int st = -1;
  int et = -1;
  std::ifstream input_file(argv[2]);
  std::string str;
  int level_per[300];
  int level_assg=0;
  int level_incr=6;
  int i,j;
  for(i=0,j=1;i<=100;i++,j++){
  	level_per[100+i]=level_assg;
	level_per[100-i]=-level_assg;
	if(j>level_incr){
	level_incr=5;
	j=1;
	level_assg++;
	}

  }
  while(std::getline(input_file,str)){
  	HFSAT::BulkFileReader reader;
	std::istringstream ss(str);
	std::string ifile,cprice;
	int  change_per;
	double act_mov;
	ss >> ifile >> cprice;
	double trade_price;
	double closing_price = std::stod(cprice);
  	reader.open(ifile);
        trade_price = (double)MDSLogReader::ReadMDSStructsGeneric(reader, st, et);
	trade_price = trade_price/100.0;
	if ( trade_price == -1 ) continue;
	else {
		change_per = ((trade_price - closing_price)/(double)closing_price)*100;
		act_mov = ((trade_price - closing_price)/(double)closing_price)*100.0;
		std::cout << sym << " " << closing_price << " " << trade_price << " "<< act_mov <<" " << level_per[change_per+100] << std::endl;
	}
 }
 return 0;
}
