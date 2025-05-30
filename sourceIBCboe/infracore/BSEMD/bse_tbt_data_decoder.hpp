// =====================================================================================
//
//       Filename:  nse_tbt_data_decoder.hpp
//
//    Description:  This class decodes the binary stream recevied via socket for BSE TBT data,
//                  prepares a struct and forwards it to processor class on relevant updates
//
//        Version:  1.0
//        Created:  09/17/2015 04:06:29 PM
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

#pragma once

#include <iostream>
#include <fstream>
#include <cstdlib>
#include<chrono>
#include<iomanip>
#include<bits/stdc++.h>
#include <tr1/unordered_map>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "infracore/BSEMD/bse_tbt_data_processor.hpp"
//#include <lzo/lzoconf.h>
//#include <lzo/lzo1z.h>

#define EOBI_MSG_HEADER_SIZE 8
#define EOBI_PRICE_DIVIDER 100000000.0

#define IS_LOGGING_ENABLE 0
#define ORDER_ADD 13100
#define ORDER_MODIFY 13101
#define ORDER_MODIFY_SAME_PRIORITY 13106
#define ORDER_DELETE 13102
#define ORDER_MASS_DELETE 13103
#define PARTIAL_EXECUTION 13105
#define FULL_EXECUTION 13104
#define EXECUTION_SUMMARY 13202
#define INSTRUMENT_INFO 13203
#define PRODUCT_SUMMARY 13600
#define SNAPSHOT_ORDER 13602
#define INSTRUMENT_SUMMARY 13601
#define HEARTBEAT_BSE 13001

#define IN_LEN (128 * 1024L)
#define OUT_LEN (IN_LEN + IN_LEN / 16 + 64 + 3)
//#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]
//static HEAP_ALLOC(wrkmem, LZO1Z_999_MEM_COMPRESS);

//static unsigned char __LZO_MMODEL out_lzo[OUT_LEN];

namespace HFSAT {
namespace BSEMD {

std::pair<uint16_t,std::pair<uint16_t,uint16_t>> get_current_date(){
        std::time_t t = std::time(0);   // get time now
        std::tm* now = std::localtime(&t);
        uint16_t year=now->tm_year+1900;
        uint16_t month=(now->tm_mon+1);
        uint16_t day=now->tm_mday;
        return {year,{month,day}};
}


// Function to swap endianness for a 32-bit integer
uint32_t swapEndian32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
}

// Function to swap endianness for a 16-bit integer
uint16_t swapEndian16(uint16_t value) {
    return ((value & 0xFF00) >> 8) |
           ((value & 0x00FF) << 8);
}

void swapEndianDouble(double &value) {
    // Create a temporary buffer to store the byte representation of the double
    unsigned char buffer[sizeof(double)];

    // Copy the byte representation of the double to the buffer
    memcpy(buffer, &value, sizeof(double));

    // Reverse the byte order in the buffer
    std::reverse(buffer, buffer + sizeof(double));

    // Copy the modified buffer back to the double variable
    memcpy(&value, buffer, sizeof(double));
}

std::time_t get_unix_time(uint16_t year,uint16_t month,uint16_t day,uint16_t hour,uint16_t minute,uint16_t second){
    std::tm timeInfo = {0};
    timeInfo.tm_year = year - 1900; // Years since 1900
    timeInfo.tm_mon = month - 1;    // Months are 0-based
    timeInfo.tm_mday = day;
    timeInfo.tm_hour = hour;
    timeInfo.tm_min = minute;
    timeInfo.tm_sec = second;

    std::chrono::system_clock::time_point tp=std::chrono::system_clock::from_time_t(std::mktime(&timeInfo));
    std::time_t unixTime = std::chrono::system_clock::to_time_t(tp);
    return unixTime;
}
class BSETBTDataDecoder {
 private:

  /*
  uint32_t bse_tbt_data_processor_.o_seq_num_;
  int64_t bse_tbt_data_processor_.o_security_id_;
  uint32_t bse_tbt_data_processor_.o_action_;
  uint8_t bse_tbt_data_processor_.o_side_;
  int64_t bse_tbt_data_processor_.o_price_;
  int32_t bse_tbt_data_processor_.o_size_;
  uint64_t bse_tbt_data_processor_.o_trd_reg_ts_;
  uint64_t bse_tbt_data_processor_.o_priority_ts_;
  int64_t bse_tbt_data_processor_.o_prev_price_;
  int32_t bse_tbt_data_processor_.o_prev_size_;
  uint64_t bse_tbt_data_processor_.o_prev_priority_ts_;
  bool bse_tbt_data_processor_.o_intermediate_;
  int16_t bse_tbt_data_processor_.o_synthetic_match_;

  */

  HFSAT::DebugLogger& dbglogger_;
  HFSAT::FastMdConsumerMode_t daemon_mode_;
  // Events after decoding will be forwarded to this class and it will choose what to based on modes defined
  BSETBTDataProcessor& bse_tbt_data_processor_;
  // Decoded event
  EOBI_MDS::EOBICommonStruct* bse_tbt_data_common_struct_;
  std::map<char, std::unordered_map<int32_t, uint32_t> > segment_to_token_seqno_map_;
//  std::tr1::unordered_map<char, std::tr1::unordered_map<int32_t, uint32_t> > segment_to_token_seqno_map_;
  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;
  uint32_t bse_spot_msg_seq_no;
  bool is_combined_source_mode_;
  uint32_t bardata_period_;
  int bardata_time_;

