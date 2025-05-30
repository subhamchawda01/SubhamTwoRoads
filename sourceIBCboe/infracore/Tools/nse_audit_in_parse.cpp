#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include "string.h"
#include <fstream>
#include <time.h>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "infracore/NSET/NSEEngine.hpp"
#include "infracore/NSET/nse_tap_invitation_manager.hpp"
#include "infracore/NSET/nse_msg_handler.hpp"
#include "infracore/NSET/nse_msg_handler_cash_market.hpp"
#include "infracore/NSET/nse_msg_handler_derivatives.hpp"
#include "infracore/NSET/NSETemplates/OrderPriceChangeRequest.hpp"
#include "infracore/NSET/NSETemplates/cash_market_order_structure_defines.hpp"
#include "infracore/NSET/NSETemplates/TradeConfirmationResponseDerivatives.hpp"

#define ziffies_to_sec 65536
#define epoch_nse_start_diff 315532800
void ParseAuditIn(std::string filename);

HFSAT::NSE::NseMsgHandler* nse_msgs_handler_;
std::string segment;

std::string nse_to_epoch(uint64_t timestamp) {
  time_t time = epoch_nse_start_diff+timestamp;
  struct tm *tm = localtime(&time);
  char date[32];
  strftime(date,sizeof(date),"%Y%m%d-%H:%M:%S",tm);
  return std::string(date);
}

std::string ziffiestounix(uint64_t time_stamp_ziffies) {
  time_t time = epoch_nse_start_diff+(time_stamp_ziffies/65536) ; 
  struct tm *tm = localtime(&time);
  char date[32];
  strftime(date,sizeof(date),"%Y%m%d-%H:%M:%S",tm);
  return std::string(date);
}

typedef struct OutputFO {
  uint16_t transaction_code;
  uint32_t log_time;
  uint32_t trader_id;
  uint64_t time_stamp;
  uint64_t time_stamp1;
  uint64_t time_stamp2;
  uint64_t response_order_no;
  char broker_id[6];
  char account_no[11];
  uint16_t  buy_sell;
  uint32_t original_vol;
  uint32_t disclosed_val;
  uint32_t remain_vol;
  uint32_t disclosed_val_remain;
  uint32_t good_till_date;
  double price;
  uint32_t fill_no;
  uint32_t fill_qty;
  double fill_price;
  uint32_t vol_filled_today;
  char activity_type[3];
  uint32_t activity_time;
  uint32_t token;
  char instruement[7];
  char symbol[11];
  uint32_t expiry;
  uint32_t strike_price;
  char option_type;
  char open_close;
  char book_type;
  char participant[13];
  double trade_val;

  std::string ToString() {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2)
       << transaction_code << ","
       << nse_to_epoch(log_time) << ","
       << trader_id << ","
       << ziffiestounix(time_stamp) << ","
       << ziffiestounix(time_stamp1) << ","
       << time_stamp2 << ","
       << response_order_no << ","
       << std::string(broker_id) << ","
       << std::string(account_no) << ","
       << buy_sell << ","
       << original_vol << ","
       << disclosed_val << ","
       << remain_vol << ","
       << disclosed_val_remain << ","
       << price << ","
       << good_till_date << ","
       << fill_no << ","
       << fill_qty << ","
       << fill_price << ","
       << vol_filled_today << ","
       << activity_type << ","
       << nse_to_epoch(activity_time) << ","
       << token << ","
       << std::string(instruement) << ","
       << std::string(symbol) << ","
       << expiry << ","
       << strike_price << ","
       << option_type << ","
       << open_close << ","
       << book_type << ","
       << participant << ","
        << trade_val << ",";
       return ss.str();
  }
 }OutputFO;

