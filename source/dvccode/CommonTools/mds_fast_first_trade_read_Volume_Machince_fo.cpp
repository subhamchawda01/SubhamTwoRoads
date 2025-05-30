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

// Storage class of old size and price
typedef struct {
  double price;
  int32_t size;
}Data;

class HistoryTracker {
 public:
  HistoryTracker() {}
  std::map<uint64_t, Data> order_id_data_;
};

class MDSLogReader {

 public:
  static void ReadMDSStructsGeneric(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::MDS_MSG::GenericMDSMessage next_event_;
    int64_t total_msgs = 0;
    int64_t total_mod_msgs = 0;
    int64_t total_price_change_msgs = 0;
    int64_t count_trade = 0;
    int64_t trade_quan = 0;
    double ratio = 1;
    double low_exec_band_price = 0.0;
    std::map<int32_t, HistoryTracker*> token_to_history_tracker; 
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.generic_data_.nse_data_.source_time;
        if(st > next_event_timestamp_.tv_sec )
                continue;
	else if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
        
        if ((next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSEOrderDelta)) { //|| (next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSEOrderSpreadDelta )){

          if(next_event_.generic_data_.nse_data_.activity_type == 'N' || 
	     next_event_.generic_data_.nse_data_.activity_type == 'M' || 
	     next_event_.generic_data_.nse_data_.activity_type == 'X'){

             //Order Add
             if(next_event_.generic_data_.nse_data_.activity_type == 'N') {
               total_msgs++;
               if (token_to_history_tracker.find(next_event_.generic_data_.nse_data_.token) == token_to_history_tracker.end()) {
                 token_to_history_tracker[next_event_.generic_data_.nse_data_.token] = new HistoryTracker();
               }
               // Update History
               std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.generic_data_.nse_data_.token]->order_id_data_;
               Data data; 
               data.price = (double)((next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100);
               data.size = (next_event_.generic_data_.nse_data_.data).nse_order.order_qty;
               old_data_info[(next_event_.generic_data_.nse_data_.data).nse_order.order_id] = data;
             }
       	     //Order Modify
	     else if(next_event_.generic_data_.nse_data_.activity_type == 'M'){
               total_msgs++;
               if (token_to_history_tracker.end() == token_to_history_tracker.find(next_event_.generic_data_.nse_data_.token)) {
                 token_to_history_tracker[next_event_.generic_data_.nse_data_.token] = new HistoryTracker();
               }
               std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.generic_data_.nse_data_.token]->order_id_data_;

               double old_price;
               double old_size;
               if (old_data_info.find((next_event_.generic_data_.nse_data_.data).nse_order.order_id) == old_data_info.end()) {
                 // order id not found, consider this as a new order add
                 old_price = low_exec_band_price;
                 old_size = 0;
                 Data data;
                 data.price = (double)(next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100;
                 data.size = (next_event_.generic_data_.nse_data_.data).nse_order.order_qty;
                 old_data_info[(next_event_.generic_data_.nse_data_.data).nse_order.order_id] = data;
                 continue;
               } else {
                 old_price = old_data_info[(next_event_.generic_data_.nse_data_.data).nse_order.order_id].price;
                 old_size = old_data_info[(next_event_.generic_data_.nse_data_.data).nse_order.order_id].size;

                 old_data_info[(next_event_.generic_data_.nse_data_.data).nse_order.order_id].price =
                 (double)(next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100;
                 old_data_info[(next_event_.generic_data_.nse_data_.data).nse_order.order_id].size = (next_event_.generic_data_.nse_data_.data).nse_order.order_qty;
               }

		total_mod_msgs++;
                if ( (next_event_.generic_data_.nse_data_.data).nse_order.buysell == (HFSAT::TradeType_t::kTradeTypeBuy) ) {
                  if ((double)((next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100) > old_price)
                    total_price_change_msgs++;
                  else if ( (double)((next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100) == old_price ) {
                    if ( (next_event_.generic_data_.nse_data_.data).nse_order.order_qty > old_size )
                      total_price_change_msgs++;
                  }
                }
                else if ((next_event_.generic_data_.nse_data_.data).nse_order.buysell == (HFSAT::TradeType_t::kTradeTypeSell) ) {
                  if ((double)((next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100) < old_price)
                    total_price_change_msgs++;
                  else if ( (double)((next_event_.generic_data_.nse_data_.data).nse_order.order_price / (double)100) == old_price ) {
                    if ( (next_event_.generic_data_.nse_data_.data).nse_order.order_qty > old_size )
                      total_price_change_msgs++;
                  }
                }
	     }
             //Order Delete
             else if(next_event_.generic_data_.nse_data_.activity_type == 'X') {
               if (token_to_history_tracker.find(next_event_.generic_data_.nse_data_.token) == token_to_history_tracker.end()) {
                 continue;
               }
               std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.generic_data_.nse_data_.token]->order_id_data_;

               if (old_data_info.find((next_event_.generic_data_.nse_data_.data).nse_order.order_id) == old_data_info.end()) {
                 continue;
               }
               total_msgs++;

               old_data_info.erase((next_event_.generic_data_.nse_data_.data).nse_order.order_id); 
             }
          }
       
        }else if ((next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSETrade)) { //|| (next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSESpreadTrade) ){
           if (token_to_history_tracker.find(next_event_.generic_data_.nse_data_.token) == token_to_history_tracker.end()) {
             token_to_history_tracker[next_event_.generic_data_.nse_data_.token] =
                new HistoryTracker();  // create dummy struct in case of packet loss
           }

           std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_.generic_data_.nse_data_.token]->order_id_data_;

           if (old_data_info.end() != old_data_info.find((next_event_.generic_data_.nse_data_.data).nse_trade.buy_order_id)) {
             old_data_info[(next_event_.generic_data_.nse_data_.data).nse_trade.buy_order_id].size -= (next_event_.generic_data_.nse_data_.data).nse_trade.trade_qty;
           }

           if (old_data_info.end() != old_data_info.find((next_event_.generic_data_.nse_data_.data).nse_trade.sell_order_id)) {
             old_data_info[(next_event_.generic_data_.nse_data_.data).nse_trade.sell_order_id].size -= (next_event_.generic_data_.nse_data_.data).nse_trade.trade_qty;
           }

           total_msgs++;
	   count_trade++;
	   trade_quan += next_event_.generic_data_.nse_data_.GetTradeSize();
        }else if (next_event_.generic_data_.nse_data_.msg_type == NSE_MDS::MsgType::kNSETradeExecutionRange){
           low_exec_band_price = next_event_.generic_data_.nse_data_.data.nse_trade_range.low_exec_band / 100;
        }
      }
      if(count_trade > 0){
        ratio = (double)total_msgs / count_trade;
      }else{
        ratio = total_msgs;
      }
      std::string sym_ = HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(
                          next_event_.generic_data_.nse_data_.token, next_event_.generic_data_.nse_data_.segment_type);
      if(sym_.find("_CE_") != std::string::npos || sym_.find("_PE_") != std::string::npos){
        std::vector<char*> tokens_;
        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(&sym_[0], "_", tokens_);
        std::ostringstream oss_;
	oss_ << tokens_[0] << "_" << tokens_[1] << "_" << tokens_[2] << "_" << tokens_[4] << "_" << tokens_[3];
	sym_ = oss_.str();
      }
      std::cout << sym_ << " " << total_msgs << " " << count_trade << " "  << ratio << " " << total_mod_msgs << " " << total_price_change_msgs << "\n";
//      std::cout << next_event_.symbol << " " << count_trade << " " << trade_quan << "\n";
      bulk_file_reader_.close();
      sleep(2);
    }
  }