  BSETBTDataDecoder(HFSAT::DebugLogger& dbglogger, HFSAT::FastMdConsumerMode_t const& daemon_mode,
                    HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource)
      : dbglogger_(dbglogger),
        daemon_mode_(daemon_mode),
        bse_tbt_data_processor_(BSETBTDataProcessor::GetUniqueInstance(dbglogger, daemon_mode, p_ccm_livesource)),
        bse_tbt_data_common_struct_(NULL),
	segment_to_token_seqno_map_(),
        clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
        using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()),
        bse_spot_msg_seq_no(0),
        is_combined_source_mode_(HFSAT::kComShm == daemon_mode_),
        bardata_period_(60),
        bardata_time_(-1) {

    if (HFSAT::kComShm == daemon_mode) {
      bse_tbt_data_common_struct_ = bse_tbt_data_processor_.GetBSECommonStructFromGenericPacket();
    } else {
      bse_tbt_data_common_struct_ = new EOBI_MDS::EOBICommonStruct();
    }

    //segment_to_token_seqno_map_.clear();
    //segment_to_token_seqno_map_['E'].insert(std::make_pair(543335,3029757));
    //segment_to_token_seqno_map_['E'][543335] = 3029757;
    //std::cout << "CONSTRUCTOR MAP: " << segment_to_token_seqno_map_['E'][543335] << std::endl;
  }

  BSETBTDataDecoder(BSETBTDataDecoder const& disabled_copy_constructor);

 public:
 
  void SetBardataPeriod(uint32_t bardata_period) {
    bardata_period_ = bardata_period;
  }

  // To ensure there is only 1 decoder instance of the class in a program
  static BSETBTDataDecoder& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                              HFSAT::FastMdConsumerMode_t const& daemon_mode,
                                              HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource) {
    static BSETBTDataDecoder unqiue_instance(dbglogger, daemon_mode, p_ccm_livesource);
    return unqiue_instance;
  }

  int update_seq_num(){

    //if(segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type].find(bse_tbt_data_common_struct_->token_) == 
	//segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type].end()){
    if(segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] <= 0) {
/*
      DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING DATA FOR TOKEN : " << bse_tbt_data_common_struct_->token_ 
                                  << " STARTING SEQUENCE IS : " << bse_tbt_data_common_struct_->order_.msg_seq_num_
				  << " MAP VALUE: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_]
				  << DBGLOG_ENDL_NOFLUSH;
*/
      segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type].insert(std::make_pair(bse_tbt_data_common_struct_->token_,bse_tbt_data_common_struct_->order_.msg_seq_num_));


    }else if(segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] >= bse_tbt_data_common_struct_->order_.msg_seq_num_){
      if(0 != bse_tbt_data_common_struct_->order_.msg_seq_num_){

          DBGLOG_CLASS_FUNC_LINE_ERROR << "DUPLICATE PACKETS FOR TOKEN : " << bse_tbt_data_common_struct_->token_
                                       << " RECEVIED MSGSEQ IS : " << bse_tbt_data_common_struct_->order_.msg_seq_num_
				       << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_]) 
                                       << "SEGMENT TYPE: " << bse_tbt_data_common_struct_->segment_type
				       << " MAP VALUE: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_]
				       << DBGLOG_ENDL_NOFLUSH;

      }
      return -1;
    } else if ((1 + segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_]) == bse_tbt_data_common_struct_->order_.msg_seq_num_){

      segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] = bse_tbt_data_common_struct_->order_.msg_seq_num_;

//      DBGLOG_CLASS_FUNC_LINE_ERROR << "GOT IT:  MAP VALUE: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << DBGLOG_ENDL_NOFLUSH;

    }else {
	struct timeval drop_time;
        gettimeofday(&drop_time,NULL);


        DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FOR TOKEN : " << bse_tbt_data_common_struct_->token_
                                     << " RECEVIED MSGSEQ IS : " << bse_tbt_data_common_struct_->order_.msg_seq_num_ << " EXPECTED SEQUENCE NUMBER WAS : "
                                     << (1 + segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_])
                                     << " DROP OF : " << ( bse_tbt_data_common_struct_->order_.msg_seq_num_ -  (1 + segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_]) )
                                     << " JUMPING SEQ AND CONTINUING... " 
				     << drop_time.tv_sec << "." << drop_time.tv_usec << DBGLOG_ENDL_NOFLUSH;

       
      segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] = bse_tbt_data_common_struct_->order_.msg_seq_num_;

    }
    return 0;
  }

  // Main function
  inline bool DecodeEvents(char const* data_buffer, int32_t const& msg_seq_no, char const& segment_marking) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "Decoding starts..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;
    // This variable will give an idea to raw data source handler on how many packets are there in a single UDP message
    // which are re-directed again here for decoding if read_length - this variable returns a positive number
    uint16_t template_id_ = *(uint16_t*)(data_buffer + 2);
    bse_tbt_data_common_struct_->segment_type = segment_marking;
    if (true == using_simulated_clocksource_) {
      bse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&bse_tbt_data_common_struct_->source_time, NULL);
    }

    switch (template_id_) {
      case ORDER_ADD: {
	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 16);
        bse_tbt_data_processor_.o_action_ = '0';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_trd_reg_ts_ = *(uint64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 32);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 36);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 40);
        bse_tbt_data_processor_.o_intermediate_ = false;

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.intermediate_ = bse_tbt_data_processor_.o_intermediate_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;
        bse_tbt_data_common_struct_->order_.trd_reg_ts = bse_tbt_data_processor_.o_trd_reg_ts_;


