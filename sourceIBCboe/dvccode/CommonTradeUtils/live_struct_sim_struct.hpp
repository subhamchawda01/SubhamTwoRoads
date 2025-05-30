// =====================================================================================
//
//       Filename:  live_struct_sim_struct.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/02/2017 11:15:31 AM
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

#include <cstring>

#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/Utils/nse_refdata_loader.hpp"

namespace HFSAT {
namespace Utils {

void ConvertNSELiveStructToSimStruct(const NSE_MDS::NSEDotexOfflineCommonStruct& sim_struct,
                                     NSE_MDS::NSETBTDataCommonStruct* nse_tbtdata_real_struct, int trading_date) {
  nse_tbtdata_real_struct->source_time = sim_struct.source_time;
  nse_tbtdata_real_struct->msg_type = sim_struct.msg_type;
  // Msg Sequence is not needed
  nse_tbtdata_real_struct->msg_seq = 0;

  HFSAT::Utils::NSEDailyTokenSymbolHandler& nse_daily_token_symbol_handler =
      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(trading_date);
  if (nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(sim_struct.symbol, NSE_EQ_SEGMENT_MARKING) != -1) {
    nse_tbtdata_real_struct->token =
        nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(sim_struct.symbol, NSE_EQ_SEGMENT_MARKING);
    nse_tbtdata_real_struct->segment_type = NSE_EQ_SEGMENT_MARKING;
  } else if (nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(sim_struct.symbol, NSE_FO_SEGMENT_MARKING) !=
             -1) {
    nse_tbtdata_real_struct->token =
        nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(sim_struct.symbol, NSE_FO_SEGMENT_MARKING);
    nse_tbtdata_real_struct->segment_type = NSE_FO_SEGMENT_MARKING;
  } else if (nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(sim_struct.symbol, NSE_CD_SEGMENT_MARKING) !=
             -1) {
    nse_tbtdata_real_struct->token =
        nse_daily_token_symbol_handler.GetTokenFromInternalSymbol(sim_struct.symbol, NSE_CD_SEGMENT_MARKING);
    nse_tbtdata_real_struct->segment_type = NSE_CD_SEGMENT_MARKING;
  }

  HFSAT::Utils::NSERefDataLoader& nse_ref_data_loader = HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(trading_date);
  double price_multiplier =
      nse_ref_data_loader.GetPriceMultiplier(nse_tbtdata_real_struct->token, nse_tbtdata_real_struct->segment_type);

  switch (sim_struct.msg_type) {
    case NSE_MDS::MsgType::kNSEOrderDelta:
    case NSE_MDS::MsgType::kNSEOrderSpreadDelta: {
      switch (sim_struct.data.nse_dotex_order_delta.activity_type) {
        case '3': {
          nse_tbtdata_real_struct->activity_type = 'X';
        } break;
        case '1': {
          nse_tbtdata_real_struct->activity_type = 'N';
        } break;
        case '4': {
          nse_tbtdata_real_struct->activity_type = 'M';
        } break;
      }
      switch (sim_struct.data.nse_dotex_order_delta.buysell) {
        case 'B':
          nse_tbtdata_real_struct->data.nse_order.buysell = HFSAT::kTradeTypeBuy;
          break;
        case 'S':
          nse_tbtdata_real_struct->data.nse_order.buysell = HFSAT::kTradeTypeSell;
          break;
        default:
          nse_tbtdata_real_struct->data.nse_order.buysell = HFSAT::kTradeTypeNoInfo;
      }
      nse_tbtdata_real_struct->data.nse_order.order_id = sim_struct.order_number;
      nse_tbtdata_real_struct->data.nse_order.order_qty = sim_struct.data.nse_dotex_order_delta.volume_original;
      nse_tbtdata_real_struct->data.nse_order.order_price =
          (int32_t)(round(price_multiplier * sim_struct.data.nse_dotex_order_delta.order_price));
    } break;
    case NSE_MDS::MsgType::kNSESpreadTrade:
    case NSE_MDS::MsgType::kNSETrade: {
      nse_tbtdata_real_struct->data.nse_trade.trade_qty = sim_struct.data.nse_dotex_trade.trade_quantity;
      nse_tbtdata_real_struct->data.nse_trade.buy_order_id = sim_struct.data.nse_dotex_trade.buy_order_number;
      nse_tbtdata_real_struct->data.nse_trade.sell_order_id = sim_struct.data.nse_dotex_trade.sell_order_number;
      nse_tbtdata_real_struct->data.nse_trade.trade_price =
          (int32_t)(round(price_multiplier * sim_struct.data.nse_dotex_trade.trade_price));
      nse_tbtdata_real_struct->activity_type = 'T';
    } break;
    default: {
      std::cerr << "Unsupported message type in sim struct \n";
      exit(1);
    }
  }
}

void ConvertORSLiveStructToSimStruct(const HFSAT::GenericORSReplyStructLive& live_struct,
                                     HFSAT::GenericORSReplyStruct& sim_struct) {
  memcpy(sim_struct.symbol_, live_struct.symbol_, kSecNameLen);
  sim_struct.price_ = live_struct.price_;
  sim_struct.time_set_by_server_ = live_struct.time_set_by_server_;
  sim_struct.client_request_time_ = live_struct.client_request_time_;
  sim_struct.orr_type_ = live_struct.orr_type_;
  sim_struct.server_assigned_message_sequence_ = live_struct.server_assigned_message_sequence_;
  sim_struct.server_assigned_client_id_ = live_struct.server_assigned_client_id_;
  sim_struct.size_remaining_ = live_struct.size_remaining_;
  sim_struct.buysell_ = live_struct.buysell_;
  sim_struct.server_assigned_order_sequence_ = live_struct.server_assigned_order_sequence_;
  sim_struct.client_assigned_order_sequence_ = live_struct.client_assigned_order_sequence_;
  sim_struct.size_executed_ = live_struct.size_executed_;
  sim_struct.client_position_ = live_struct.client_position_;
  sim_struct.global_position_ = live_struct.global_position_;
  sim_struct.int_price_ = live_struct.int_price_;
  sim_struct.pad_ = live_struct.pad_;
  sim_struct.exch_assigned_sequence_ = live_struct.exch_assigned_sequence_;
}

void ConvertICELiveStructToSimStruct(const ICE_MDS::ICECommonStructLive& live_struct,
                                     ICE_MDS::ICECommonStruct& sim_struct) {
  memcpy(sim_struct.contract_, live_struct.contract_, ICE_MDS_CONTRACT_TEXT_SIZE);
  sim_struct.msg_ = live_struct.msg_;
  sim_struct.time_ = live_struct.time_;
  sim_struct.seqno_ = live_struct.seqno_;
  memcpy(&sim_struct.data_, &live_struct.data_, sizeof(sim_struct.data_));
}

void ConvertICESimStructToLiveStruct(ICE_MDS::ICECommonStructLive& live_struct,
                                     const ICE_MDS::ICECommonStruct& sim_struct) {
  memcpy(live_struct.contract_, sim_struct.contract_, ICE_MDS_CONTRACT_TEXT_SIZE);
  live_struct.msg_ = sim_struct.msg_;
  live_struct.time_ = sim_struct.time_;
  live_struct.seqno_ = sim_struct.seqno_;
  memcpy(&live_struct.data_, &sim_struct.data_, sizeof(live_struct.data_));
}

void ConvertAflashLiveStructToSimStruct(const AFLASH_MDS::AFlashCommonStructLive& live_struct,
                                        AFLASH_MDS::AFlashCommonStruct sim_struct) {
  sim_struct.uid_ = live_struct.uid_;
  sim_struct.time_ = live_struct.time_;
  memcpy(sim_struct.symbol_, live_struct.symbol_, 4);
  sim_struct.type_ = live_struct.type_;
  sim_struct.version_ = live_struct.version_;
  sim_struct.nfields_ = live_struct.nfields_;
  sim_struct.category_id_ = live_struct.category_id_;

  for (unsigned int i = 0; i < AFLASH_MDS::MAX_FIELDS; i++) {
    memcpy(&sim_struct.fields[i].data_, &live_struct.fields[i].data_, sizeof(sim_struct.fields[i].data_));
    sim_struct.fields[i].field_id_ = live_struct.fields[i].field_id_;
  }
}
}
}