typedef struct OutputCM {
  uint16_t transaction_code;
  uint32_t log_time;
  uint32_t user_id;
  uint64_t time_stamp;
  uint64_t time_stamp1;
  uint64_t response_order_no;
  char time_stamp2;
  char broker_id[6];
  uint32_t trader_num;
  uint16_t buy_sell;
  char account_no[11];
  uint32_t original_vol;
  uint32_t disclosed_val;
  uint32_t remain_vol;
  uint32_t disclosed_val_remain;
  double  price;
  uint32_t good_till_date;
  uint32_t fill_no;
  uint32_t fill_qty;
  double fill_price;
  uint32_t vol_filled_today;
  char activity_type[3];
  uint32_t activity_time;
  char symbol[11];
  char series[3];
  uint16_t book_type;
  uint16_t pro_client;
  double trade_val;
  std::string ToString() {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) 
       <<transaction_code << ","
       << nse_to_epoch(log_time) << ","
       << user_id << "," << "," << ","
       << time_stamp2 << ","
       << response_order_no << ","
       << std::string(broker_id) << ","
       << std::string(account_no) << ","
       //<< trader_num << ","
       << buy_sell << ","
       << original_vol << ","
       << disclosed_val << ","
       << remain_vol << ","
       << disclosed_val_remain << ","
       << disclosed_val_remain << ","
       //<< good_till_date << ","
       << price << ","
       << fill_no << ","  << fill_no  <<  ","
       << fill_qty << ","
       << fill_price << ","
       << vol_filled_today << ","
       << std::string(activity_type) << ","
       << nse_to_epoch(activity_time) << ","
       << std::string(series) << ","
       << std::string(symbol) << ","
       << "-1,-1,XX,0,"
       << book_type << ","
       <<"-1,"
      //  << std::fixed << std::setprecision(2)
       // Just to match the format of the older file
       << trade_val;
    return ss.str();
  }
}OutputCM;




OutputFO* ProcessFOPacket(const char* ptr) {
  OutputFO* output = new OutputFO();
  output->transaction_code = ntoh16((*((uint16_t*)(ptr+NSE_TRADE_RESPONSE_TRANSACTIONCODE_OFFSET))));
  output->log_time = ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_LOGTIME_OFFSET)));
  output->trader_id = ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_TRADERID_OFFSET)));
  output->time_stamp = ntoh64((*(uint64_t*)(ptr+NSE_TRADE_RESPONSE_TIMESTAMP_OFFSET)));
  output->time_stamp1 = ntoh64(*(uint64_t*)(ptr+NSE_TRADE_RESPONSE_TIMESTAMP1_OFFSET));
  output->time_stamp2 = ntoh64(*((uint64_t*)(ptr+NSE_TRADE_RESPONSE_TIMESTAMP2_OFFSET))); 
  output->response_order_no = ntoh64(*((uint64_t*)(ptr+NSE_TRADE_RESPONSE_ORDER_NUM_OFFSET)));
  memcpy((void*)output->broker_id,(void*)(ptr+NSE_TRADE_RESPONSE_BROKERID_OFFSET),NSE_TRADE_RESPONSE_ORDER_NUM_LENGTH);
  output->broker_id[5] = '\0';
  memcpy((void*)output->account_no,(void*)(ptr+NSE_TRADE_RESPONSE_ACCOUNTNUMBER_OFFSET),NSE_TRADE_RESPONSE_ACCOUNTNUMBER_LENGTH);
  output->account_no[9] = '\0';
  output->buy_sell = ntoh16((*((uint16_t*)(ptr+NSE_TRADE_RESPONSE_BUYSELLINDICATOR_OFFSET))));
  output->original_vol = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_ORIGINALVOLUME_OFFSET))));
  output->disclosed_val = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_DISCLOSEDVOLUME_OFFSET))));
  output->remain_vol = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_REMAININGVOLUME_OFFSET))));
  output->disclosed_val_remain = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_DISCLOSEDVOLUMEREMAINING_OFFSET))));
  output->price = ((double)ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_PRICE_OFFSET))))*0.01;
  output->good_till_date = ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_GOODTILLDATE_OFFSET)));
  output->fill_no = ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_FILLNUMBER_OFFSET)));
  output->fill_qty = ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_FILLQUANTITY_OFFSET)));
  output->fill_price = ((double)ntoh32((*(uint32_t*)(ptr+NSE_TRADE_RESPONSE_FILLPRICE_OFFSET)))) * 0.01;
  output->vol_filled_today = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_VOL_FILLED_TODAY_OFFSET))));
  memcpy((void*)output->activity_type,(void*)(ptr+NSE_TRADE_RESPONSE_ACTIVITY_TYPE_OFFSET),NSE_TRADE_RESPONSE_ACTIVITY_TYPE_LENGTH);
  output->activity_type[2] = '\0';
  output->activity_time = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_ACTIVITY_TIME_OFFSET))));
  output->token = ntoh32(*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_TOKENNUM_OFFSET)));
  memcpy((void*)output->instruement ,(void*)(ptr+NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_OFFSET),NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_LENGTH);
  output->instruement[6] = '\0';
  memcpy((void*)output->symbol,(void*)(ptr+NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_SYMBOL_OFFSET),NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_SYMBOL_LENGTH);
  output->symbol[10] = '\0';
  output->expiry = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_EXPIRYDATE_OFFSET))));
  output->strike_price = ntoh32((*((uint32_t*)(ptr+NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_STRIKEPRICE_OFFSET))));
  output->option_type = (*((char*)(ptr+NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_OPTIONTYPE_OFFSET)));
  output->open_close = (*((char*)(ptr+NSE_TRADE_RESPONSE_OPENCLOSE_OFFSET)));
  output->book_type = (*((char*)(ptr+NSE_TRADE_RESPONSE_BOOK_TYPE_OFFSET)));
  memcpy((void*)output->participant,(void*)(ptr+NSE_TRADE_RESPONSE_PARTICIPANT_OFFSET),NSE_TRADE_RESPONSE_PARTICIPANT_LENGTH);
  output->participant[12] = '\0';
  output->trade_val = output->fill_qty *  output->fill_price;
  return output;
}