/*
        char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "ORDER_ADD: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  " 
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << "TRD_REG_TS: " << bse_tbt_data_processor_.o_trd_reg_ts_ << "\n"
		  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
		  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
		  << "SIDE: " << t_side_ << "\n"
		  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
  		  << std::endl;
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
        // Packed and sent further to processor
        // need to shift this to processor
        /*
        if (true == using_simulated_clocksource_) {
          nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
        } else {
          gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
        }
*/
//        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_ADD " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;
       
      case ORDER_MODIFY: {
	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 40);
	bse_tbt_data_processor_.o_action_ = '1';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_trd_reg_ts_ = *(uint64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_prev_priority_ts_ = *(uint64_t *)(data_buffer + 16);
        bse_tbt_data_processor_.o_prev_price_ = *(int64_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_prev_size_ = *(int32_t *)(data_buffer + 32);
        bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 48);
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 56);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 60);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 64);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;
        bse_tbt_data_common_struct_->order_.prev_price = bse_tbt_data_processor_.o_prev_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.prev_size = bse_tbt_data_processor_.o_prev_size_;
        bse_tbt_data_common_struct_->order_.prev_priority_ts = bse_tbt_data_processor_.o_prev_priority_ts_;
        bse_tbt_data_common_struct_->order_.trd_reg_ts = bse_tbt_data_processor_.o_trd_reg_ts_;


/*
        char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "ORDER_MODIFY: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << " "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
                  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
                  << "TRD_REG_TS: " << bse_tbt_data_processor_.o_trd_reg_ts_ << "\n"
                  << "PREV_PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
                  << "PREV_PRICE: " << bse_tbt_data_processor_.o_prev_price_ / EOBI_PRICE_DIVIDER << "\n"
                  << "PREV_SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
                  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
                  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
                  << "SIDE: " << t_side_ << "\n"
                  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
                  << std::endl;
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_MODIFY " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case ORDER_MODIFY_SAME_PRIORITY: {
        bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 32);
        bse_tbt_data_processor_.o_action_ = '1';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_trd_reg_ts_ = *(uint64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_prev_size_ = *(int32_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 40);
        bse_tbt_data_processor_.o_prev_priority_ts_ = *(uint64_t *)(data_buffer + 40);
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 48);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 52);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 56);
        bse_tbt_data_processor_.o_prev_price_ = *(int64_t *)(data_buffer + 56);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;
        bse_tbt_data_common_struct_->order_.prev_price = bse_tbt_data_processor_.o_prev_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.prev_size = bse_tbt_data_processor_.o_prev_size_;
        bse_tbt_data_common_struct_->order_.prev_priority_ts = bse_tbt_data_processor_.o_prev_priority_ts_;
        bse_tbt_data_common_struct_->order_.trd_reg_ts = bse_tbt_data_processor_.o_trd_reg_ts_;



/*
        char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "ORDER_MODIFY_SAME_PRIORITY: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << " "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
                  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
                  << "TRD_REG_TS: " << bse_tbt_data_processor_.o_trd_reg_ts_ << "\n"
                  << "PREV_PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
                  << "PREV_PRICE: " << bse_tbt_data_processor_.o_prev_price_ / EOBI_PRICE_DIVIDER << "\n"
                  << "PREV_SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
                  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
                  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
                  << "SIDE: " << t_side_ << "\n"
                  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
                  << std::endl;
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_MODIFY_SAME_PRIORITY " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case ORDER_DELETE: {

        bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_action_ = '2';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_trd_reg_ts_ = *(uint64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 32);
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 40);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 44);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 48);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;
        bse_tbt_data_common_struct_->order_.trd_reg_ts = bse_tbt_data_processor_.o_trd_reg_ts_;



/*
        char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "ORDER_DELETE: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << "TRD_REG_TS: " << bse_tbt_data_processor_.o_trd_reg_ts_ << "\n"
		  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
		  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
		  << "SIDE: " << t_side_ << "\n"
		  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
  		  << std::endl;
*/   
 
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_DELETE " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case ORDER_MASS_DELETE: {
        bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_action_ = '3';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;