  static void ReadMDSStructsNse(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
    int64_t total_msgs = 0;
    int64_t total_mod_msgs = 0;
    int64_t total_price_change_msgs = 0;
    int64_t count_trade = 0;
    int64_t trade_quan = 0;
    double ratio = 1;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
        if (available_len_ < sizeof(next_event_)) break;
        HFSAT::ttime_t next_event_timestamp_ = next_event_.source_time;
        if(st > next_event_timestamp_.tv_sec )
                continue;
	else if(et < next_event_timestamp_.tv_sec ) //time filter
                break;
        
        if (next_event_.msg_type == NSE_MDS::MsgType::kNSEOrderDelta){
          if(next_event_.data.nse_dotex_order_delta.activity_type == '1' || 
	     next_event_.data.nse_dotex_order_delta.activity_type == '3' || 
	     next_event_.data.nse_dotex_order_delta.activity_type == '4'){
             total_msgs++;
       	     //Order Modify
	     if(next_event_.data.nse_dotex_order_delta.activity_type == '4'){
		total_mod_msgs++;
                if ( next_event_.data.nse_dotex_order_delta.buysell == 'B' ) {
                  if (next_event_.data.nse_dotex_order_delta.order_price > next_event_.data.nse_dotex_order_delta.old_price)
                    total_price_change_msgs++;
                  else if ( next_event_.data.nse_dotex_order_delta.order_price == next_event_.data.nse_dotex_order_delta.old_price ) {
                    if ( next_event_.data.nse_dotex_order_delta.volume_original > next_event_.data.nse_dotex_order_delta.old_size )
                      total_price_change_msgs++;
                  }
                }
                else if (next_event_.data.nse_dotex_order_delta.buysell == 'S' ) {
                  if (next_event_.data.nse_dotex_order_delta.order_price < next_event_.data.nse_dotex_order_delta.old_price)
                    total_price_change_msgs++;
                  else if ( next_event_.data.nse_dotex_order_delta.order_price == next_event_.data.nse_dotex_order_delta.old_price ) {
                    if ( next_event_.data.nse_dotex_order_delta.volume_original > next_event_.data.nse_dotex_order_delta.old_size )
                      total_price_change_msgs++;
                  }
                }
	     }
          }
       
        }else if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade){
        	total_msgs++;
		count_trade++;
		trade_quan += next_event_.GetTradeSize();
      	}
      }
      if(count_trade > 0){
        ratio = (double)total_msgs / count_trade;
      }else{
        ratio = total_msgs;
      }
      std::string sym_ = next_event_.symbol;
      if(sym_.find("_CE_") != std::string::npos || sym_.find("_PE_") != std::string::npos){
        std::vector<char*> tokens_;
        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(next_event_.symbol, "_", tokens_);
        std::ostringstream oss_;
	oss_ << tokens_[0] << "_" << tokens_[1] << "_" << tokens_[2] << "_" << tokens_[4] << "_" << tokens_[3];
	sym_ = oss_.str();
      }
      std::cout << sym_ << " " << total_msgs << " " << count_trade << " "  << ratio << " " << total_mod_msgs << " " << total_price_change_msgs << "\n";