OutputCM* ProcessCMPacket(const char *ptr) {
  OutputCM* output = new OutputCM();
  output->transaction_code = ntoh16((*((uint16_t*)(ptr+NSE_CM_TRADE_RESPONSE_TRANSACTIONCODE_OFFSET))));
  output->log_time = ntoh32((*(uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_LOGTIME_OFFSET)));
  output->user_id = ntoh32((*(uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_CM_TRADERID_OFFSET)));
  output->time_stamp = ntoh64((*(uint64_t*)(ptr+NSE_CM_TRADE_RESPONSE_TIMESTAMP_OFFSET)));
  output->time_stamp1 = ntoh64(*((uint64_t*)(ptr+NSE_CM_TRADE_RESPONSE_TIMESTAMP1_OFFSET)));
  output->response_order_no = ntoh64(*((uint64_t*)(ptr+NSE_CM_TRADE_RESPONSE_ORDER_NUM_OFFSET)));
  output->time_stamp2 = (*(char*)(ptr+NSE_CM_TRADE_RESPONSE_TIMESTAMP2_OFFSET));
  memcpy((void*)output->broker_id,(void*)(ptr+NSE_CM_TRADE_RESPONSE_BROKERID_OFFSET),NSE_CM_TRADE_RESPONSE_BROKERID_LENGTH);
  output->broker_id[5] = '\0';
  output->trader_num = ntoh32((*((uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_TRADERNUM_OFFSET))));
  output->buy_sell = ntoh16((*((uint16_t*)(ptr+NSE_CM_TRADE_RESPONSE_BUYSELLINDICATOR_OFFSET))));
  memcpy((void*)output->account_no,(void*)(ptr+NSE_CM_TRADE_RESPONSE_ACCOUNTNUMBER_OFFSET),NSE_CM_TRADE_RESPONSE_ACCOUNTNUMBER_LENGTH);
  output->account_no[10] = '\0';
  output->original_vol = ntoh32((*((uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_ORIGINALVOLUME_OFFSET))));
  output->disclosed_val = ntoh32((*((uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_DISCLOSEDVOLUME_OFFSET))));
  output->remain_vol = ntoh32((*((uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_REMAININGVOLUME_OFFSET))));
  output->disclosed_val_remain = ntoh32((*((int32_t*)(ptr+NSE_CM_TRADE_RESPONSE_DISCLOSEDVOLUMEREMAINING_OFFSET))));
  output->price = (ntoh32((*(uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_PRICE_OFFSET))))*0.01;
  output->good_till_date = ((double)ntoh32((*(uint32_t*)(ptr+NSE_CM_ORDERENTRY_REQUEST_GOODTILLDATE_OFFSET))));
  output->fill_no = ntoh32((*(uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_FILLNUMBER_OFFSET)));
  output->fill_qty = ntoh32((*(uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_FILLQUANTITY_OFFSET)));
  output->fill_price = (ntoh32((*(uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_FILLPRICE_OFFSET))))  * 0.01;
  output->vol_filled_today = ntoh32((*((uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_VOL_FILLED_TODAY_OFFSET))));
  memcpy((void*)output->activity_type,(void*)(ptr+NSE_CM_TRADE_RESPONSE_ACTIVITY_TYPE_OFFSET),NSE_CM_TRADE_RESPONSE_ACTIVITY_TYPE_LENGTH);
  output->activity_type[2] = '\0';
  output->activity_time = ntoh32((*((uint32_t*)(ptr+NSE_CM_TRADE_RESPONSE_ACTIVITY_TIME_OFFSET))));
  memcpy((void*)(output->symbol),(ptr+NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SYMBOL_OFFSET),NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SYMBOL_LENGTH);
  output->symbol[10] = '\0';
  memcpy((void*)(output->series),(ptr+NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SERIES_OFFSET),NSE_CM_TRADE_RESPONSE_CONTRACT_SECINFO_STRUCT_SERIES_LENGTH);
  output->series[2] = '\0';
  output->book_type = ntoh16((*((uint16_t*)(ptr+NSE_CM_TRADE_RESPONSE_BOOK_TYPE_OFFSET))));
  output->pro_client = ntoh16((*((uint16_t*)(ptr+NSE_CM_TRADE_RESPONSE_PROCLIENT_OFFSET))));
  output->trade_val = output->fill_qty *  output->price;
  return output;
}