/*
        std::cout << "ORDER_MASS_DELETE: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << std::endl;
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "ORDER_MASS_DELETE " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case PARTIAL_EXECUTION: {
 	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 32);
	bse_tbt_data_processor_.o_action_ = '4';
	bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
	bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 8);
	bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 24);
	bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 44);
	bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 48);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;


/*
        char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "PARTIAL_EXECUTION: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
		  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
		  << "SIDE: " << t_side_ << "\n"
		  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
  		  << std::endl;
*/


/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "PARTIAL_EXECUTION " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;


      case FULL_EXECUTION: {
	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 32);
        bse_tbt_data_processor_.o_action_ = '5';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 44);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 48);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;


/*
        char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "FULL_EXECUTION: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
		  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
		  << "SIDE: " << t_side_ << "\n"
		  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
  		  << std::endl;
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "FULL_EXECUTION " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case EXECUTION_SUMMARY: {
	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_action_ = '6';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_trd_reg_ts_ = *(uint64_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_prev_priority_ts_ = *(uint64_t *)(data_buffer + 16); // Storing AggressorTimestamp
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 32); //40);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 36); //44);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 40); //48);
        bse_tbt_data_processor_.o_synthetic_match_ = (int16_t)(*(uint8_t *)(data_buffer + 37));
        bse_tbt_data_processor_.o_prev_price_ = (double)(*(int32_t *)(data_buffer + 48)); // Storing Hidden Quantity in Prev Price

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;
        bse_tbt_data_common_struct_->order_.prev_size = (int)bse_tbt_data_processor_.o_synthetic_match_;  // Hack to get synthetic match
        bse_tbt_data_common_struct_->order_.prev_priority_ts = bse_tbt_data_processor_.o_prev_priority_ts_; // Storing AggressorTimestamp
        bse_tbt_data_common_struct_->order_.prev_price = bse_tbt_data_processor_.o_prev_price_;  // Storing Hidden Quantity in Prev Price
        bse_tbt_data_common_struct_->order_.trd_reg_ts = bse_tbt_data_processor_.o_trd_reg_ts_;


/*
	char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
        std::cout << "EXECUTION_SUMMARY: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << " "
		  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << "TRD_REG_TS: " << bse_tbt_data_processor_.o_trd_reg_ts_ << "\n"
		  << "AGGRESSOR_TS: " << bse_tbt_data_processor_.o_prev_priority_ts_ << "\n"
		  << "SYNTHETIC_MATCH: " << bse_tbt_data_processor_.o_synthetic_match_ << "\n" 
		  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
		  << "SIDE: " << t_side_ << "\n"
		  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
                  << "RESTINGHIDDENQTY: " << bse_tbt_data_processor_.o_prev_price_ << " : " << bse_tbt_data_common_struct_->order_.prev_price 
                  << " : " << *(int32_t *)(data_buffer + 48) << "\n"
  		  << std::endl;
    
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "EXECUTION_SUMMARY " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case INSTRUMENT_INFO: {
	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_action_ = '7';
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_size_ = (int32_t)(*(int64_t *)(data_buffer + 16) / EOBI_PRICE_DIVIDER * 100);  // Storing Close Price in Size
        bse_tbt_data_processor_.o_prev_size_ = (int32_t)(*(int64_t *)(data_buffer + 24) / EOBI_PRICE_DIVIDER * 100);  // Storing Prev Close Price in Prev Size
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 32);  // Storing UpperCktLimit in price
        bse_tbt_data_processor_.o_prev_price_ = *(int64_t *)(data_buffer + 40); // Storing LowerCktLimit in prev_price 

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;  // Storing UpperCktLimit in price
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;  // Storing Close Price in Size
        bse_tbt_data_common_struct_->order_.prev_size = bse_tbt_data_processor_.o_prev_size_;  // Storing Prev Close Price in Prev Size
        bse_tbt_data_common_struct_->order_.prev_price = bse_tbt_data_processor_.o_prev_price_ / EOBI_PRICE_DIVIDER;  // Storing LowerCktLimit in prev_price


/*
        std::cout << "INSTRUMENT_INFO: \t\t TOKEN: " << bse_tbt_data_processor_.o_security_id_ << "  "
		  << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(bse_tbt_data_processor_.o_security_id_, segment_marking) << " "
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << "\n"
		  << "CLOSE_PRICE: " << bse_tbt_data_processor_.o_size_ << " : " << bse_tbt_data_common_struct_->order_.size << "\n"
                  << "PREV_CLOSE_PRICE: " << bse_tbt_data_processor_.o_prev_size_ << " : " << bse_tbt_data_common_struct_->order_.prev_size << "\n"
                  << "UPPER_SKT_LIMIT: " << bse_tbt_data_processor_.o_price_ << " : " << bse_tbt_data_common_struct_->order_.price << "\n"
                  << "LOWER_SKT_LIMIT: " << bse_tbt_data_processor_.o_prev_price_ << " : " << bse_tbt_data_common_struct_->order_.prev_price << "\n"
  		  << std::endl;
    
*/
/* //Recovery part
        if (eobi_md_processor_.initial_recovery_not_finished_) {
          eobi_md_processor_.CheckSequenceNum();
	  return;
        }

*/      
//        DBGLOG_CLASS_FUNC_LINE_INFO << "INSTRUMENT_INFO " << DBGLOG_ENDL_NOFLUSH;
//	if (update_seq_num() != -1) { 
          bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