//      std::cout << next_event_.symbol << " " << count_trade << " " << trade_quan << "\n";
      bulk_file_reader_.close();
    }
  }
  static void ReadMDSStructsORSReply(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    HFSAT::GenericORSReplyStructLiveProShm next_event_;
    int total_exec = 0;
    int total_order = 0;
    int total_modify = 0;
    int total_modifyof = 0;
    double trade_price = -1;
    std::map<int,int> saciToPos;
// as we are considering cash machines
    saciToPos[314] = 0;
    saciToPos[305] = 1;
    saciToPos[340] = 2;
    saciToPos[320] = 3;
    saciToPos[322] = 4;
    saciToPos[350] = 5;
    saciToPos[357] = 6;
    int saciToExec[7] = {0};
    int saciToOrder[7] = {0};
    int saciToModifyOrder[7] = {0};
    int saciToModifyOF[7] = {0};
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HFSAT::GenericORSReplyStructLiveProShm));
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
		else
                        std::cout << "Unknown Machine" << std::endl;
        }
	else if(next_event_.orr_type_ == HFSAT::kORRType_Conf ||next_event_.orr_type_ == HFSAT::kORRType_CxRe || next_event_.orr_type_ == HFSAT::kORRType_CxlRejc || next_event_.orr_type_ ==  HFSAT::kORRType_Cxld){
        
		if(saciToPos.find(saci) != saciToPos.end()) {
	          saciToOrder[saciToPos[saci]]++;
                  if(next_event_.orr_type_ == HFSAT::kORRType_CxRe) {
                    saciToModifyOrder[saciToPos[saci]]++;
                    if(next_event_.order_flags_ == 1)
                      saciToModifyOF[saciToPos[saci]]++;
                  }
                  
                }
		else 
			std::cout << "Unknown Machine" << std::endl;
	}
      }
      std::cout << next_event_.symbol_;
      for (int pos =0; pos < 7; pos++){
	total_order += saciToOrder[pos];
	total_exec += saciToExec[pos];
        total_modify += saciToModifyOrder[pos];
        total_modifyof += saciToModifyOF[pos];
      	std::cout << " " << saciToModifyOrder[pos] << " " << saciToModifyOF[pos] << " " << saciToOrder[pos] << " " << saciToExec[pos];
      }
      std::cout << " " << total_modify << " " << total_modifyof << " " << total_order << " " << total_exec << " " << std::fixed << std::setprecision(2) << trade_price << "\n";
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
        else if (exch == "ORS_REPLY_LIVE")
               MDSLogReader::ReadMDSStructsORSReply(reader, st, et);
  	else if (exch == "NSE")
    		MDSLogReader::ReadMDSStructsNse(reader, st, et);
	else
    		std::cout << "Wrong exchange...!!" << exch << std::endl;
 }
 return 0;
}