void ParseAuditIn(std::string filename,std::string output_file) {
  HFSAT::BulkFileReader reader1, reader2;
  reader1.open(filename);
  reader2.open(filename);
  std::ofstream fileoutstream;
  fileoutstream.open(output_file,std::ofstream::app); 
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
    reader2.read(nse_msg_ptr, processed_packet_header->packet_length);
    const char* msg_ptr = nse_msg_ptr;
    msg_ptr += NSE_PACKET_RESPONSE_LENGTH;
    msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
    if ( processed_response_header->transaction_code ==  TRADE_CONFIRMATION_TR ) {
      msg_ptr -= NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
      if(segment != "NSE_EQ") {
        fileoutstream << ProcessFOPacket(msg_ptr)->ToString() << "\n";
      }
      else {
        fileoutstream << ProcessCMPacket(msg_ptr)->ToString() << "\n";
      }
      msg_ptr += NSE_RESPONSE_MESSAGE_HEADER_LENGTH;
    }
    size_t msg_len =
        (processed_packet_header->packet_length - NSE_RESPONSE_MESSAGE_HEADER_LENGTH - NSE_PACKET_RESPONSE_LENGTH);
    reader1.read(nse_msg_ptr, msg_len);
  }
std::cout << output_file << std::endl;
  fileoutstream.flush();
  fileoutstream.close();
}


int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage : <exec> <NSE_FO/NSE_EQ/NSE_CD> <input-file> <outfile>" << std::endl;
    exit(-1);
  }

  std::string segment_(argv[1]);
  segment = segment_;
  std::string filename(argv[2]);
  std::string outfile(argv[3]);
  if (segment == "NSE_EQ") {
    nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerCashMarket();
  } else {
    nse_msgs_handler_ = new HFSAT::NSE::NseMsgHandlerDerivatives();
  } 
  ParseAuditIn(filename,outfile);
  return 0;
}