//        }
      } break;

      case PRODUCT_SUMMARY: {
	uint32_t seq_n = *(uint32_t *)(data_buffer + 8);
	// uint32_t last_msg_seq_n = *(uint32_t *)(data_buffer + 12);
	bse_tbt_data_processor_.mkt_seg_id_to_seq_num_[bse_tbt_data_processor_.current_mkt_seg_id_] = seq_n;
/*
	std::cout << "PRODUCT_SUMMARY: CUR_MKT_SEG_ID: " << bse_tbt_data_processor_.current_mkt_seg_id_ << " " 
		  << "SEQ: " << seq_n << " LAST_SEQ_N: " << last_msg_seq_n
		  << std::endl;
*/
      } break;

//This is normal snapshot data for recovery purpose which we get after INSTRUMENT_SUMMARY msg type. 
//Treat this msg as ORDEADD for building the Order Book.
      case SNAPSHOT_ORDER: {
        int64_t security_id_ = bse_tbt_data_processor_.curr_sec_id_[bse_tbt_data_processor_.current_mkt_seg_id_];

/*
        std::cout << "SNAPSHOT_ORDER: \t\t TOKEN: " << security_id_ << "  "
                  << "SEQ_NO: " << *(uint32_t *)(data_buffer + 4) << std::endl;
*/

	//checks if security is in recovery or not
	if (!bse_tbt_data_processor_.security_in_recovery[security_id_]) return false;

//        std::cout << "SNAPSHOT_ORDER SUMMARY DATA CHECK" << std::endl;
	//checks if INSTRUMENT_SUMMARY recovery data type is received or not
	if (!bse_tbt_data_processor_.instr_summry_rcvd_[security_id_]) return false;

	//continues if security is in recovery and INSTRUMENT_SUMMARY is received
        bse_tbt_data_processor_.o_action_ = '0'; // treat all snapsot order as ORDERADD as it provides the updated details
        bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
        bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 8);
        bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 16);
        bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 20);
        bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 24);
        bse_tbt_data_processor_.o_security_id_ = security_id_;
        bse_tbt_data_processor_.o_intermediate_ = true;
        bse_tbt_data_processor_.o_trd_reg_ts_ = 0;

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.intermediate_ = bse_tbt_data_processor_.o_intermediate_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;
        bse_tbt_data_common_struct_->order_.trd_reg_ts = bse_tbt_data_processor_.o_trd_reg_ts_;

       // char t_side_ = bse_tbt_data_processor_.o_side_ == 1 ? 'B': 'S';
/*
	std::cout << "SNAPSHOT_ORDER: SEC_ID: " << security_id_ << "\n" 
		  << "SEQ_NO: " << bse_tbt_data_processor_.o_seq_num_ << "\n"
	//	  << "CURR_SEQ_NO: " << segment_to_token_seqno_map_[bse_tbt_data_common_struct_->segment_type][bse_tbt_data_common_struct_->token_] << "\n"
		  << "TRD_REG_TS: " << bse_tbt_data_processor_.o_trd_reg_ts_ << "\n"
		  << "PRIORITY: " << bse_tbt_data_processor_.o_priority_ts_ << "\n"
		  << "SIZE: " << bse_tbt_data_processor_.o_size_ << "\n"
		  << "SIDE: " << t_side_ << "\n"
		  << "PRICE: " << bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER << "\n"
		  << std::endl;
*/

        bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);

	if (bse_tbt_data_processor_.o_seq_num_ == bse_tbt_data_processor_.target_msg_seq_num_[security_id_]) {
//          std::cout << "SNAPSHOT_ORDER: SEQNO: " << security_id_ << " : " << bse_tbt_data_processor_.o_seq_num_ << " : " << bse_tbt_data_processor_.target_msg_seq_num_[security_id_] << std::endl;
	  bse_tbt_data_processor_.security_in_recovery[security_id_] = false;
	  bse_tbt_data_processor_.instr_summry_rcvd_[security_id_] = true;
	}
      } break;

