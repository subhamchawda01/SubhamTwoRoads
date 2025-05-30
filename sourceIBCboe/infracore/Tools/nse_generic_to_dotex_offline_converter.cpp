// =====================================================================================
//
//       Filename:  nse_generic_to_dotex_offline_converter.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/07/2015 08:55:25 AM
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/CDef/mds_messages.hpp"

// Storage class of old size and price
struct Data {
  double price;
  int32_t size;
};

// Simple interface to maintain per security based historical size and price
class HistoryTracker {
 public:
  HistoryTracker() {}
  std::map<uint64_t, Data> order_id_data_;
};

std::map<int32_t, HistoryTracker*> token_to_history_tracker;
HFSAT::Utils::NSEDailyTokenSymbolHandler* nse_daily_token_handler = NULL;
HFSAT::Utils::NSERefDataLoader* nse_refdata_loader = NULL;
NSE_MDS::NSEDotexOfflineCommonStruct* nse_dotex_common_struct = NULL;
double price_multiplier = 1;
double low_exec_band_price = 0.0;

// It's good to load and store this once as it's not going to be changed
std::map<char, std::map<int32_t, NSE_UDP_MDS::NSERefData>*> segment_to_nse_ref_data_;

void SetCommonFieldsOfDotexStruct(NSE_MDS::NSETBTDataCommonStruct next_event_) {
  if ( next_event_.msg_type == NSE_MDS::MsgType::kNSESpotIndexUpdate ) {
      std::string internal_sym = HFSAT::SpotTokenGenerator::GetUniqueInstance().GetSpotIndex(next_event_.token);
      if (internal_sym == "") {std::cerr << "Invalid internal symbol for token: " << next_event_.token << " segment: " << next_event_.segment_type
              << " Exiting!!" << std::endl;
    exit(-1);
      }
      memcpy((void*)nse_dotex_common_struct->symbol, (void*)internal_sym.c_str(),
         internal_sym.length());  // copy symbol name
      return;
  }
  std::string internal_symbol =
      nse_daily_token_handler->GetInternalSymbolFromToken(next_event_.token, next_event_.segment_type);
  if (std::string("INVALID") == internal_symbol) {
    std::cerr << "Invalid internal symbol for token: " << next_event_.token << " segment: " << next_event_.segment_type
              << " Exiting!!" << std::endl;
    exit(-1);
  }
  memcpy((void*)nse_dotex_common_struct->symbol, (void*)internal_symbol.c_str(),
         internal_symbol.length());  // copy symbol name

  std::map<int32_t, NSE_UDP_MDS::NSERefData>& nse_ref_data = *(segment_to_nse_ref_data_[next_event_.segment_type]);
  nse_dotex_common_struct->strike_price = nse_ref_data[next_event_.token].strike_price;
  nse_dotex_common_struct->option_type[0] = nse_ref_data[next_event_.token].option_type[0];
  nse_dotex_common_struct->option_type[1] = nse_ref_data[next_event_.token].option_type[1];
  price_multiplier = nse_ref_data[next_event_.token].price_multiplier;
}

