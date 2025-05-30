#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include "string.h"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "infracore/NSET/NSEEngine.hpp"
#include "infracore/NSET/nse_tap_invitation_manager.hpp"
#include "infracore/NSET/nse_msg_handler.hpp"
#include "infracore/NSET/nse_msg_handler_cash_market.hpp"
#include "infracore/NSET/nse_msg_handler_derivatives.hpp"
#include "infracore/NSET/NSETemplates/OrderPriceChangeRequest.hpp"

void ParseAuditIn(std::string filename);
void ParseAuditOut(std::string filename);

HFSAT::NSE::NseMsgHandler* nse_msgs_handler_;
std::string segment;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage : <exec> <AUDIT_IN/AUDIT_OUT> <NSE_FO/NSE_EQ/NSE_CD> <input-file>" << std::endl;
    exit(-1);
  }

  std::string option(argv[1]);
  std::string segment_(argv[2]);
  segment = segment_;
  std::string filename(argv[3]);

  if (segment == "NSE_EQ") {
    nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerCashMarket();
  } else {
    nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerDerivatives();
  }

  if (option == "AUDIT_IN") {
    ParseAuditIn(filename);
  } else if (option == "AUDIT_OUT") {
    ParseAuditOut(filename);
  } else {
    std::cout << "Invalid Option" << std::endl;
  }

  return 0;
}