//This case tells how many snapshot order will be there through total_orders_
      case INSTRUMENT_SUMMARY: {
        timeval current_time;
        gettimeofday(&current_time, NULL);
 //       std::cout << "INSTRUMENT_SUMMARY : " << ctime(&(current_time.tv_sec)) << std::endl;
        uint32_t seq_no_ = *(uint32_t *)(data_buffer + 4);
        int64_t security_id_ = *(int64_t *)(data_buffer + 8);
        uint16_t total_orders_ = *(uint16_t *)(data_buffer + 32);


//	std::cout << "INSTRUMENT_SUMMARY: TOKEN : " << security_id_ << " "
// 		  << "SEQ_NO: " << seq_no_ << " INSTRUMENT_SUMMARY RECOVERY CHECK: " << bse_tbt_data_processor_.security_in_recovery[security_id_] << std::endl;

	if (!bse_tbt_data_processor_.security_in_recovery[security_id_]) return false;

        bse_tbt_data_processor_.target_msg_seq_num_[security_id_] = seq_no_ + total_orders_;
        bse_tbt_data_processor_.curr_sec_id_[bse_tbt_data_processor_.current_mkt_seg_id_] = security_id_;
        bse_tbt_data_processor_.instr_summry_rcvd_[security_id_] = true;

//	std::cout << "INSTRUMENT_SUMMARY: SEC_ID : " << security_id_ << " SEQ_NO: " << seq_no_ << " TOTAL ORDERS: " << total_orders_ << std::endl;
	
	//if no orders are there then we will wait for next Instrument summary to get details
  	if (total_orders_ == 0) {
//         std::cout << "total_orders_0: " << bse_tbt_data_processor_.security_in_recovery[security_id_] << " : " << bse_tbt_data_processor_.instr_summry_rcvd_[security_id_] << std::endl;
	  bse_tbt_data_processor_.security_in_recovery[security_id_] = true;
	  bse_tbt_data_processor_.instr_summry_rcvd_[security_id_] = false;
	}

      } break;

      case HEARTBEAT_BSE: {
/*
        uint32_t msg_seq_num_h = *(uint32_t *)(data_buffer + 4);
        uint32_t h_seq_num_ = *(uint32_t *)(data_buffer + 8);
 	
        std::cout << "HEARTBEAT_BSE: msg_seq_num_h: " << msg_seq_num_h << " Lst_msg_Seq_num_: " << h_seq_num_ << std::endl;
*/	
      } break;
      default: {
        //std::cout << "Unhandled template_id: " << template_id_ << std::endl;
        // TODO Register Error and return message length from nse packet

      } break;
    }

