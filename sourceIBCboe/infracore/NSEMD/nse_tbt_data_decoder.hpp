// =====================================================================================
//
//       Filename:  nse_tbt_data_decoder.hpp
//
//    Description:  This class decodes the binary stream recevied via socket for NSE TBT data,
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

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "infracore/NSEMD/nse_tbt_data_processor.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/nse_refdata_loader.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include <lzo/lzoconf.h>
#include <lzo/lzo1z.h>

#define IN_LEN (128 * 1024L)
#define OUT_LEN (IN_LEN + IN_LEN / 16 + 64 + 3)
#define HEAP_ALLOC(var, size) lzo_align_t __LZO_MMODEL var[((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)]
static HEAP_ALLOC(wrkmem, LZO1Z_999_MEM_COMPRESS);

static unsigned char __LZO_MMODEL out_lzo[OUT_LEN];

namespace HFSAT {
namespace NSEMD {

class NSETBTDataDecoder {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::FastMdConsumerMode_t daemon_mode_;
  // Events after decoding will be forwarded to this class and it will choose what to based on modes defined
  NSETBTDataProcessor& nse_tbt_data_processor_;
  // Decoded event
  NSE_MDS::NSETBTDataCommonStruct* nse_tbt_data_common_struct_;
  HFSAT::ClockSource& clock_source_;
  int bardata_time_;
  bool using_simulated_clocksource_;
  bool is_combined_source_mode_;
  bool is_raw_source_;
  uint32_t bardata_period_;

  // It's good to load and store this once as it's not going to be changed
  const SecurityNameIndexer& sec_name_indexer_;
  HFSAT::Utils::NSERefDataLoader& nse_refdata_loader_;
  std::map<char, std::map<int32_t, NSE_UDP_MDS::NSERefData>*> segment_to_nse_ref_data_;
  std::map<char, std::unordered_map<int32_t, int32_t> > segment_to_token_secid_map_;
  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler_;

