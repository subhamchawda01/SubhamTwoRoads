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
#include <map>
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
    int total_exec = 0;
    int total_order = 0;
    double trade_price = -1;
    std::map<int,int> saciToPos;
// as we are considering cash machines
    saciToPos[308] = 0;
    saciToPos[307] = 1;
    saciToPos[306] = 2;
    int Unknownmachine_Exec = 0;
    int Unknownmachine_Order = 0;
    int saciToExec[3] = {0};
    int saciToOrder[3] = {0};
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.generic_data_.ors_reply_data_.client_request_time_;
        if(st > next_event_timestamp_.tv_sec )
		continue;
	else if(et < next_event_timestamp_.tv_sec ) //time filter
        	break;
	int saci = next_event_.generic_data_.ors_reply_data_.server_assigned_client_id_ >> 16;
	if (next_event_.generic_data_.ors_reply_data_.orr_type_ == HFSAT::kORRType_Exec){
                if(saciToPos.find(saci) != saciToPos.end()){
                         saciToExec[saciToPos[saci]]++;
			 trade_price = next_event_.generic_data_.ors_reply_data_.price_; }
                else {
                        Unknownmachine_Exec++;
                        std::cout << "Unknown Machine" << std::endl;
                }
        }
	else if(next_event_.generic_data_.ors_reply_data_.orr_type_ == HFSAT::kORRType_Conf || next_event_.generic_data_.ors_reply_data_.orr_type_ == HFSAT::kORRType_CxRe ||next_event_.generic_data_.ors_reply_data_.orr_type_ == HFSAT::kORRType_CxlRejc || next_event_.generic_data_.ors_reply_data_.orr_type_ == HFSAT::kORRType_Cxld ){
                if(saciToPos.find(saci) != saciToPos.end()){
                        saciToOrder[saciToPos[saci]]++;
                }
                else{
                        Unknownmachine_Order++;
                        std::cout << "Unknown Machine" << std::endl;
                }
        }
      }
      std::cout << next_event_.generic_data_.ors_reply_data_.symbol_;
      for (int pos =0; pos < 3; pos++){
        total_order += saciToOrder[pos];
        total_exec += saciToExec[pos];
        std::cout << " " << saciToOrder[pos] << " " << saciToExec[pos];
      }
      total_order += Unknownmachine_Order;
      total_exec += Unknownmachine_Exec;
      std::cout << " " << total_order << " " << total_exec << " " << std::fixed << std::setprecision(2) << trade_price <<"\n";
      bulk_file_reader_.close();
    }
  }


  static void ReadMDSStructsORSReplyLiveSACI(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::GenericORSReplyStructLiveProShm next_event_;
    int total_exec = 0;
    int total_order = 0;
    double total_exec_price = 0;
    double total_order_price = 0;
    int total_modify = 0;
    int total_modifyof = 0;
    std::unordered_map<int,double> trade_price;
    std::unordered_map<int,int> saciToPos;
    std::unordered_map<int,int> saciToBasePos;
    std::unordered_map<int,int> saciToExec;
    std::unordered_map<int,int> saciToOrder;
    std::unordered_map<int,double> saciToExecPrice;
    std::unordered_map<int,double> saciToOrderPrice;
    std::unordered_map<int,int> saciToModifyOrder;
    std::unordered_map<int,int> saciToModifyOF;
// as we are considering cash machines
    saciToPos[308] = 0;
    saciToPos[307] = 1;
    saciToPos[306] = 2;
    saciToPos[312] = 3;
    saciToPos[344] = 4;
    saciToPos[330] = 5;
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
        int saci_base=next_event_.server_assigned_client_id_;
        
        std::string map_key = std::to_string(next_event_.server_assigned_client_id_) + "_" + std::to_string(next_event_.server_assigned_order_sequence_);
        if (next_event_.orr_type_ == HFSAT::kORRType_Exec){
                if(saciToPos.find(saci) != saciToPos.end()){
                         saciToBasePos[saci_base] = saciToPos[saci];
                         saciToExec[saci_base]++;
                         int temp_size = next_event_.size_executed_ - saci_saos_sizeexecuted_map[map_key];
                         saciToExecPrice[saci_base] += next_event_.price_ * temp_size;
                         saci_saos_sizeexecuted_map[map_key] = next_event_.size_executed_;
                         trade_price[saci_base] = next_event_.price_; }
                else
                        std::cout << "Unknown Machine test " << saci << std::endl;
        }
        else if(next_event_.orr_type_ == HFSAT::kORRType_Conf ||next_event_.orr_type_ == HFSAT::kORRType_CxRe || next_event_.orr_type_ ==  HFSAT::kORRType_Cxld){

                if(saciToPos.find(saci) != saciToPos.end()) {
                  saciToBasePos[saci_base] = saciToPos[saci];
                  saciToOrder[saci_base]++;
                  saciToOrderPrice[saci_base] += next_event_.price_ * next_event_.size_remaining_;
                  if(next_event_.orr_type_ == HFSAT::kORRType_CxRe) {
                    saciToModifyOrder[saci_base]++;
                    if(next_event_.order_flags_ == 1)
                      saciToModifyOF[saci_base]++;
                  }

                }
                else
                        std::cout << "Unknown Machine test " << saci << std::endl;
        }
      }
      for ( auto saci_itr : saciToBasePos ) {
        std::cout << next_event_.symbol_;
        for (int pos =0; pos < 6; pos++){
          if ( saci_itr.second == pos ) { 
            std::cout << " " << saciToModifyOrder[saci_itr.first] << " " << saciToModifyOF[saci_itr.first] << " " << saciToOrder[saci_itr.first] << " " << saciToExec[saci_itr.first];
          total_order = saciToOrder[saci_itr.first];
          total_exec = saciToExec[saci_itr.first];
          total_order_price = saciToOrderPrice[saci_itr.first];
          total_exec_price = saciToExecPrice[saci_itr.first];
          total_modify = saciToModifyOrder[saci_itr.first];
          total_modifyof = saciToModifyOF[saci_itr.first];
          } else {
            std::cout << " 0 0 0 0";
          }
        }
        std::cout << " " << total_modify << " " << total_modifyof << " " << total_order << " " << total_exec  
                  << std::fixed << std::setprecision(2) << " " << total_order_price << " " << total_exec_price \
                  << " " << trade_price[saci_itr.first] << " " << saci_itr.first << std::endl;
      }
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
    double trade_price = -1;
    int Unknownmachine_Exec = 0;
    int Unknownmachine_Order = 0;
    std::map<int,int> saciToPos;
