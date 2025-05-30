#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include "string.h"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "infracore/BSE/BSEEngine.hpp"

#define epoch_bse_start_diff 315532800

void ParseAuditIn(std::string filename, std::string outfile);

std::string segment;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage : <exec> <BSE_EQ> <input-file> <output-file>" << std::endl;
    exit(-1);
  }

  std::string segment_(argv[1]);
  segment = segment_;
  std::string filename(argv[2]);
  std::string outfile(argv[3]);

  ParseAuditIn(filename, outfile);

  return 0;
}

std::string bse_to_epoch_time(uint64_t timestamp) {
  time_t time = epoch_bse_start_diff+timestamp;
  struct tm *tm = localtime(&time);
  char date[32];
  strftime(date,sizeof(date),"%H:%M:%S",tm);
  return std::string(date);
}

std::string bse_to_epoch_date(uint64_t timestamp) {
  time_t time = epoch_bse_start_diff+timestamp;
  struct tm *tm = localtime(&time);
  char date[32];
  strftime(date,sizeof(date),"%m/%d/%Y",tm);
  return std::string(date);
}

typedef struct OutputCM {
  std::string memberId;
  std::string traderId;
  int64_t scriptCode;
  std::string scriptId;
  int rate;
  int32_t qty;
  int32_t tradeStatus;
  int32_t cmCode;
  std::string time; 
  std::string date;
  std::string clientId;
  uint64_t orderId;
  char buySell;
  std::string clientType;
  int64_t locationId;
  std::string sessionId;
  char cpCodeConf;
  
  std::string ToString() {
    std::ostringstream ss;
    ss << memberId << "|"
       << traderId << "|" 
       << scriptCode << "|"
       << scriptId << "|"
       << rate << "|"
       << qty << "|"
       << tradeStatus << "|"
       << cmCode << "|"
       << time << "|"
       << date << "|"
       << clientId << "|"
       << orderId << "|"
       << buySell << "|"
       << clientType << "|"
       << locationId << "|"
       << sessionId << "|"
       << cpCodeConf << "|";
    return ss.str();
  }
}OutputCM;

void ParseAuditIn(std::string filename, std::string outfile) {

  std::vector<std::string> tokens_;
  HFSAT::PerishableStringTokenizer::StringSplit(filename, '.', tokens_);
  int yyyymmdd = std::stoi(tokens_[2]); 
  std::string user_id = tokens_[1];
  std::string member_id = user_id.substr(0,4);
  std::string trader_id = user_id.substr(5,4);
  HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd);
  HFSAT::BulkFileReader reader1;
  int64_t location_id = -1;

  if ( user_id == "670101001" )
    location_id = 5600490001001188;
  else if ( user_id == "670101002" )
    location_id = 5600490001002188;

  reader1.open(filename);

  std::ofstream fileoutstream;
  fileoutstream.open(outfile,std::ofstream::app);


  while (true) {
    char bse_msg_ptr[MAX_BSE_RESPONSE_BUFFER_SIZE];
    size_t available_len_1 = reader1.read(bse_msg_ptr, BSE_RESPONSE_HEADER_LENGTH);
    // we read the header from the file

    if (available_len_1 < BSE_RESPONSE_HEADER_LENGTH) break;
    uint32_t this_bse_message_bodylength_ = (uint32_t)(*((char*)(bse_msg_ptr)));
    uint16_t this_bse_template_id_ = *((uint16_t *)(bse_msg_ptr + 4));

    available_len_1 = reader1.read((bse_msg_ptr + BSE_RESPONSE_HEADER_LENGTH), (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH));
    if (available_len_1 < (this_bse_message_bodylength_ - BSE_RESPONSE_HEADER_LENGTH)) break;

    const char* msg_ptr = bse_msg_ptr;

    switch (this_bse_template_id_) {

      case TID_ORDER_EXEC_RESPONSE: {
        HFSAT::BSE::BSEOrderExecResponse process_order_exec_;
        OrderExecResponseT* order_exec_ = process_order_exec_.ProcessOrderExecResponse(msg_ptr);
        std::string exch_symbol = to_string(order_exec_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
     
        OutputCM* output = new OutputCM();
        
        output->memberId = member_id;
        output->traderId = trader_id;
        output->scriptCode = order_exec_->SecurityID;
        output->scriptId = shortcode.substr(4,(shortcode.length() -1));
        output->rate = (int)((order_exec_->FillsGrp[0].FillPx) / 1000000);
        output->qty = order_exec_->CumQty;
        output->tradeStatus = 11; // 11->OriginalTrade  12->ModifiedTrade  13->CancelledTrade  17->ApprovedTrade  18->Rejected Trade
        output->cmCode = 52017; // Clearing member code
        output->time = bse_to_epoch_time(order_exec_->ResponseHeaderME.SendingTime / 1000000000);
        output->date = bse_to_epoch_date(order_exec_->ResponseHeaderME.SendingTime / 1000000000);
        output->clientId = "OWN";
        output->orderId = order_exec_->OrderID;
        output->buySell = (order_exec_->InstrmntLegExecGrp[0].LegSide == 1) ? 'B' : 'S'; 
        output->clientType = "OWN";
        output->locationId = location_id;
        output->sessionId = trader_id;
        output->cpCodeConf = 'N';

        fileoutstream << output->ToString() << "\n";

      } break;

      case TID_ORDER_EXEC_NOTIFICATION: {
        HFSAT::BSE::BSEOrderExecNotification process_order_exec_notification_;
        OrderExecNotificationT* order_exec_notification_ = process_order_exec_notification_.ProcessOrderExecNotification(msg_ptr);
        std::string exch_symbol = to_string(order_exec_notification_->SecurityID);
        std::string shortcode = HFSAT::BSESecurityDefinitions::GetUniqueInstance(yyyymmdd).GetShortCodeFromExchangeId(exch_symbol);
     
        OutputCM* output = new OutputCM();

        output->memberId = member_id;
        output->traderId = trader_id;
        output->scriptCode = order_exec_notification_->SecurityID;
        output->scriptId = shortcode.substr(4,(shortcode.length() -1));
        output->rate = (int)((order_exec_notification_->FillsGrp[0].FillPx) / 1000000);
        output->qty = order_exec_notification_->CumQty;
        output->tradeStatus = 11; // 11->OriginalTrade  12->ModifiedTrade  13->CancelledTrade  17->ApprovedTrade  18->Rejected Trade
        output->cmCode = 52017; // Clearing member code
        output->time = bse_to_epoch_time(order_exec_notification_->RBCHeaderME.SendingTime / 1000000000);
        output->date = bse_to_epoch_date(order_exec_notification_->RBCHeaderME.SendingTime / 1000000000);
        output->clientId = "OWN";
        output->orderId = order_exec_notification_->OrderID;
        output->buySell = (order_exec_notification_->Side == 1) ? 'B' : 'S'; 
        output->clientType = "OWN";
        output->locationId = location_id;
        output->sessionId = trader_id;
        output->cpCodeConf = 'N';
        
        fileoutstream << output->ToString() << "\n";

      } break;

      default: {

      } break;
    }

  }
//  std::cout << outfile << std::endl;
  fileoutstream.flush();
  fileoutstream.close();
}