void ParseAuditOut(std::string filename) {
  HFSAT::BulkFileReader reader1, reader2;

  reader1.open(filename);
  reader2.open(filename);

  HFSAT::NSE::ProcessedPacketHeader* processed_packet_header;
  HFSAT::NSE::ProcessedResponseHeader* processed_response_header;

  while (true) {
    char nse_msg_ptr[MAX_NSE_RESPONSE_BUFFER_SIZE];
    size_t available_len_1 = reader1.read(nse_msg_ptr, NSE_PACKET_RESPONSE_LENGTH);
    processed_packet_header = nse_msgs_handler_->packet_response_.ProcessPakcet(nse_msg_ptr);
    if (available_len_1 < NSE_PACKET_RESPONSE_LENGTH) break;
    available_len_1 = reader1.read(nse_msg_ptr, NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
    processed_response_header = nse_msgs_handler_->response_header_.ProcessHeader(nse_msg_ptr);
    if (available_len_1 < NSE_RESPONSE_MESSAGE_HEADER_LENGTH) break;
    const char* msg_ptr = nse_msg_ptr;

    switch (processed_response_header->transaction_code) {
      case NSE_HEARTBEAT: {
        reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
        int32_t packet_seq_no = ntoh32(*((int32_t*)(nse_msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
        std::cout << "NSE_HEARTBEAT: "
                  << "packet_sequence_number: " << (int)packet_seq_no << std::endl;
      } break;
      case SIGN_OFF_REQUEST_IN: {
        std::cout << "SIGN_OFF_REQUEST" << std::endl;
        reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
      } break;

      case SIGN_ON_REQUEST_IN: {
        std::cout << "SIGN_ON_REQUEST" << std::endl;
        reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
      } break;
      case PRICE_MOD_IN: {
        reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
        HFSAT::NSE::OrderPriceModifyRequest req;
        memcpy(&req, nse_msg_ptr, sizeof(HFSAT::NSE::OrderPriceModifyRequest));
        std::cout << "PRICE_MOD_IN: " << req.ToString() << std::endl;
      } break;
      case ORDER_MOD_IN_TR: {
        if (segment == "NSE_EQ") {
          reader2.read(nse_msg_ptr, NSE_CM_CHANGE_REQUEST_LENGTH);
          HFSAT::NSE::SecurityInfoCashMarket inst_desc;
          memcpy(&inst_desc, (msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                              NSE_CM_ORDER_MODCXL_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
                 sizeof(HFSAT::NSE::SecurityInfoCashMarket));
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int32_t user_id = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                NSE_CM_ORDER_MODCXL_REQUEST_USERID_OFFSET)));  // TODO
          int64_t order_no = hton64(*((int64_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                 NSE_CM_ORDER_MODCXL_REQUEST_ORDERNUMBER_OFFSET)));
          int32_t entry_date = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                   NSE_CM_ORDER_MODCXL_REQUEST_ENTRYDATETIME_OFFSET)));
          int32_t modified_date = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                      NSE_CM_ORDER_MODCXL_REQUEST_LASTMODIFIED_OFFSET)));
          int16_t buy_sell = ntoh16(*((int16_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                 NSE_CM_ORDER_MODCXL_REQUEST_BUYSELLINDICATOR_OFFSET)));
          int32_t disclosed_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                      NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET)));
          int32_t remaining_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                            NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)));
	  int32_t vol = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                            NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)));
	  int32_t disclosed_vol_remaining = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
					     NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)));
          int32_t price = ntoh32(
              *((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PRICE_OFFSET)));
          int32_t saos = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                             NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTION_ID_OFFSET)));
          double nnf = (double)swap_endian(
              *((double*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_NFFIELD_OFFSET)));
	  int64_t last_activity_ref = ntoh64(*((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + 
				  NSE_CM_ORDER_MODCXL_REQUEST_LASTACTIVITYREFERENCE_OFFSET)));
          std::cout << "ORDER_MOD_IN_TR: "
                    << "packet_sequence_number: " << packet_seq_no << " user_id: " << user_id
                    << " buy_sell: " << buy_sell << " disclosed_vol: " << disclosed_vol << " Remaining_vol: " 
		    << remaining_vol << "vol: " << vol << " disclosed_vol_remaining: "
		    << disclosed_vol_remaining << " price: " << price << " saos: " << saos << " order_no: " << order_no
                    << " entry_date: " << entry_date << " modified_date: " << modified_date << " " << "Last_Activity_ref: " 
		    << last_activity_ref << inst_desc.ToString() << " nnf: " << std::setprecision(16) << nnf << std::endl;
        } else {
          reader2.read(nse_msg_ptr, NSE_CHANGE_REQUEST_LENGTH);
          HFSAT::NSE::InstrumentDesc inst_desc;
          memcpy(&inst_desc, (msg_ptr + NSE_CHANGE_REQUEST_TOKENNUM_OFFSET), sizeof(HFSAT::NSE::InstrumentDesc));
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int32_t user_id = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_TRADERID_OFFSET)));  // TODO
          int64_t order_no = hton64(*((int64_t*)(msg_ptr + NSE_CHANGE_REQUEST_ORDERNUMBER_OFFSET)));
          int32_t entry_date = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_ENTRYDATETIME_OFFSET)));
          int32_t modified_date = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_LASTMODIFIED_OFFSET)));
          int16_t buy_sell = ntoh16(*((int16_t*)(msg_ptr + NSE_CHANGE_REQUEST_BUYSELLINDICATOR_OFFSET)));
          int32_t disclosed_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_OFFSET)));
          int32_t vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_VOLUME_OFFSET)));
          int32_t disclosed_vol_remaining =
              ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)));
          int32_t remaining_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_OFFSET)));
          int32_t price = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_PRICE_OFFSET)));
          int32_t saos = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_FILLER_OFFSET)));
          double nnf = (double)swap_endian(*((double*)(msg_ptr + NSE_CHANGE_REQUEST_NFFIELD_OFFSET)));
	  int64_t last_activity_ref = ntoh64(*((int64_t *)(msg_ptr + NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_OFFSET)));
	  std::cout << "ORDER_MOD_IN_TR: "
                    << "packet_sequence_number: " << packet_seq_no << " user_id: " << user_id
                    << " buy_sell: " << buy_sell << " disclosed_vol: " << disclosed_vol << " Remaining_vol: "
                    << remaining_vol << " vol: " << vol << " disclosed_vol_remaining: "
                    << disclosed_vol_remaining << " price: " << price << " saos: " << saos << " order_no: " << order_no
                    << " entry_date: " << entry_date << " modified_date: " << modified_date << " " << "Last_Activity_ref: "
                    << last_activity_ref << inst_desc.ToString() << " nnf: " << std::setprecision(16) << nnf << std::endl;
        }
      } break;
      case ORDER_CANCEL_IN_TR: {
        if (segment == "NSE_EQ") {
          reader2.read(nse_msg_ptr, NSE_CM_CHANGE_REQUEST_LENGTH);
          HFSAT::NSE::SecurityInfoCashMarket inst_desc;
          memcpy(&inst_desc, (msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                              NSE_CM_ORDER_MODCXL_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
                 sizeof(HFSAT::NSE::SecurityInfoCashMarket));
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int32_t user_id = ntoh32(
              *((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_USERID_OFFSET)));
          int64_t order_no = hton64(*((int64_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                 NSE_CM_ORDER_MODCXL_REQUEST_ORDERNUMBER_OFFSET)));
          int32_t entry_date = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                   NSE_CM_ORDER_MODCXL_REQUEST_ENTRYDATETIME_OFFSET)));
          int32_t modified_date = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                      NSE_CM_ORDER_MODCXL_REQUEST_LASTMODIFIED_OFFSET)));
          int16_t buy_sell = ntoh16(*((int16_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                                 NSE_CM_ORDER_MODCXL_REQUEST_BUYSELLINDICATOR_OFFSET)));
          int32_t disclosed_vol = ntoh32(*((int32_t*)(msg_ptr +NSE_PACKET_REQUEST_LENGTH +
					 	 NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUME_OFFSET)));
          int32_t vol = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                            NSE_CM_ORDER_MODCXL_REQUEST_VOLUME_OFFSET)));
	  int32_t disclosed_vol_remaining = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
					    NSE_CM_ORDER_MODCXL_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)));
          int32_t vol_remaining = ntoh32(*((int32_t*)(msg_ptr +NSE_PACKET_REQUEST_LENGTH +
					     NSE_CM_ORDER_MODCXL_REQUEST_TOTALVOLUMEREMAINING_OFFSET)));
          int32_t price = ntoh32(
              *((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + NSE_CM_ORDER_MODCXL_REQUEST_PRICE_OFFSET)));
          int32_t saos = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_REQUEST_LENGTH +
                                             NSE_CM_ORDER_MODCXL_REQUEST_TRANSACTION_ID_OFFSET)));
          double nnf = (double)swap_endian(*((double*)(msg_ptr + NSE_CM_ORDER_MODCXL_REQUEST_NFFIELD_OFFSET)));
          int64_t last_activity_ref = ntoh64(*((int64_t *)(msg_ptr + NSE_PACKET_REQUEST_LENGTH + 
                                  NSE_CM_ORDER_MODCXL_REQUEST_LASTACTIVITYREFERENCE_OFFSET)));
          std::cout << "ORDER_CANCEL_IN_TR: "
                    << "packet_sequence_number: " << packet_seq_no << " user_id: " << user_id
                    << " buy_sell: " << buy_sell  << " disclosed_vol: " << disclosed_vol << " vol: " << vol 
		    << " disclosed_vol_remaining: " << disclosed_vol_remaining << " vol_remaining: " << vol_remaining
		    << " price: " << price << " saos: " << saos<< " order_no: " << order_no 
		    << " entry_date: " << entry_date << " modified_date: " << modified_date<< " " << "Last_Activity_ref: "
                    << last_activity_ref << inst_desc.ToString() << " nnf: " << std::setprecision(16) << nnf << std::endl;
        } else {
          reader2.read(nse_msg_ptr, NSE_CHANGE_REQUEST_LENGTH);
          HFSAT::NSE::InstrumentDesc inst_desc;
          memcpy(&inst_desc, (msg_ptr + NSE_CHANGE_REQUEST_TOKENNUM_OFFSET), sizeof(HFSAT::NSE::InstrumentDesc));
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int32_t user_id = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_TRADERID_OFFSET)));
          int64_t order_no = hton64(*((int64_t*)(msg_ptr + NSE_CHANGE_REQUEST_ORDERNUMBER_OFFSET)));
          int32_t entry_date = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_ENTRYDATETIME_OFFSET)));
          int32_t modified_date = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_LASTMODIFIED_OFFSET)));
          int16_t buy_sell = ntoh16(*((int16_t*)(msg_ptr + NSE_CHANGE_REQUEST_BUYSELLINDICATOR_OFFSET)));
          int32_t disclosed_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUME_OFFSET)));
          int32_t vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_VOLUME_OFFSET)));
          int32_t disclosed_vol_remaining =
              ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_DISCLOSEDVOLUMEREMAINING_OFFSET)));
          int32_t vol_remaining = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_TOTALVOLUMEREMAINING_OFFSET)));
          int32_t price = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_PRICE_OFFSET)));
          int32_t saos = ntoh32(*((int32_t*)(msg_ptr + NSE_CHANGE_REQUEST_FILLER_OFFSET)));
          double nnf = (double)swap_endian(*((double*)(msg_ptr + NSE_CHANGE_REQUEST_NFFIELD_OFFSET)));
	  int64_t last_activity_ref = ntoh64(*((int64_t *)(msg_ptr +  NSE_CHANGE_REQUEST_LASTACTIVITYREFERENCE_OFFSET)));

          std::cout << "ORDER_CANCEL_IN_TR: "
                    << "packet_sequence_number: " << packet_seq_no << " user_id: " << user_id
                    << " buy_sell: " << buy_sell << " disclosed_vol: " << disclosed_vol << " vol: " << vol
                    << " disclosed_vol_remaining: " << disclosed_vol_remaining << " vol_remaining: " << vol_remaining
                    << " price: " << price << " saos: " << saos << " order_no: " << order_no
                    << " entry_date: " << entry_date << " modified_date: " << modified_date << " "<< "Last_Activity_ref: "
                    << last_activity_ref << inst_desc.ToString() << " nnf: " << std::setprecision(16) << nnf << std::endl;
        }

      } break;

      case BOARD_LOT_IN_TR: {
        if (segment == "NSE_EQ") {
          HFSAT::NSE::SecurityInfoCashMarket inst_desc;
          reader2.read(nse_msg_ptr, NSE_CM_ORDERENTRY_REQUEST_LENGTH);

          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int32_t user_id = ntoh32(*((int32_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_TRADERID_OFFSET)));
          int16_t buy_sell = ntoh16(*((int16_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)));
          int32_t disclosed_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)));
          int32_t vol = ntoh32(*((int32_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_VOLUME_OFFSET)));
          int32_t price = ntoh32(*((int32_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PRICE_OFFSET)));
          int32_t saos = ntoh32(*((int32_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_TRANSACTION_ID_OFFSET)));
          int16_t ioc = (ntoh16)(*((int16_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)));
          int16_t procli = (ntoh16)(*((int16_t*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)));
          double nnf = (double)swap_endian(*((double*)(msg_ptr + NSE_CM_ORDERENTRY_REQUEST_NFFIELD_OFFSET)));

          char buffer[16];
          char buffer2[16];
          memcpy((void*)buffer, (msg_ptr + NSE_CM_ORDERENTRY_REQUEST_SETTLOR_OFFSET),
                 NSE_CM_ORDERENTRY_REQUEST_SETTLOR_LENGTH);
          memcpy((void*)buffer2, (msg_ptr + NSE_CM_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET),
                 NSE_CM_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);

          memcpy(&inst_desc, (msg_ptr + NSE_CM_ORDERENTRY_REQUEST_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),
                 sizeof(HFSAT::NSE::SecurityInfoCashMarket));
          std::cout << "BOARD_LOT_IN_TR: "
                    << "packet_sequence_number: " << packet_seq_no << " user_id: " << user_id
                    << " buy_sell: " << buy_sell << " disclosed_vol: " << disclosed_vol << " vol: " << vol
                    << " price: " << price << " saos: " << saos << " procli :" << procli << " ioc: " << ioc << " "
                    << inst_desc.ToString() << " nnf: " << std::setprecision(16) << nnf << " Settlor : " << buffer
                    << " ACC : " << buffer2 << std::endl;
        } else {
          HFSAT::NSE::InstrumentDesc inst_desc;
          reader2.read(nse_msg_ptr, NSE_ORDERENTRY_REQUEST_LENGTH);

          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int32_t user_id = ntoh32(*((int32_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_USERID_OFFSET)));
          int16_t buy_sell = ntoh16(*((int16_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_BUYSELLINDICATOR_OFFSET)));
          int32_t disclosed_vol = ntoh32(*((int32_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_DISCLOSEDVOLUME_OFFSET)));
          int32_t vol = ntoh32(*((int32_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_VOLUME_OFFSET)));
          int32_t price = ntoh32(*((int32_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_PRICE_OFFSET)));
          int32_t saos = ntoh32(*((int32_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_FILLER_OFFSET)));
          int16_t ioc = (ntoh16)(*((int16_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_STORDERFLAGS_STRUCT_OFFSET)));
          int16_t procli = (ntoh16)(*((int16_t*)(msg_ptr + NSE_ORDERENTRY_REQUEST_PROCLIENTINDICATOR_OFFSET)));
          double nnf = (double)swap_endian(*((double*)(msg_ptr + NSE_ORDERENTRY_REQUEST_NFFIELD_OFFSET)));

          char buffer[16];
          char buffer2[16];
          memcpy((void*)buffer, (msg_ptr + NSE_ORDERENTRY_REQUEST_SETTLOR_OFFSET),
                 NSE_ORDERENTRY_REQUEST_SETTLOR_LENGTH);
          memcpy((void*)buffer2, (msg_ptr + NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_OFFSET),
                 NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_LENGTH);

          memcpy(&inst_desc, (msg_ptr + NSE_ORDERENTRY_REQUEST_TOKENNUM_OFFSET), sizeof(HFSAT::NSE::InstrumentDesc));
          std::cout << "BOARD_LOT_IN_TR: "
                    << "packet_sequence_number: " << packet_seq_no << " user_id: " << user_id
                    << " buy_sell: " << buy_sell << " disclosed_vol: " << disclosed_vol << " vol: " << vol
                    << " price: " << price << " saos: " << saos << "procli :" << procli << " ioc: " << ioc << " "
                    << inst_desc.ToString() << " nnf: " << std::setprecision(16) << nnf << " Settlor : " << buffer
                    << " ACC : " << buffer2 << std::endl;
        }
      } break;

      case UPDATE_LOCALDB_IN: {
        if (segment == "NSE_EQ") {
          reader2.read(nse_msg_ptr, NSE_CM_UPDATELOCALDATABSE_REQUEST_LENGTH);
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int16_t mkt_status_normal = ntoh16(*((int16_t*)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_NORMAL_OFFSET)));
          int16_t mkt_status_oddlot = ntoh16(*((int16_t*)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_ODDLOT_OFFSET)));
          int16_t mkt_status_spot = ntoh16(*((int16_t*)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_SPOT_OFFSET)));
          int16_t call_auction_1 =
              ntoh16(*((int16_t*)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_CALL_AUCTION_1_OFFSET)));
          int16_t call_auction_2 =
              ntoh16(*((int16_t*)(msg_ptr + NSE_CM_UPDATELOCALDATABSE_REQUEST_CALL_AUCTION_2_OFFSET)));
          std::cout << "UPDATE_LOCALDB_IN: "
                    << "packet_sequence_number: " << packet_seq_no << " market_status_normal: " << mkt_status_normal
                    << " market_status_oddLot: " << mkt_status_oddlot << " market_status_spot: " << mkt_status_spot
                    << " call_auction_1: " << call_auction_1 << " call_auction_2: " << call_auction_2 << std::endl;

        } else {
          reader2.read(nse_msg_ptr, NSE_UPDATELOCALDATABSE_REQUEST_LENGTH);

          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));
          int16_t mkt_status_normal =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STMARKETSTATUS_STRUCT_NORMAL_OFFSET)));
          int16_t mkt_status_oddlot =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STMARKETSTATUS_STRUCT_ODDLOT_OFFSET)));
          int16_t mkt_status_spot =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STMARKETSTATUS_STRUCT_SPOT_OFFSET)));
          int16_t mkt_status_auction =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STMARKETSTATUS_STRUCT_AUCTION_OFFSET)));

          int16_t ex_mkt_status_normal =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STEXMARKETSTATUS_STRUCT_NORMAL_OFFSET)));
          int16_t ex_mkt_status_oddlot =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STEXMARKETSTATUS_STRUCT_ODDLOT_OFFSET)));
          int16_t ex_mkt_status_spot =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STEXMARKETSTATUS_STRUCT_SPOT_OFFSET)));
          int16_t ex_mkt_status_auction =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STEXMARKETSTATUS_STRUCT_AUCTION_OFFSET)));
          int16_t pl_market_status_normal =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STPLMARKETSTATUS_STRUCT_NORMAL_OFFSET)));
          int16_t pl_market_status_oddlot =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STPLMARKETSTATUS_STRUCT_ODDLOT_OFFSET)));
          int16_t pl_market_status_spot =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STPLMARKETSTATUS_STRUCT_SPOT_OFFSET)));
          int16_t pl_market_status_auction =
              ntoh16(*((int16_t*)(msg_ptr + NSE_UPDATELOCALDATABSE_REQUEST_STPLMARKETSTATUS_STRUCT_AUCTION_OFFSET)));
          std::cout << "UPDATE_LOCALDB_IN: "
                    << "packet_sequence_number: " << packet_seq_no << " market_status_normal: " << mkt_status_normal
                    << " market_status_oddLot: " << mkt_status_oddlot << " market_status_spot: " << mkt_status_spot
                    << " market_status_auction: " << mkt_status_auction
                    << " ex_market_status_Normal: " << ex_mkt_status_normal
                    << " ex_market_status_oddLot: " << ex_mkt_status_oddlot
                    << " ex_market_status_spot: " << ex_mkt_status_spot
                    << " ex_market_status_auction: " << ex_mkt_status_auction
                    << " pl_market_status_normal: " << pl_market_status_normal
                    << " pl_market_status_oddLot: " << pl_market_status_oddlot
                    << " pl_market_status_spot: " << pl_market_status_spot
                    << " pl_market_status_auction: " << pl_market_status_auction << std::endl;
        }
      } break;

      case SYSTEM_INFORMATION_IN: {
        if (segment == "NSE_EQ") {
          reader2.read(nse_msg_ptr, NSE_CM_SYSTEMINFO_REQUEST_LENGTH);
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));

          std::cout << "SYSTEM_INFORMATION_IN: "
                    << "packet_sequence_number: " << packet_seq_no << std::endl;
        } else {
          reader2.read(nse_msg_ptr, NSE_SYSTEMINFO_REQUEST_LENGTH);
          int32_t packet_seq_no = ntoh32(*((int32_t*)(msg_ptr + NSE_PACKET_SEQUENCE_OFFSET)));

          std::cout << "SYSTEM_INFORMATION_IN: "
                    << "packet_sequence_number: " << packet_seq_no << std::endl;
        }
      } break;

      default: { reader2.read(nse_msg_ptr, processed_packet_header->packet_length); }
    }
    size_t msg_len =
        processed_packet_header->packet_length - NSE_RESPONSE_MESSAGE_HEADER_LENGTH - NSE_PACKET_RESPONSE_LENGTH;
    reader1.read(nse_msg_ptr, msg_len);
  }
}
void ParseAuditIn(std::string filename) {
  HFSAT::BulkFileReader reader1, reader2;

  reader1.open(filename);
  reader2.open(filename);
  HFSAT::NSE::ProcessedPacketHeader* processed_packet_header;
  HFSAT::NSE::ProcessedResponseHeader* processed_response_header;

  while (true) {
    char nse_msg_ptr[MAX_NSE_RESPONSE_BUFFER_SIZE];
    size_t available_len_1 = reader1.read(nse_msg_ptr, NSE_PACKET_RESPONSE_LENGTH);
    // we read the header from the file

    processed_packet_header = nse_msgs_handler_->packet_response_.ProcessPakcet(nse_msg_ptr);
    if (available_len_1 < NSE_PACKET_RESPONSE_LENGTH) break;
    available_len_1 = reader1.read(nse_msg_ptr, NSE_RESPONSE_MESSAGE_HEADER_LENGTH);
    processed_response_header = nse_msgs_handler_->response_header_.ProcessHeader(nse_msg_ptr);
    if (available_len_1 < NSE_RESPONSE_MESSAGE_HEADER_LENGTH) break;

    int16_t error_code = processed_response_header->error_code;

    std::cout << " ERROR COD: "  << error_code << std::endl ;

    reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
    const char* msg_ptr = nse_msg_ptr;
    msg_ptr += NSE_PACKET_RESPONSE_LENGTH;
    msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;

    switch (processed_response_header->transaction_code) {
      case SIGN_ON_REQUEST_OUT: {
        std::cout << "LoginReceived: ";
        HFSAT::NSE::ProcessedLogonResponse* processed_logon_response_;
        processed_logon_response_ = nse_msgs_handler_->logon_response_->ProcessLogon(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_logon_response_->ToString() << std::endl;

      } break;

      case SIGN_OFF_REQUEST_OUT: {
        std::cout << "error_code: " << error_code << " "
                  << "Logout Received" << std::endl;
      } break;

      case GR_REQUEST_OUT: {
                             HFSAT::NSE::ProcessedGRResponse *processed_gr_response_ = nse_msgs_handler_->gr_response_->ProcessGRResponse(msg_ptr);
        std::cout << " GR LOGIN : " << std::endl ;
         std::cout << processed_gr_response_->ToString() << std::endl ;
      } break;

      case BoxLogin_REQUEST_OUT: {
                                   HFSAT::NSE::ProcessedBoxLoginResponse *processed_boxlogin_response_ = nse_msgs_handler_->boxlogin_response_->ProcessBoxLoginResponse(msg_ptr);
        std::cout << " BOX LOGIN : " << std::endl ;
        std::cout << processed_boxlogin_response_->ToString() << std::endl ;
      } break;
      case BOX_SIGN_OFF : {
                                   HFSAT::NSE::ProcessedBoxSignOffResponse *processed_boxlogout_response_ = nse_msgs_handler_->boxsignoff_response_->ProcessBoxSignOffResponse(msg_ptr);
        std::cout << " BOX LOGOUT : " << std::endl ;
        std::cout << processed_boxlogout_response_->ToString() << std::endl ;
      } break;

      case ORDER_CONFIRMATION: {
        std::cout << "ORDER_CONFIRMATION: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;
        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_CONFIRMATION_TR: {
        std::cout << "ORDER_CONFIRMATION_TR: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;
        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_MOD_CONFIRMATION_TR: {
        std::cout << "ORDER_MOD_CONFIRMATION_TR: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);

        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;
        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_CXL_CONFIRMATION_TR: {
        std::cout << "ORDER_CXL_CONFIRMATION_TR: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);

        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;

        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_CANCEL_REJECT: {
        std::cout << "ORDER_CANCEL_REJECT: ";
        HFSAT::NSE::ProcessedOrderResponseNonTR* processed_order_response_non_tr_;
        processed_order_response_non_tr_ = nse_msgs_handler_->order_response_non_tr_.ProcessOrderResponse(msg_ptr);

        std::cout << "error_code: " << error_code << " " << processed_order_response_non_tr_->ToString() << std::endl;
      } break;

      case ORDER_CANCEL_REJECT_TR: {
        std::cout << "ORDER_CANCEL_REJECT_TR: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;
        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_MOD_REJECT_TR: {
        std::cout << "ORDER_MOD_REJECT_TR: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;
        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_MOD_REJECT: {
        std::cout << "ORDER_MOD_REJECT: ";
        HFSAT::NSE::ProcessedOrderResponseNonTR* processed_order_response_non_tr_;
        processed_order_response_non_tr_ = nse_msgs_handler_->order_response_non_tr_.ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_non_tr_->ToString() << std::endl;

      } break;

      case ORDER_ERROR_TR: {
        std::cout << "ORDER_ERROR_TR: ";
        HFSAT::NSE::ProcessedOrderResponse* processed_order_response_;
        std::cout << "Received Order Reject : Reason : " << processed_response_header->error_code << "\n" << std::endl;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_order_response_ = nse_msgs_handler_->order_response_->ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_->ToString() << std::endl;
        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case ORDER_ERROR: {
        std::cout << "ORDER_ERROR: ";
        HFSAT::NSE::ProcessedOrderResponseNonTR* processed_order_response_non_tr_;
        std::cout << " Reason : " << processed_response_header->error_code << "\n" << std::endl;
        processed_order_response_non_tr_ = nse_msgs_handler_->order_response_non_tr_.ProcessOrderResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_order_response_non_tr_->ToString() << std::endl;
      } break;

      case TRADE_CONFIRMATION_TR: {
        std::cout << "TRADE_CONFIRMATION_TR: ";
        HFSAT::NSE::ProcessedTradeConfirmationResponse* processed_trade_response_;
        msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
        processed_trade_response_ = nse_msgs_handler_->trade_conf_response_->ProcessTradeConfirmationResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " "
                  << ((HFSAT::NSE::ProcessedTradeConfirmationResponse*)(processed_trade_response_))->ToString()
                  << std::endl;

        msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      } break;

      case SYSTEM_INFORMATION_OUT: {
        std::cout << "SYSTEM_INFORMATION_OUT: ";
        HFSAT::NSE::ProcessedSystemInformationResponse* processed_system_information_response_;
        processed_system_information_response_ =
            nse_msgs_handler_->system_info_response_->ProcessSystemInfoResponse(msg_ptr);
        std::cout << "error_code: " << error_code << " " << processed_system_information_response_->ToString()
                  << std::endl;

      } break;

      case UPDATE_LOCALDB_HEADER: {
        std::cout << "error_code: " << error_code << " "
                  << " UPDATE_LOCALDB_HEADER \n" << std::endl;
      } break;

      case UPDATE_LOCALDB_DATA: {
        std::cout << "UPDATE_LOCALDB_DATA\n";
        // Not processing LocalDB data template. 1. irrelevant 2. in FO, doesn't follow the structure defined in docs:
      } break;
      case UPDATE_LOCALDB_TRAILER: {
        std::cout << "error_code: " << error_code << " "
                  << " UPDATE_LOCALDB_TRAILER \n" << std::endl;
      } break;
      
      case BATCH_ORDER_CANCEL: {
       std::cout << "BATCH ORDER CANCELLATION " << "error_code: " << error_code << " \n" << std::endl;        
      } break;

      default: {
        std::cerr << "Unexpected Reply : " << processed_response_header->transaction_code << std::endl;
	       } break;
    }
    size_t msg_len =
        (processed_packet_header->packet_length - NSE_RESPONSE_MESSAGE_HEADER_LENGTH - NSE_PACKET_RESPONSE_LENGTH);
    reader1.read(nse_msg_ptr, msg_len);
  }
}