//    std::cout << "DecodeEvents: template_id_: " << template_id_ << " DONE***" << std::endl;
    //return processed_length;
    return false;
  }

  inline bool DecodeBardataEvents(char const* data_buffer, int32_t const& msg_seq_no, char const& segment_marking) {
    uint16_t template_id_ = *(uint16_t*)(data_buffer + 2);
    bse_tbt_data_common_struct_->segment_type = segment_marking;

    switch(template_id_) {
      case PARTIAL_EXECUTION:
      case FULL_EXECUTION: {
 	bse_tbt_data_processor_.o_security_id_ = *(int64_t *)(data_buffer + 32);
	bse_tbt_data_processor_.o_seq_num_ = *(uint32_t *)(data_buffer + 4);
	bse_tbt_data_processor_.o_side_ = *(uint8_t *)(data_buffer + 8);
	bse_tbt_data_processor_.o_priority_ts_ = *(uint64_t *)(data_buffer + 24);
	bse_tbt_data_processor_.o_size_ = *(int32_t *)(data_buffer + 44);
	bse_tbt_data_processor_.o_price_ = *(int64_t *)(data_buffer + 48);

        bse_tbt_data_common_struct_->order_.action_ = bse_tbt_data_processor_.o_action_;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = bse_tbt_data_processor_.o_seq_num_;
        bse_tbt_data_common_struct_->token_ = bse_tbt_data_processor_.o_security_id_;
        bse_tbt_data_common_struct_->order_.price = bse_tbt_data_processor_.o_price_ / EOBI_PRICE_DIVIDER;
        bse_tbt_data_common_struct_->order_.priority_ts = bse_tbt_data_processor_.o_priority_ts_;
        bse_tbt_data_common_struct_->order_.side = bse_tbt_data_processor_.o_side_ == 1 ? 'B' : 'S';
        bse_tbt_data_common_struct_->order_.size = bse_tbt_data_processor_.o_size_;

        // int temp_bardata_time_ = bse_tbt_data_common_struct_->source_time.tv_sec - (bse_tbt_data_common_struct_->source_time.tv_sec % bardata_period_);

        if (true == using_simulated_clocksource_) {
          bse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
        } else {
          gettimeofday(&bse_tbt_data_common_struct_->source_time, NULL);
        }

        int temp_bardata_time_ = bse_tbt_data_common_struct_->source_time.tv_sec - (bse_tbt_data_common_struct_->source_time.tv_sec % bardata_period_);
        if (temp_bardata_time_ > bardata_time_) {
          if (HFSAT::kBardataLogger == daemon_mode_) {
            bse_tbt_data_processor_.DumpBardata();
            std::cout << "Case:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
          }
          else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
            bse_tbt_data_processor_.AddBardataToRecoveryMap();
            std::cout << "Case:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
          }
          bardata_time_ = temp_bardata_time_;
        }
 
        bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);
      } break;
      default: {
        // TODO Register Error and return message length from nse packet
        struct timeval source_time;
        if (true == using_simulated_clocksource_) {
          source_time = clock_source_.GetTimeOfDay();
        } else {
          gettimeofday(&source_time, NULL);
        }

        int temp_bardata_time_ = source_time.tv_sec - (source_time.tv_sec % bardata_period_);

        if (temp_bardata_time_ > bardata_time_) {
          if (HFSAT::kBardataLogger == daemon_mode_) {
            bse_tbt_data_processor_.DumpBardata();
            std::cout << "Default:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
          }
          else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
            bse_tbt_data_processor_.AddBardataToRecoveryMap();
            std::cout << "Default:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
          }
          bardata_time_ = temp_bardata_time_;
        }
      } break;
    }
    return false;
  }

  inline void DecodeSpotIndexValue(const unsigned char* buffer, char const& segment) {
    int32_t msg_type = 0;
    memcpy((void*)&msg_type, (void*)buffer, sizeof(int32_t));
    int32_t swapped_msg_type = HFSAT::BSEMD::swapEndian32(msg_type);

    //std::cout << " Msg Type : " <<msg_type << " Swapped Msg Type: " << swapped_msg_type << std::endl;   

    if((swapped_msg_type!=2011) && (swapped_msg_type!=2012))
        return;

    bse_tbt_data_common_struct_->segment_type = segment;
    
    uint16_t total_records = 0;

    memcpy((void*)&total_records, (char*)buffer + 26, sizeof(int16_t));
     
    total_records = HFSAT::BSEMD::swapEndian16(total_records);

    int records_no = total_records;

    records_no = std::min(records_no,24);

    if (true == using_simulated_clocksource_) {
      bse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&bse_tbt_data_common_struct_->source_time, NULL);
    }

    int temp_bardata_time_ = bse_tbt_data_common_struct_->source_time.tv_sec - (bse_tbt_data_common_struct_->source_time.tv_sec % bardata_period_);
    if (temp_bardata_time_ > bardata_time_) {
      if (HFSAT::kBardataLogger == daemon_mode_) {
        bse_tbt_data_processor_.DumpBardata();
        std::cout << "DecodeSpotIndexValue:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
      }
      else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
        bse_tbt_data_processor_.AddBardataToRecoveryMap();
        std::cout << "DecodeSpotIndexValue:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
      }
      bardata_time_ = temp_bardata_time_;
    }

    for(int i=0;i<records_no;i++){
        //std::cout<<"Record number : "<<i<<std::endl;
        uint32_t index_code = 0;
        uint32_t index_val = 0;
        /*
        TEMP
        memcpy((void*)&index_val, (void*)buffer+48+40*i, sizeof(int32_t));
        memcpy((void*)&index_code, (void*)buffer+28+40*i, sizeof(int32_t));
        */
        memcpy((void*)&index_val, (char*)buffer + 48 + 40*i, sizeof(int32_t));
        memcpy((void*)&index_code, (char*)buffer + 28 + 40*i, sizeof(int32_t));
        //index_val = swapEndian32(index_val);
        //std::cout<<"UnixTime : "<<unixTime_ist<<" Microseconds : "<< msec*1000<< " Price : " << HFSAT::BSEMD::swapEndian32(index_val) << " Index_code/symbol : " << HFSAT::BSEMD::swapEndian32(index_code) <<std::endl;
        // EOBI_MDS::EOBICommonStruct* next_event_ = new EOBI_MDS::EOBICommonStruct();
        bse_tbt_data_common_struct_->segment_type='S';
        bse_tbt_data_common_struct_->token_ = HFSAT::BSEMD::swapEndian32(index_code);
        bse_tbt_data_common_struct_->order_.action_='8';
        bse_tbt_data_common_struct_->order_.size=0;
        bse_tbt_data_common_struct_->order_.side=0;
        bse_tbt_data_common_struct_->order_.price = HFSAT::BSEMD::swapEndian32(index_val);
        bse_tbt_data_common_struct_->order_.price=(bse_tbt_data_common_struct_->order_.price)/100;
        
        uint32_t msg_seq_no = bse_spot_msg_seq_no++;
        bse_tbt_data_common_struct_->order_.msg_seq_num_ = msg_seq_no;

        //std::cout << "TOKEN NUMBER " << bse_tbt_data_common_struct_->token_ << std::endl;
        //std::cout << "INDEX VALUE " <<   bse_tbt_data_common_struct_->order_.price << std::endl;

        // if(bse_tbt_data_common_struct_->token_==1){
        //         bulk_file_writer_sensex.Write(bse_tbt_data_common_struct_, sizeof(EOBI_MDS::EOBICommonStruct));
        //         bulk_file_writer_sensex.CheckToFlushBuffer();
        // }
        // else if(bse_tbt_data_common_struct_->token_==12){
        //         bulk_file_writer_bankex.Write(bse_tbt_data_common_struct_, sizeof(EOBI_MDS::EOBICommonStruct));
        //         bulk_file_writer_bankex.CheckToFlushBuffer();
        // }
        // else{
        //         bulk_file_writer.Write(bse_tbt_data_common_struct_, sizeof(EOBI_MDS::EOBICommonStruct));
        //         bulk_file_writer.CheckToFlushBuffer();
        // }
        //std::cout << "bse_tbt_data_common_struct_->token_: " << bse_tbt_data_common_struct_->token_ << " type: " << bse_tbt_data_common_struct_->segment_type << std::endl;
        bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);                
    }
  }

  inline void DecodeOIIndexValue(const unsigned char* buffer, char const& segment) {
    int32_t msg_type = 0;
    memcpy((void*)&msg_type, (void*)buffer, sizeof(int32_t));
    int32_t swapped_msg_type = HFSAT::BSEMD::swapEndian32(msg_type);

    //std::cout << " Msg Type : " <<msg_type << " Swapped Msg Type: " << swapped_msg_type << std::endl;   

    if(swapped_msg_type!=2015)
        return;

    bse_tbt_data_common_struct_->segment_type = segment;
    
    uint16_t total_records = 0;

    memcpy((void*)&total_records, (char*)buffer + 26, sizeof(int16_t));
     
    total_records = HFSAT::BSEMD::swapEndian16(total_records);

    int records_no = total_records;

    records_no = std::min(records_no,24);

    if (true == using_simulated_clocksource_) {
      bse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&bse_tbt_data_common_struct_->source_time, NULL);
    }

    int temp_bardata_time_ = bse_tbt_data_common_struct_->source_time.tv_sec - (bse_tbt_data_common_struct_->source_time.tv_sec % bardata_period_);
    if (temp_bardata_time_ > bardata_time_) {
      if (HFSAT::kBardataLogger == daemon_mode_) {
        bse_tbt_data_processor_.DumpBardata();
        std::cout << "DecodeOIIndexValue:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
      }
      else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
        bse_tbt_data_processor_.AddBardataToRecoveryMap();
        std::cout << "DecodeOIIndexValue:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
      }
      bardata_time_ = temp_bardata_time_;
    }

    for(int i=0;i<records_no;i++){
      //std::cout<<"Record number : "<<i<<std::endl;
      uint32_t instr_id = 0;
      uint32_t oi_qty = 0;
      double oi_val = 0;
      uint32_t oi_chg = 0;
      /*
      TEMP
      memcpy((void*)&oi_qty, (void*)buffer+32+36*i, sizeof(int32_t));
      memcpy((void*)&instr_id, (void*)buffer+28+36*i, sizeof(int32_t));
      memcpy((void*)&oi_val, (void*)buffer+36+36*i, sizeof(double));
      memcpy((void*)&oi_chg, (void*)buffer+44+36*i, sizeof(int32_t));
      */
      memcpy((void*)&oi_qty, (char*)buffer + 32 + 36*i, sizeof(int32_t));
      memcpy((void*)&instr_id, (char*)buffer + 28 + 36*i, sizeof(int32_t));
      memcpy((void*)&oi_val, (char*)buffer + 36 + 36*i, sizeof(double));
      memcpy((void*)&oi_chg, (char*)buffer + 44 + 36*i, sizeof(int32_t));

      //index_val = swapEndian32(index_val);
      double oi_qty_double = HFSAT::BSEMD::swapEndian32(oi_qty);
      oi_qty_double = oi_qty_double * 0.1;
      oi_qty = oi_qty_double;
      //std::cout<<"UnixTime : "<<unixTime_ist<<" Microseconds : "<< msec*1000<< " instr_id : " << HFSAT::BSEMD::swapEndian32(instr_id) << " oi_qty : " << oi_qty <<std::endl;
      // EOBI_MDS::EOBICommonStruct* next_event_ = new EOBI_MDS::EOBICommonStruct();
      //HFSAT::BSEMD::swapEndianDouble(oi_val)<<std::endl;
      //std::cout << "Swapped OI val : " << oi_val;
      bse_tbt_data_common_struct_->segment_type='F';
      bse_tbt_data_common_struct_->token_ = HFSAT::BSEMD::swapEndian32(instr_id);
      bse_tbt_data_common_struct_->order_.action_='9';
      bse_tbt_data_common_struct_->order_.size=oi_qty;
      bse_tbt_data_common_struct_->order_.side=0;
      //bse_tbt_data_common_struct_->order_.price = oi_val;
      //bse_tbt_data_common_struct_->order_.price=(bse_tbt_data_common_struct_->order_.price)/100;
      
      uint32_t msg_seq_no = bse_spot_msg_seq_no++;
      bse_tbt_data_common_struct_->order_.msg_seq_num_ = msg_seq_no;
      // if(bse_tbt_data_common_struct_->token_==1){
      //         bulk_file_writer_sensex.Write(bse_tbt_data_common_struct_, sizeof(EOBI_MDS::EOBICommonStruct));
      //         bulk_file_writer_sensex.CheckToFlushBuffer();
      // }
      // else if(bse_tbt_data_common_struct_->token_==12){
      //         bulk_file_writer_bankex.Write(bse_tbt_data_common_struct_, sizeof(EOBI_MDS::EOBICommonStruct));
      //         bulk_file_writer_bankex.CheckToFlushBuffer();
      // }
      // else{
      //         bulk_file_writer.Write(bse_tbt_data_common_struct_, sizeof(EOBI_MDS::EOBICommonStruct));
      //         bulk_file_writer.CheckToFlushBuffer();
      // }
      //std::cout << "bse_tbt_data_common_struct_->token_: " << bse_tbt_data_common_struct_->token_ << " type: " << bse_tbt_data_common_struct_->segment_type << std::endl;
      bse_tbt_data_processor_.OnMarketEvent(bse_tbt_data_common_struct_);        
    }
  }

};
}
}