// This Function takes in TBT data logged in using combined_logger and converts into dotex_offline struct
// to be used by strategies.
NSE_MDS::NSEDotexOfflineCommonStruct* DecodeGenericMessage(NSE_MDS::NSETBTDataCommonStruct* next_event_) {
  nse_dotex_common_struct->source_time = next_event_->source_time;  // set time

  switch (next_event_->msg_type) {
    case NSE_MDS::MsgType::kNSEOrderDelta:
    case NSE_MDS::MsgType::kNSEOrderSpreadDelta: {
      // Order New
      if ('N' == next_event_->activity_type) {
        nse_dotex_common_struct->data.nse_dotex_order_delta.activity_type = '1';  // for order add

        nse_dotex_common_struct->order_number = (next_event_->data).nse_order.order_id;
        nse_dotex_common_struct->msg_type = NSE_MDS::MsgType::kNSEOrderDelta;
        nse_dotex_common_struct->data.nse_dotex_order_delta.order_price =
            (next_event_->data).nse_order.order_price / price_multiplier;
        nse_dotex_common_struct->data.nse_dotex_order_delta.volume_original = (next_event_->data).nse_order.order_qty;
        nse_dotex_common_struct->data.nse_dotex_order_delta.buysell =
            HFSAT::GetTradeTypeChar((next_event_->data).nse_order.buysell);

        if (token_to_history_tracker.find(next_event_->token) == token_to_history_tracker.end()) {
          token_to_history_tracker[next_event_->token] = new HistoryTracker();
        }
        // Update History
        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_->token]->order_id_data_;
        Data data;
        data.price = nse_dotex_common_struct->data.nse_dotex_order_delta.order_price;
        data.size = (next_event_->data).nse_order.order_qty;
        old_data_info[(next_event_->data).nse_order.order_id] = data;

      } else if ('M' == next_event_->activity_type) {  // Order Modify
        // sanity checks to ensure there exists a order to be modified
        if (token_to_history_tracker.end() == token_to_history_tracker.find(next_event_->token)) {
          std::cerr << "Couldn't Find OrderInfo to modify For token. Considering as new order " << (next_event_->token)
                    << " @ : " << next_event_->source_time.tv_sec << std::endl;

          token_to_history_tracker[next_event_->token] = new HistoryTracker();
        }
        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_->token]->order_id_data_;

        if (old_data_info.find((next_event_->data).nse_order.order_id) == old_data_info.end()) {
          std::cerr << "Couldn't Find OrderId For Modification. Considering as new order : "
                    << (next_event_->data).nse_order.order_id << " @ : " << next_event_->source_time.tv_sec
                    << std::endl;

          // order id not found, consider this as a new order add
          nse_dotex_common_struct->data.nse_dotex_order_delta.old_price = low_exec_band_price;
          nse_dotex_common_struct->data.nse_dotex_order_delta.old_size = 0;
          Data data;
          data.price = (next_event_->data).nse_order.order_price / price_multiplier;
          data.size = (next_event_->data).nse_order.order_qty;
          old_data_info[(next_event_->data).nse_order.order_id] = data;
          nse_dotex_common_struct->data.nse_dotex_order_delta.activity_type = '1';  // new order
        } else {
          nse_dotex_common_struct->data.nse_dotex_order_delta.old_price =
              old_data_info[(next_event_->data).nse_order.order_id].price;
          nse_dotex_common_struct->data.nse_dotex_order_delta.old_size =
              old_data_info[(next_event_->data).nse_order.order_id].size;

          old_data_info[(next_event_->data).nse_order.order_id].price =
              (next_event_->data).nse_order.order_price / price_multiplier;
          old_data_info[(next_event_->data).nse_order.order_id].size = (next_event_->data).nse_order.order_qty;
          nse_dotex_common_struct->data.nse_dotex_order_delta.activity_type = '4';  // order modify
        }

        nse_dotex_common_struct->order_number = (next_event_->data).nse_order.order_id;
        nse_dotex_common_struct->msg_type = NSE_MDS::MsgType::kNSEOrderDelta;
        nse_dotex_common_struct->data.nse_dotex_order_delta.order_price =
            (next_event_->data).nse_order.order_price / price_multiplier;
        nse_dotex_common_struct->data.nse_dotex_order_delta.volume_original = (next_event_->data).nse_order.order_qty;
        nse_dotex_common_struct->data.nse_dotex_order_delta.buysell =
            HFSAT::GetTradeTypeChar((next_event_->data).nse_order.buysell);

      } else if ('X' == next_event_->activity_type) {  // Order Delete
        // sanity checks to ensure there exists a order to be deleted
        if (token_to_history_tracker.find(next_event_->token) == token_to_history_tracker.end()) {
          std::cerr << "Couldn't Find OrderInfo to delete For token: " << (next_event_->token)
                    << " @ : " << next_event_->source_time.tv_sec << std::endl;
          return nullptr;
        }
        std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_->token]->order_id_data_;

        if (old_data_info.find((next_event_->data).nse_order.order_id) == old_data_info.end()) {
          std::cerr << "Couldn't Find OrderId For Deletion : " << (next_event_->data).nse_order.order_id
                    << " @ : " << next_event_->source_time.tv_sec << std::endl;
          return nullptr;
        }
        nse_dotex_common_struct->data.nse_dotex_order_delta.activity_type = '3';
        nse_dotex_common_struct->order_number = (next_event_->data).nse_order.order_id;
        nse_dotex_common_struct->msg_type = NSE_MDS::MsgType::kNSEOrderDelta;
        nse_dotex_common_struct->data.nse_dotex_order_delta.order_price =
            (next_event_->data).nse_order.order_price / price_multiplier;
        nse_dotex_common_struct->data.nse_dotex_order_delta.old_size =
            old_data_info[(next_event_->data).nse_order.order_id].size;
        nse_dotex_common_struct->data.nse_dotex_order_delta.buysell =
            HFSAT::GetTradeTypeChar((next_event_->data).nse_order.buysell);

        old_data_info.erase((next_event_->data).nse_order.order_id);

      } else {
        std::cerr << "UNEXPECTED ACTIVITY TYPE RECEVIED IN DATA : " << next_event_->activity_type << std::endl;
        return nullptr;  // ignore this packet
      }

    } break;
    // process trade msgs
    case NSE_MDS::MsgType::kNSETrade:
    case NSE_MDS::MsgType::kNSESpreadTrade: {
      int32_t bid_size_remaining = -1;
      int32_t ask_size_remaining = -1;

      if (token_to_history_tracker.find(next_event_->token) == token_to_history_tracker.end()) {
        std::cerr << "Couldn't Find OrderInfo on trade For token: " << (next_event_->token)
                  << " @ : " << next_event_->source_time.tv_sec << std::endl;
        token_to_history_tracker[next_event_->token] =
            new HistoryTracker();  // create dummy struct in case of packet loss
      }

      std::map<uint64_t, Data>& old_data_info = token_to_history_tracker[next_event_->token]->order_id_data_;

      if (old_data_info.end() != old_data_info.find((next_event_->data).nse_trade.buy_order_id)) {
        old_data_info[(next_event_->data).nse_trade.buy_order_id].size -= (next_event_->data).nse_trade.trade_qty;
        bid_size_remaining = std::max(0, (int32_t)old_data_info[(next_event_->data).nse_trade.buy_order_id].size);
      }

      if (old_data_info.end() != old_data_info.find((next_event_->data).nse_trade.sell_order_id)) {
        old_data_info[(next_event_->data).nse_trade.sell_order_id].size -= (next_event_->data).nse_trade.trade_qty;
        ask_size_remaining = std::max(0, (int32_t)old_data_info[(next_event_->data).nse_trade.sell_order_id].size);
      }

      nse_dotex_common_struct->order_number = -1;
      nse_dotex_common_struct->msg_type = next_event_->msg_type;
      nse_dotex_common_struct->data.nse_dotex_trade.trade_price =
          next_event_->data.nse_trade.trade_price / price_multiplier;
      nse_dotex_common_struct->data.nse_dotex_trade.trade_quantity = next_event_->data.nse_trade.trade_qty;
      nse_dotex_common_struct->data.nse_dotex_trade.buy_order_number = next_event_->data.nse_trade.buy_order_id;
      nse_dotex_common_struct->data.nse_dotex_trade.sell_order_number = next_event_->data.nse_trade.sell_order_id;
      nse_dotex_common_struct->data.nse_dotex_trade.bid_size_remaining = bid_size_remaining;
      nse_dotex_common_struct->data.nse_dotex_trade.ask_size_remaining = ask_size_remaining;

    } break;

    case NSE_MDS::MsgType::kNSETradeExecutionRange: {
      nse_dotex_common_struct->order_number = -1;
      nse_dotex_common_struct->msg_type = next_event_->msg_type;
      nse_dotex_common_struct->data.nse_dotex_trade_range.high_exec_band =
          next_event_->data.nse_trade_range.high_exec_band / price_multiplier;
      nse_dotex_common_struct->data.nse_dotex_trade_range.low_exec_band =
          next_event_->data.nse_trade_range.low_exec_band / price_multiplier;
      low_exec_band_price = nse_dotex_common_struct->data.nse_dotex_trade_range.low_exec_band;
    } break;

    case NSE_MDS::MsgType::kNSESpotIndexUpdate: {
      nse_dotex_common_struct->order_number = -1;
      nse_dotex_common_struct->msg_type = next_event_->msg_type;
      nse_dotex_common_struct->data.nse_spot_index.IndexValue =
          next_event_->data.nse_spot_index.IndexValue;
      nse_dotex_common_struct->data.nse_spot_index.PercentChange =
          next_event_->data.nse_spot_index.PercentChange;
    } break;

    default: {
      std::cerr << "UNEXPECTED TYPE OF MESSAGE RECEVIED : " << (int32_t)(next_event_->msg_type) << std::endl;
      return nullptr;  // ignore this packet
    } break;
  }

  return nse_dotex_common_struct;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage : " << argv[0] << " < InputFile >"
              << " < Date >"
              << " < OUTPUT FILE>" << std::endl;
    exit(-1);
  }
  int date = atoi(argv[2]);
  HFSAT::BulkFileReader* bulk_file_reader = new HFSAT::BulkFileReader();
  bulk_file_reader->open(argv[1]);
  HFSAT::BulkFileWriter bulk_file_writer;
  bulk_file_writer.Open(argv[3]);
  nse_daily_token_handler = &(HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(date));

  bool initialise_common_fields = false;
  // Load ref data for all segments
  nse_refdata_loader = &(HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(date));
  segment_to_nse_ref_data_[NSE_EQ_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
  segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();
  segment_to_nse_ref_data_[NSE_CD_SEGMENT_MARKING] = new std::map<int32_t, NSE_UDP_MDS::NSERefData>();

  std::map<int32_t, NSE_UDP_MDS::NSERefData>& eq_nse_ref_data = *(segment_to_nse_ref_data_[NSE_EQ_SEGMENT_MARKING]);
  std::map<int32_t, NSE_UDP_MDS::NSERefData>& fo_nse_ref_data = *(segment_to_nse_ref_data_[NSE_FO_SEGMENT_MARKING]);
  std::map<int32_t, NSE_UDP_MDS::NSERefData>& cd_nse_ref_data = *(segment_to_nse_ref_data_[NSE_CD_SEGMENT_MARKING]);
  for (auto& itr : nse_refdata_loader->GetNSERefData(NSE_EQ_SEGMENT_MARKING)) {
    eq_nse_ref_data[itr.first] = itr.second;
  }
  for (auto& itr : nse_refdata_loader->GetNSERefData(NSE_CD_SEGMENT_MARKING)) {
    cd_nse_ref_data[itr.first] = itr.second;
  }
  for (auto& itr : nse_refdata_loader->GetNSERefData(NSE_FO_SEGMENT_MARKING)) {
    fo_nse_ref_data[itr.first] = itr.second;
  }
  // ref data loaded for all segments

  // Load Sec Def and exchange symbol manager
  HFSAT::SecurityDefinitions::GetUniqueInstance(date).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date);

  // allocate memory to result struct once
  nse_dotex_common_struct = new NSE_MDS::NSEDotexOfflineCommonStruct();
  if (!bulk_file_reader->is_open()) {
    std::cerr << "Failed To Open DataFile : " << argv[1] << " For Reading" << std::endl;
    exit(-1);
  } else {
    while (true) {
      // expect generic logged data as input which has NSE struct
      HFSAT::MDS_MSG::GenericMDSMessage generic_msg;
      NSE_MDS::NSETBTDataCommonStruct next_event_;
      size_t available_len_ = bulk_file_reader->read(&generic_msg, sizeof(generic_msg));
      if (available_len_ < sizeof(generic_msg)) {  // check if reading complete structs
        std::cerr << "Incorrect len read: expected " << sizeof(generic_msg) << " read len: " << available_len_
                  << std::endl;
        break;
      }
      if (generic_msg.mds_msg_exch_ != HFSAT::MDS_MSG::NSE) {
        std::cerr << "Only supports NSE generic logged data. Exiting. Given exchange type is "
                  << (int)generic_msg.mds_msg_exch_ << std::endl;
        exit(-1);
      }
      next_event_ = generic_msg.generic_data_.nse_data_;

      if (!initialise_common_fields) {
        SetCommonFieldsOfDotexStruct(next_event_);
        initialise_common_fields = true;
      }
      // convert NSETBTDataCommonStruct into NSEDotexOfflineCommonStruct and log using bulkFileWriter
      NSE_MDS::NSEDotexOfflineCommonStruct* converted_dotex_offline_msg = DecodeGenericMessage(&next_event_);

      if (converted_dotex_offline_msg != nullptr) {
        bulk_file_writer.Write(converted_dotex_offline_msg, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
        bulk_file_writer.CheckToFlushBuffer();
      }
    }
    // always close the writer, the buffered data need to be flushed
    bulk_file_reader->close();
    bulk_file_writer.Close();
  }
  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
