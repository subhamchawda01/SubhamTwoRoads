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
  bool using_simulated_clocksource_;
  bool is_combined_source_mode_;

  NSETBTDataDecoder(HFSAT::DebugLogger& dbglogger, HFSAT::FastMdConsumerMode_t const& daemon_mode,
                    HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource)
      : dbglogger_(dbglogger),
        daemon_mode_(daemon_mode),
        nse_tbt_data_processor_(NSETBTDataProcessor::GetUniqueInstance(dbglogger, daemon_mode, p_ccm_livesource)),
        nse_tbt_data_common_struct_(NULL),
        clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
        using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()),
        is_combined_source_mode_(HFSAT::kComShm == daemon_mode_) {
    if (HFSAT::kComShm == daemon_mode) {
      nse_tbt_data_common_struct_ = nse_tbt_data_processor_.GetNSECommonStructFromGenericPacket();
    } else {
      nse_tbt_data_common_struct_ = new NSE_MDS::NSETBTDataCommonStruct();
    }

    if (lzo_init() != LZO_E_OK) {
      std::cerr << "lzo_init() failed. Exiting" << std::endl;
      exit(-1);
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
        nse_tbt_data_common_struct_->data.nse_trade_range.high_exec_band = ntoh32(*((uint32_t*)(buffer + 4)));
        nse_tbt_data_common_struct_->data.nse_trade_range.low_exec_band = ntoh32(*((uint32_t*)(buffer + 8)));
        nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);
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

  // Main function
  inline int32_t DecodeEvents(char const* data_buffer, int32_t const& msg_seq_no, char const& segment_marking) {
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

        if (true == is_combined_source_mode_ &&
            (false == nse_tbt_data_processor_.IsProcessingSecurityForComShm(nse_tbt_data_common_struct_->token)))
          break;

        // Mark Segment
        nse_tbt_data_common_struct_->segment_type = segment_marking;

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

        nse_tbt_data_processor_.OnMarketEvent(nse_tbt_data_common_struct_);

      } break;

      default: {
        // TODO Register Error and return message length from nse packet

      } break;
    }

    return processed_length;
  }
};
}
}