  NSETBTDataDecoder(HFSAT::DebugLogger& dbglogger, HFSAT::FastMdConsumerMode_t const& daemon_mode,
                    HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource)
      : dbglogger_(dbglogger),
        daemon_mode_(daemon_mode),
        nse_tbt_data_processor_(NSETBTDataProcessor::GetUniqueInstance(dbglogger, daemon_mode, p_ccm_livesource)),
        nse_tbt_data_common_struct_(NULL),
        clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
        bardata_time_(-1),
        using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()),
        is_combined_source_mode_(HFSAT::kComShm == daemon_mode_),
        is_raw_source_(HFSAT::kRaw == daemon_mode_),
        bardata_period_(60),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        nse_refdata_loader_(HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger).GetTradingDate())),
        segment_to_nse_ref_data_(),
        segment_to_token_secid_map_(),
        nse_daily_token_symbol_handler_(
            HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())) {
    if (HFSAT::kComShm == daemon_mode) {
      nse_tbt_data_common_struct_ = nse_tbt_data_processor_.GetNSECommonStructFromGenericPacket();
    } else {
      nse_tbt_data_common_struct_ = new NSE_MDS::NSETBTDataCommonStruct();
    }

    if (lzo_init() != LZO_E_OK) {
      std::cerr << "lzo_init() failed. Exiting" << std::endl;
      exit(-1);
    }

    // Initialize
    segment_to_nse_ref_data_[NSE_EQ_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
    segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
    segment_to_nse_ref_data_[NSE_CD_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
    segment_to_nse_ref_data_[NSE_IX_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();

    std::map<int32_t, NSE_UDP_MDS::NSERefData>& eq_nse_ref_data = *(segment_to_nse_ref_data_[NSE_EQ_SEGMENT_MARKING]);
    std::map<int32_t, NSE_UDP_MDS::NSERefData>& fo_nse_ref_data = *(segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING]);
    std::map<int32_t, NSE_UDP_MDS::NSERefData>& cd_nse_ref_data = *(segment_to_nse_ref_data_[NSE_CD_SEGMENT_MARKING]);

    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_EQ_SEGMENT_MARKING)) {
      eq_nse_ref_data[itr.first] = itr.second;

      std::ostringstream internal_symbol_str;
      internal_symbol_str << "NSE"
                          << "_" << (itr.second).symbol;
      std::string exchange_symbol =
          NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
      int32_t security_id = -1;

      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        if (security_id >= 0)
          segment_to_token_secid_map_[NSE_EQ_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));
      }
    }

    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_CD_SEGMENT_MARKING)) {
      cd_nse_ref_data[itr.first] = itr.second;
      std::ostringstream internal_symbol_str;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "NSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "NSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);
      }
      std::string exchange_symbol =
          NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());
      int32_t security_id = -1;

      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        segment_to_token_secid_map_[NSE_CD_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));
      }
    }

    for (auto& itr : nse_refdata_loader_.GetNSERefData(NSE_FO_SEGMENT_MARKING)) {
      std::ostringstream internal_symbol_str;
      fo_nse_ref_data[itr.first] = itr.second;

      if (std::string("XX") == std::string((itr.second).option_type)) {
        internal_symbol_str << "NSE"
                            << "_" << (itr.second).symbol << "_FUT_"
                            << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);

      } else {
        internal_symbol_str << "NSE"
                            << "_" << (itr.second).symbol << "_" << (itr.second).option_type << "_";
        internal_symbol_str << std::fixed << std::setprecision(2) << (itr.second).strike_price;
        internal_symbol_str << "_" << HFSAT::Utils::ConvertNSEExpiryInSecToDate((itr.second).expiry);
      }
      std::string exchange_symbol =
          NSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

      int32_t security_id = -1;
      if (std::string("INVALID") != exchange_symbol) {
        security_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol.c_str());
        if (security_id >= 0)
          segment_to_token_secid_map_[NSE_FO_SEGMENT_MARKING].insert(std::make_pair(itr.first, security_id));
      }
    }

    std::unordered_map<std::string, int> spot_index_2_token_map_ =
        HFSAT::SpotTokenGenerator::GetUniqueInstance().GetSpotIndexToTokenMap();
    if (0 == spot_index_2_token_map_.size()) {
      std::cerr << "IX SEGMENT REF DATA NOT PRESENT" << std::endl;
      exit(-1);
    }

    for (auto& itr : spot_index_2_token_map_) {
      std::string exchange_sym = "NSE_IDX" + std::to_string(itr.second);
      int security_id = sec_name_indexer_.GetIdFromSecname(exchange_sym.c_str());
      // std::cout<<"SEC ID: " << security_id << " TOKEN: " << itr.second  << " STR: " << exchange_sym << std::endl;

      if (security_id >= 0)
        segment_to_token_secid_map_[NSE_IX_SEGMENT_MARKING].insert(
            std::make_pair(itr.second, security_id));  // currently wrong// need to check other things
    }
  }

  NSETBTDataDecoder(NSETBTDataDecoder const& disabled_copy_constructor);

 public:
  // To ensure there is only 1 decoder instance of the class in a program
  static NSETBTDataDecoder& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                              HFSAT::FastMdConsumerMode_t const& daemon_mode,
                                              HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource) {
    static NSETBTDataDecoder unqiue_instance(dbglogger, daemon_mode, p_ccm_livesource);
    return unqiue_instance;
  }

  void SetBardataPeriod(uint32_t bardata_period) { bardata_period_ = bardata_period; }

  void DecodeTradeExecutionHelper(const unsigned char* buffer) {
    // 8 bytes have to be ignored
    buffer += 8;

    int16_t trans_code = ntoh16(*((int16_t*)(buffer + 10)));
    // bcast header
    buffer += 40;

    if (trans_code == NSE_BCAST_TRADE_RANGE_TRANS_CODE) {
      int32_t msg_count = ntoh32(*((int32_t*)buffer));
      buffer += 4;

      while (msg_count > 0) {
        nse_tbt_data_common_struct_->token = ntoh32(*((uint32_t*)buffer));

        int sec_id = -1;
        if (true == is_raw_source_) {
          if (segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].find(
                  nse_tbt_data_common_struct_->token) !=
              segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].end()) {
            sec_id = segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type]
                                                [nse_tbt_data_common_struct_->token];

            if (sec_id >= 0) {
              nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band = ntoh32(*((uint32_t*)(buffer + 4)));
              nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band = ntoh32(*((uint32_t*)(buffer + 8)));
              nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);
            }
          }
        } else {
          nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band = ntoh32(*((uint32_t*)(buffer + 4)));
          nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band = ntoh32(*((uint32_t*)(buffer + 8)));
          nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);
        }
        buffer += 12;
        msg_count--;
      }
    }
  }

  void DecodeTradeExecutionRange(const unsigned char* buffer, char const& segment) {
    // Decoder Doc: FOTS_NNF_PROTOCOL -> Broadcast -> Trade Execution Range

    nse_tbt_data_common_struct_->segment_type = segment;
    nse_tbt_data_common_struct_->msg_type = NSE_MDS::MsgType::kNSETradeExecutionRange;

    // Packed and sent further to processor
    if (true == using_simulated_clocksource_) {
      nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
    }

    int16_t num_packets = (int16_t)ntoh16(*((int16_t*)(buffer + 2)));
    buffer += 4;

    while (num_packets > 0) {
      lzo_uint in_len;
      lzo_uint out_len;

      in_len = (int64_t)ntoh16(*((int16_t*)(buffer)));

      buffer += 2;

      // Data isn't compressed
      if (in_len == 0) {
        DecodeTradeExecutionHelper(buffer);
        int16_t msg_len = (int16_t)ntoh16(*((int16_t*)(buffer + 46)));
        buffer += (msg_len + 8);
      } else {
        lzo1z_decompress(buffer, in_len, out_lzo, &out_len, wrkmem);
        const unsigned char* tmp = out_lzo;
        DecodeTradeExecutionHelper(tmp);
        buffer += in_len;
      }

      num_packets--;
    }
  }

  inline void DecodeSpotIndexValue(const unsigned char* buffer, char const& segment) {
    uint32_t msg_seq_no = *((uint32_t*)((char*)(buffer + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET)));
    nse_tbt_data_common_struct_->segment_type = segment;
    nse_tbt_data_common_struct_->msg_type = NSE_MDS::MsgType::kNSESpotIndexUpdate;
    nse_tbt_data_common_struct_->msg_seq = msg_seq_no;
    // Packed and sent further to processor
    if (true == using_simulated_clocksource_) {
      nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
    }
    int16_t num_packets = (int16_t)ntoh16(*((int16_t*)(buffer + 2)));
    buffer += 4;
    while (num_packets > 0) {
      lzo_uint in_len;
      lzo_uint out_len;
      in_len = (int64_t)ntoh16(*((int16_t*)(buffer)));
      buffer += 2;

      // Data isn't compressed
      if (in_len == 0) {
        DecodeSpotIndexValueHelper(buffer, segment);
        int16_t msg_len = (int16_t)ntoh16(*((int16_t*)(buffer + 46)));
        buffer += (msg_len + 8);
      } else {
        lzo1z_decompress(buffer, in_len, out_lzo, &out_len, wrkmem);
        const unsigned char* tmp = out_lzo;
        DecodeSpotIndexValueHelper(tmp, segment);
        buffer += in_len;
      }
      num_packets--;
    }
  }

  inline void DecodeSpotIndexValueHelper(const unsigned char* buffer, char const& segment) {
    buffer += 8;
    int16_t trans_code = ntoh16(*((int16_t*)(buffer + 10)));
    buffer += 40;
    if (trans_code != MS_BCAST_INDICES_TRANS_CODE) return;

    if (true == using_simulated_clocksource_) {
      nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
    }

    int16_t msg_count = ntoh16(*((int16_t*)(buffer + 0)));  // 40 is already incremented
    buffer += 2;
    // std::cout << "MS_BCAST_INDICES_TRANS_CODE (7207) Code:" << std::endl;
    while (msg_count > 0) {
      char index_name[22];
      memcpy((void*)index_name, (void*)(buffer), 21);
      index_name[21] = '\0';
      string str(index_name);
      // std::replace(str.begin(), str.end(), ' ', '_');
      str = "NSE_" + str;
      str.erase(remove(str.begin(), str.end(), ' '), str.end());
      transform(str.begin(), str.end(), str.begin(), ::toupper);
      nse_tbt_data_common_struct_->token = HFSAT::SpotTokenGenerator::GetUniqueInstance().GetTokenOrUpdate(str);
      buffer += 1;  // byte align

      int sec_id = -1;
      if (true == is_raw_source_) {
        if (segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].find(
                nse_tbt_data_common_struct_->token) !=
            segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].end()) {
          sec_id = segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type]
                                              [nse_tbt_data_common_struct_->token];

          if (sec_id >= 0) {
            nse_tbt_data_common_struct_->data.nse_spot_index.IndexValue = 1;
            nse_tbt_data_common_struct_->data.nse_spot_index.IndexValue = ntoh32(*((uint32_t*)(buffer + 21)));
            nse_tbt_data_common_struct_->data.nse_spot_index.PercentChange = ntoh32(*((uint32_t*)(buffer + 41)));
            // std::cout << nse_tbt_data_common_struct_->ToString() << std::endl;
            nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);
          }
        }
      } else {
        nse_tbt_data_common_struct_->data.nse_spot_index.IndexValue = 1;
        nse_tbt_data_common_struct_->data.nse_spot_index.IndexValue = ntoh32(*((uint32_t*)(buffer + 21)));
        nse_tbt_data_common_struct_->data.nse_spot_index.PercentChange = ntoh32(*((uint32_t*)(buffer + 41)));
        // std::cout << nse_tbt_data_common_struct_->ToString() << std::endl;
        nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);
      }

      buffer += 71;
      msg_count--;
    }
  }

  void DecodeOpenInterestHelper(const unsigned char* buffer, char segment_type) {
    // 8 bytes have to be ignored
    buffer += 8;

    int16_t trans_code = ntoh16(*((int16_t*)(buffer + 10)));
    // int32_t LogTime = ntoh32(*((int32_t *)(buffer + 4)));
    // int16_t msg_length = ntoh16(*((int16_t *)(buffer + 38)));
    // int16_t NoOfRecords = ntoh16(*((int16_t *)(buffer + 12)));
    // memcpy((void *)timestamp,(void *)(buffer + 22),8);
    // bcast header
    buffer += 40;
    if (trans_code == BCAST_TICKER_AND_MKT_INDEX) {
      int32_t msg_count = ntoh16(*((int16_t*)buffer));
      buffer += 2;
      while (msg_count > 0) {
        nse_tbt_data_common_struct_->token = ntoh32(*((uint32_t*)buffer));

        int sec_id = -1;

        if (true == is_raw_source_) {
          if (segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].find(
                  nse_tbt_data_common_struct_->token) !=
              segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].end()) {
            sec_id = segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type]
                                                [nse_tbt_data_common_struct_->token];

            if (sec_id >= 0) {
              nse_tbt_data_common_struct_->data.nse_oi_data.MarketType = ntoh16(*((uint16_t*)(buffer + 4)));
              nse_tbt_data_common_struct_->data.nse_oi_data.FillPrice = ntoh32(*((uint32_t*)(buffer + 6)));
              nse_tbt_data_common_struct_->data.nse_oi_data.FillVolume = ntoh32(*((uint32_t*)(buffer + 10)));
              nse_tbt_data_common_struct_->data.nse_oi_data.OpenInterest = ntoh32(*((uint32_t*)(buffer + 14)));
              nse_tbt_data_common_struct_->data.nse_oi_data.DayHiOI = ntoh32(*((uint32_t*)(buffer + 18)));
              nse_tbt_data_common_struct_->data.nse_oi_data.DayLoOI = ntoh32(*((uint32_t*)(buffer + 22)));
              // std::cout <<nse_tbt_data_common_struct_->ToString();
              nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);
            }
          }
        } else {
          nse_tbt_data_common_struct_->data.nse_oi_data.MarketType = ntoh16(*((uint16_t*)(buffer + 4)));
          nse_tbt_data_common_struct_->data.nse_oi_data.FillPrice = ntoh32(*((uint32_t*)(buffer + 6)));
          nse_tbt_data_common_struct_->data.nse_oi_data.FillVolume = ntoh32(*((uint32_t*)(buffer + 10)));
          nse_tbt_data_common_struct_->data.nse_oi_data.OpenInterest = ntoh32(*((uint32_t*)(buffer + 14)));
          nse_tbt_data_common_struct_->data.nse_oi_data.DayHiOI = ntoh32(*((uint32_t*)(buffer + 18)));
          nse_tbt_data_common_struct_->data.nse_oi_data.DayLoOI = ntoh32(*((uint32_t*)(buffer + 22)));
        }
        buffer += 26;
        msg_count--;
      }
    }
  }

  inline void DecodeOpenInterestValue(const unsigned char* buffer, char const& segment) {
    // Decoder Doc: FOTS_NNF_PROTOCOL -> Broadcast -> Trade Execution Range
    // int msg_seq_no = *((int32_t*)((char*)(buffer + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET)));
    nse_tbt_data_common_struct_->segment_type = segment;
    nse_tbt_data_common_struct_->msg_type = NSE_MDS::MsgType::kNSEOpenInterestTick;
    // nse_tbt_data_common_struct_->msg_seq = msg_seq_no;
    // Packed and sent further to processor
    if (true == using_simulated_clocksource_) {
      nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
    }
    int16_t num_packets = (int16_t)ntoh16(*((int16_t*)(buffer + 2)));
    buffer += 4;
    while (num_packets > 0) {
      lzo_uint in_len;
      lzo_uint out_len;
      in_len = (int64_t)ntoh16(*((int16_t*)(buffer)));
      buffer += 2;

      // std::cout<<"numpackets: " << num_packets<<std::endl;
      // Data isn't compressed
      if (in_len == 0) {
        DecodeOpenInterestHelper(buffer, segment);
        int16_t msg_len = (int16_t)ntoh16(*((int16_t*)(buffer + 46)));
        buffer += (msg_len + 8);
      } else {
        lzo1z_decompress(buffer, in_len, out_lzo, &out_len, wrkmem);
        const unsigned char* tmp = out_lzo;
        DecodeOpenInterestHelper(tmp, segment);
        buffer += in_len;
      }
      num_packets--;
    }
  }
  // Main function
  inline int32_t DecodeEvents(char const* data_buffer, uint32_t const& msg_seq_no, char const& segment_marking) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "Decoding starts..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;
    // This variable will give an idea to raw data source handler on how many packets are there in a single UDP message
    // which are re-directed again here for decoding if read_length - this variable returns a positive number
    int32_t processed_length = 0;
    char message_type = data_buffer[NSE_TBT_DATA_MESSAGE_TYPE_OFFSET];

    switch (message_type) {
      case MASTER_DATA_HEADER_MESSAGE_TYPE:  // Masters Data/Ref Data header
      {
        processed_length = NSE_TBT_DATA_MASTER_INFO_HEADER_MESSAGE_LENGTH;
      } break;
      case MASTER_DATA_CONTRACT_SPEC_MESSAGE_TYPE:  // Masters Data Contract Specs
      {
        processed_length = NSE_TBT_DATA_MASTER_CONTRACT_INFO_MESSAGE_LENGTH;
      } break;
      case MASTER_DATA_SPREAD_CONTRACT_SPEC_MESSAGE_TYPE:  // Masters Data Spread Contract Specs
      {
        processed_length = NSE_TBT_DATA_MASTER_SPREAD_CONTRACT_INFO_HEADER_LENGTH;
      } break;
      case MASTER_DATA_TRAILER_MESSAGE_TYPE:  // Masters Data Trailer
      {
        processed_length = NSE_TBT_DATA_MASTER_TRAILER_MESSAGE_LENGTH;
      } break;

      case HEARTBEAT_MESSAGE_TYPE:  // Heartbeat
      {
        processed_length = NSE_TBT_DATA_HEARTBEAT_MESSAGE_LENGTH;
      } break;

      // All Order Messages Including Spreads
      case ORDER_MESSAGE_TYPE_NEW:
      case ORDER_MESSAGE_TYPE_MODIFY:
      case ORDER_MESSAGE_TYPE_DELETE:
      case SPREAD_ORDER_MESSAGE_TYPE_NEW:
      case SPREAD_ORDER_MESSAGE_TYPE_MODIFY:
      case SPREAD_ORDER_MESSAGE_TYPE_DELETE: {
        processed_length = NSE_TBT_DATA_ORDER_MESSAGE_LENGTH;
        nse_tbt_data_common_struct_->token =
            *((uint32_t*)((char*)(data_buffer + NSE_TBT_DATA_ORDER_MESSAGE_TOKEN_OFFSET)));
        // filtering _PE and _CE option data

        if (true == is_combined_source_mode_ &&
            (false == nse_tbt_data_processor_.IsProcessingSecurityForComShm(nse_tbt_data_common_struct_->token)))
          break;

        // Mark Segment
        nse_tbt_data_common_struct_->segment_type = segment_marking;

        int sec_id = -1;
        if (true == is_raw_source_) {
          if (segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].find(
                  nse_tbt_data_common_struct_->token) ==
              segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].end()) {
            break;
          }

          sec_id = segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type]
                                              [nse_tbt_data_common_struct_->token];
          if (sec_id < 0) break;
        }

        // Fill up the relavant fields in the struct
        if (ORDER_MESSAGE_TYPE_NEW == message_type || ORDER_MESSAGE_TYPE_MODIFY == message_type ||
            ORDER_MESSAGE_TYPE_DELETE == message_type) {
          nse_tbt_data_common_struct_->msg_type = NSE_MDS::MsgType::kNSEOrderDelta;
        } else {
          nse_tbt_data_common_struct_->msg_type = NSE_MDS::MsgType::kNSEOrderSpreadDelta;
        }

        nse_tbt_data_common_struct_->msg_seq = msg_seq_no;

        nse_tbt_data_common_struct_->activity_type = message_type;
        nse_tbt_data_common_struct_->data.nse_order.order_id =
            *((uint64_t*)((char*)(data_buffer + NSE_TBT_DATA_ORDER_MESSAGE_ORDERID_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_order.buysell = HFSAT::kTradeTypeNoInfo;

        if ('B' == data_buffer[NSE_TBT_DATA_ORDER_MESSAGE_ORDERTYPE_OFFSET])
          nse_tbt_data_common_struct_->data.nse_order.buysell = HFSAT::kTradeTypeBuy;
        else if ('S' == data_buffer[NSE_TBT_DATA_ORDER_MESSAGE_ORDERTYPE_OFFSET])
          nse_tbt_data_common_struct_->data.nse_order.buysell = HFSAT::kTradeTypeSell;
        else {
          // TODO Register An Error
        }

        nse_tbt_data_common_struct_->data.nse_order.order_price =
            *((int32_t*)((char*)(data_buffer + NSE_TBT_DATA_ORDER_MESSAGE_PRICE_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_order.order_qty =
            *((int32_t*)((char*)(data_buffer + NSE_TBT_DATA_ORDER_MESSAGE_QUANTITY_OFFSET)));

        // Packed and sent further to processor
        if (true == using_simulated_clocksource_) {
          nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
        } else {
          gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
        }

        nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);

      } break;

      // Trades + Spreads Trades
      case TRADE_MESSAGE_TYPE_TRADE:
      case SPREAD_TRADE_MESSAGE_TYPE_TRADE: {
        processed_length = NSE_TBT_DATA_TRADE_MESSAGE_LENGTH;
        nse_tbt_data_common_struct_->token =
            *((uint32_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_TOKEN_OFFSET)));
        // filtering _PE and _CE option data

        if (true == is_combined_source_mode_ &&
            (false == nse_tbt_data_processor_.IsProcessingSecurityForComShm(nse_tbt_data_common_struct_->token)))
          break;

        nse_tbt_data_common_struct_->segment_type = segment_marking;
        int sec_id = -1;
        if (true == is_raw_source_) {
          if (segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].find(
                  nse_tbt_data_common_struct_->token) ==
              segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type].end()) {
            break;
          }

          sec_id = segment_to_token_secid_map_[nse_tbt_data_common_struct_->segment_type]
                                              [nse_tbt_data_common_struct_->token];
          if (sec_id < 0) break;
        }

        // Mark Segment

        // Fill up the relavant fields in the struct
        nse_tbt_data_common_struct_->msg_type = (TRADE_MESSAGE_TYPE_TRADE == message_type)
                                                    ? NSE_MDS::MsgType::kNSETrade
                                                    : NSE_MDS::MsgType::kNSESpreadTrade;
        nse_tbt_data_common_struct_->msg_seq = msg_seq_no;
        nse_tbt_data_common_struct_->activity_type = message_type;

        nse_tbt_data_common_struct_->data.nse_trade.buy_order_id =
            *((uint64_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_BUYORDERID_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_trade.sell_order_id =
            *((uint64_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_SELLORDERID_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_trade.trade_price =
            *((int32_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_TRADEPRICE_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_trade.trade_qty =
            *((int32_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_TRADEQUANTITY_OFFSET)));

        // Packed and sent further to processor
        if (true == using_simulated_clocksource_) {
          nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();
        } else {
          gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
        }

        nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);

      } break;

      default: {
        // TODO Register Error and return message length from nse packet

      } break;
    }

    return processed_length;
  }

  inline int32_t DecodeBardataEvents(char const* data_buffer, int32_t const& msg_seq_no, char const& segment_marking) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "Decoding starts..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;
    // This variable will give an idea to raw data source handler on how many packets are there in a single UDP message
    // which are re-directed again here for decoding if read_length - this variable returns a positive number
    int32_t processed_length = 0;
    char message_type = data_buffer[NSE_TBT_DATA_MESSAGE_TYPE_OFFSET];

    switch (message_type) {
      // Trades + Spreads Trades
      case TRADE_MESSAGE_TYPE_TRADE:
      case SPREAD_TRADE_MESSAGE_TYPE_TRADE: {
        processed_length = NSE_TBT_DATA_TRADE_MESSAGE_LENGTH;
        nse_tbt_data_common_struct_->token =
            *((uint32_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_TOKEN_OFFSET)));

        if (true == is_combined_source_mode_ &&
            (false == nse_tbt_data_processor_.IsProcessingSecurityForComShm(nse_tbt_data_common_struct_->token)))
          break;

        // Mark Segment
        nse_tbt_data_common_struct_->segment_type = segment_marking;

        // Fill up the relavant fields in the struct
        nse_tbt_data_common_struct_->msg_type = (TRADE_MESSAGE_TYPE_TRADE == message_type)
                                                    ? NSE_MDS::MsgType::kNSETrade
                                                    : NSE_MDS::MsgType::kNSESpreadTrade;
        nse_tbt_data_common_struct_->msg_seq = msg_seq_no;
        nse_tbt_data_common_struct_->activity_type = message_type;

        nse_tbt_data_common_struct_->data.nse_trade.buy_order_id =
            *((uint64_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_BUYORDERID_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_trade.sell_order_id =
            *((uint64_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_SELLORDERID_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_trade.trade_price =
            *((int32_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_TRADEPRICE_OFFSET)));
        nse_tbt_data_common_struct_->data.nse_trade.trade_qty =
            *((int32_t*)((char*)(data_buffer + NSE_TBT_DATA_TRADE_MESSAGE_TRADEQUANTITY_OFFSET)));

        // Packed and sent further to processor
        if (true == using_simulated_clocksource_) {
          nse_tbt_data_common_struct_->source_time = clock_source_.GetTimeOfDay();

        } else {
          gettimeofday(&nse_tbt_data_common_struct_->source_time, NULL);
        }

        int temp_bardata_time_ = nse_tbt_data_common_struct_->source_time.tv_sec -
                                 (nse_tbt_data_common_struct_->source_time.tv_sec % bardata_period_);

        if (temp_bardata_time_ != bardata_time_) {
          if (HFSAT::kBardataLogger == daemon_mode_) {
            nse_tbt_data_processor_.DumpBardata();
	  }
          else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
            nse_tbt_data_processor_.AddBardataToRecoveryMap();
	  }
          bardata_time_ = temp_bardata_time_;
        }

        nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);

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

        if (temp_bardata_time_ != bardata_time_) {
          if (HFSAT::kBardataLogger == daemon_mode_) {
            nse_tbt_data_processor_.DumpBardata();
          } else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
            nse_tbt_data_processor_.AddBardataToRecoveryMap();
          }
          bardata_time_ = temp_bardata_time_;
        }

      } break;
    }

    return processed_length;
  }

  const char* GetExchangeSymbol(const std::string& shortcode) {
    const char* exchange_symbol = nullptr;
    try {
      exchange_symbol = ExchangeSymbolManager::GetUniqueInstance().GetExchSymbol(shortcode);

    } catch (...) {
      std::cerr << " ReqCW: NSETBTDataProcessor: Error in ExchangeSymbol Conversion for Shortcode : " << shortcode
                << std::endl;
    }

    return exchange_symbol;
  }


/*
  void FilterTokensMap(std::vector<std::string>& source_shortcode_list_, char segment) {
    std::unordered_map<int32_t, int32_t>& token_secid_map_ = segment_to_token_secid_map_[segment];
    std::vector<int> segment_tokens;

    std::cout << "REMOVING TOKENS FOR SEGMENT " << segment << std::endl;
    std::cout << "SIZE OF SHORTCODE VECTOR IS " << source_shortcode_list_.size() << std::endl;
    std::cout << "BEFORE SIZE OF MAP IS " << token_secid_map_.size() << std::endl;

    for (auto shc : source_shortcode_list_) {
      std::string exchange_symbol = GetExchangeSymbol(shc);
      std::string internal_symbol = NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exchange_symbol);
      if (internal_symbol == std::string("INVALID")) {
        std::cerr << "OnLiveProductsChange Error: Invalid Internal Symbol for " << exchange_symbol << std::endl;
        return;
      }

      char segment = NSESecurityDefinitions::GetSegmentTypeFromShortCode(shc);
      int32_t token = nse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol.c_str(), segment);

      if (token != -1 ) {
        segment_tokens.push_back(token);
      } else {
        std::cerr << " Invalid Token for shortcode " << shc << std::endl;
      }
    }

    for (auto itr = token_secid_map_.begin(); itr != token_secid_map_.end();) {
      int token = itr->first;
      if (std::find(segment_tokens.begin(), segment_tokens.end(), token) == segment_tokens.end()) {
        // Erase the element and get the next valid iterator
        itr = token_secid_map_.erase(itr);
      } else {
        std::cout << "PRESERVING TOKEN " << segment << " " << token << std::endl;
        ++itr;  // Only increment the iterator when not erasing
      }
    }

    std::cout << "MODIFIED SIZE OF MAP IS " << token_secid_map_.size() << std::endl;
  }
  */
};
}  // namespace NSEMD
}  // namespace HFSAT
