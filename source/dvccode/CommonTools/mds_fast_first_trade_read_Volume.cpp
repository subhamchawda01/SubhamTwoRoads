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
    int count_trade = 0;
    int trade_quan = 0;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.generic_data_.nse_data_.source_time;
        if(st > next_event_timestamp_.tv_sec )
		continue;
	else if(et < next_event_timestamp_.tv_sec ) //time filter
        	break;
	if (next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSETrade){
	        count_trade++;
	       	trade_quan += next_event_.GetTradeSize();
	}
      }
      std::cout << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
            next_event_.generic_data_.nse_data_.token, next_event_.generic_data_.nse_data_.segment_type) << 
	    " " << count_trade << " " << trade_quan << "\n";
      bulk_file_reader_.close();
    }
  }

  static void ReadMDSStructsNse(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
    int count_trade = 0;
    int trade_quan = 0;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.source_time;
        if(st > next_event_timestamp_.tv_sec )
                continue;
	else if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
        if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade){
		count_trade++;
		trade_quan += next_event_.GetTradeSize();
      	}
      }
      std::cout << next_event_.symbol << " " << count_trade << " " << trade_quan << "\n";
      bulk_file_reader_.close();
    }
  }
  static void ReadMDSStructsORSReply(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::GenericORSReplyStruct next_event_;
    int total_exec = 0;
    int total_order = 0;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::GenericORSReplyStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.client_request_time_;
        if(st > next_event_timestamp_.tv_sec )
                continue;
        else if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
        if (next_event_.orr_type_ == HFSAT::kORRType_Exec){
                total_exec++;
        }
	else if(next_event_.orr_type_ == HFSAT::kORRType_Conf || next_event_.orr_type_ == HFSAT::kORRType_CxRe || next_event_.orr_type_ == HFSAT::kORRType_CxlRejc || next_event_.orr_type_ == HFSAT::kORRType_Cxld ){
		total_order++;
	}
      }
      std::cout << next_event_.symbol_ << " " << total_exec << " " << total_order << "\n";
      bulk_file_reader_.close();
    }
  }
  static void ReadMDSStructsORSReplyLive(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::GenericORSReplyStructLiveProShm next_event_;
    int total_exec = 0;
    int total_order = 0;
    double total_exec_price = 0;
    double total_order_price = 0;
    int total_modify = 0;
    int total_modifyof = 0;
    double trade_price = -1;
    std::map<int,int> saciToPos;
// as we are considering cash machines
    saciToPos[308] = 0;
    saciToPos[307] = 1;
    saciToPos[306] = 2;
    saciToPos[312] = 3;
    saciToPos[344] = 4;
    saciToPos[330] = 5;
    int saciToExec[6] = {0};
    int saciToOrder[6] = {0};
    double saciToExecPrice[6] = {0};
    double saciToOrderPrice[6] = {0};
    int saciToModifyOrder[6] = {0};
    int saciToModifyOF[6] = {0};
    if (bulk_file_reader_.is_open()) {
      std::unordered_map <std::string, int> saci_saos_sizeexecuted_map;
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::GenericORSReplyStructLiveProShm));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.client_request_time_;
        if(st > next_event_timestamp_.tv_sec )
                continue;
        else if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
        int saci=next_event_.server_assigned_client_id_>>16;
        std::string map_key = std::to_string(next_event_.server_assigned_client_id_) + "_" + std::to_string(next_event_.server_assigned_order_sequence_);
        if (next_event_.orr_type_ == HFSAT::kORRType_Exec){
                if(saciToPos.find(saci) != saciToPos.end()){
                         saciToExec[saciToPos[saci]]++;
                         int temp_size = next_event_.size_executed_ - saci_saos_sizeexecuted_map[map_key];
                         saciToExecPrice[saciToPos[saci]] += next_event_.price_ * temp_size;
                         saci_saos_sizeexecuted_map[map_key] = next_event_.size_executed_;
                         trade_price = next_event_.price_; }
                else
                        std::cout << "Unknown Machine test " << saci << std::endl;
        }
        else if(next_event_.orr_type_ == HFSAT::kORRType_Conf ||next_event_.orr_type_ == HFSAT::kORRType_CxRe || next_event_.orr_type_ ==  HFSAT::kORRType_Cxld){

                if(saciToPos.find(saci) != saciToPos.end()) {
                  saciToOrder[saciToPos[saci]]++;
                  saciToOrderPrice[saciToPos[saci]] += next_event_.price_ * next_event_.size_remaining_;
                  if(next_event_.orr_type_ == HFSAT::kORRType_CxRe) {
                    saciToModifyOrder[saciToPos[saci]]++;
                    if(next_event_.order_flags_ == 1)
                      saciToModifyOF[saciToPos[saci]]++;
                  }   

                }
                else
                        std::cout << "Unknown Machine test " << saci << std::endl;
        }               
      }
      std::cout << next_event_.symbol_;
      for (int pos =0; pos < 6; pos++){
        total_order += saciToOrder[pos];
        total_exec += saciToExec[pos];
        total_order_price += saciToOrderPrice[pos];
        total_exec_price += saciToExecPrice[pos];
        total_modify += saciToModifyOrder[pos];
        total_modifyof += saciToModifyOF[pos];
        std::cout << " " << saciToModifyOrder[pos] << " " << saciToModifyOF[pos] << " " << saciToOrder[pos] << " " << saciToExec[pos];
      }
      std::cout << " " << total_modify << " " << total_modifyof << " " << total_order << " " << total_exec
                << std::fixed << std::setprecision(2) << " " << total_order_price << " " << total_exec_price << " " << trade_price << "\n";
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
  	else if (exch == "ORS_REPLY")
		MDSLogReader::ReadMDSStructsORSReply(reader, st, et);
        else if (exch == "ORS_REPLY_LIVE")
               MDSLogReader::ReadMDSStructsORSReplyLive(reader, st, et);
	else
    		std::cout << "Wrong exchange...!!" << exch << std::endl;
 }
 return 0;
}