// as we are considering cash machines
    saciToPos[308] = 0;
    saciToPos[307] = 1;
    saciToPos[306] = 2;
    int saciToExec[3] = {0};
    int saciToOrder[3] = {0};
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::GenericORSReplyStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.client_request_time_;
        if(st > next_event_timestamp_.tv_sec )
                continue;
        else if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
	int saci=next_event_.server_assigned_client_id_>>16;
        if (next_event_.orr_type_ == HFSAT::kORRType_Exec){
		if(saciToPos.find(saci) != saciToPos.end()){
               		 saciToExec[saciToPos[saci]]++;
			 trade_price = next_event_.price_; }
		else{
                        Unknownmachine_Exec++;
               //         std::cout << "Unknown Machine" << std::endl;
                }
        }
	else if(next_event_.orr_type_ == HFSAT::kORRType_Conf ||next_event_.orr_type_ == HFSAT::kORRType_CxRe || next_event_.orr_type_ == HFSAT::kORRType_CxlRejc || next_event_.orr_type_ ==  HFSAT::kORRType_Cxld){
		if(saciToPos.find(saci) != saciToPos.end()){
			saciToOrder[saciToPos[saci]]++;
                }
		else {
                        Unknownmachine_Order++;
		//	std::cout << "Unknown Machine" << std::endl;
                }
	}
      }
      std::cout << next_event_.symbol_;
      for (int pos =0; pos < 3; pos++){
	total_order += saciToOrder[pos];
	total_exec += saciToExec[pos];
      	std::cout << " " << saciToOrder[pos] << " " << saciToExec[pos];
      }
      total_order += Unknownmachine_Order;
      total_exec += Unknownmachine_Exec;
      std::cout << " " << total_order << " " << total_exec << " " << std::fixed << std::setprecision(2) << trade_price << "\n";
      bulk_file_reader_.close();
    }
  }

};


int main(int argc, char** argv) {
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
        else if (exch == "ORS_REPLY")
               MDSLogReader::ReadMDSStructsORSReply(reader, st, et);
        else if (exch == "ORS_REPLY_LIVE")
               MDSLogReader::ReadMDSStructsORSReplyLiveSACI(reader, st, et);
	else
    		std::cout << "Wrong exchange...!!" << exch << std::endl;
 }
 return 0;
}
