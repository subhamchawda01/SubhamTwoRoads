/**
 \file Tools/sgx_offline_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 354, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "infracore/TMXT/TMXSAIL/TMXMessageDefs.hpp"

void ParseAuditIn(HFSAT::BulkFileReader &reader);
void ParseAuditOut(HFSAT::BulkFileReader &reader);
void PrintOutMessage(char str[1000], uint32_t len);
void PrintInMessage(char str[1000], uint32_t len);

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <AUDIT_IN/AUDIT_OUT> <input-file>" << std::endl;
    exit(-1);
  }

  std::string option(argv[1]);
  std::string filename(argv[2]);

  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(filename);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open Audit File : " << filename << "\n";
    exit(-1);
  }

  if (option == "AUDIT_IN") {
    ParseAuditIn(bulk_file_reader_);
  } else if (option == "AUDIT_OUT") {
    ParseAuditOut(bulk_file_reader_);
  } else {
    std::cout << "Invalid Option " << option << ". Exiting." << std::endl;
  }

  return 0;
}

void ParseAuditIn(HFSAT::BulkFileReader &reader) {
  char str[1000];
  size_t LEN_FILED_SIZE = sizeof(uint32_t);
  uint32_t message_len = 0;
  size_t read_len = reader.read(&message_len, LEN_FILED_SIZE);

  while (true) {
    if (read_len < LEN_FILED_SIZE) break;
    memset(str, 0, 1000);

    uint32_t total_bytes_to_read = ((message_len / 4) + 1) * 4;

    read_len = reader.read(&str, total_bytes_to_read);
    std::cout << str << " " << read_len << " " << total_bytes_to_read << std::endl;
    if (read_len < total_bytes_to_read) {
      break;
    }

    PrintInMessage(str, total_bytes_to_read);

    read_len = reader.read(&message_len, LEN_FILED_SIZE);
    // std::cout << "ReadLen: " << read_len << " MessageLen: " << message_len << std::endl;
  }
}

void PrintInMessage(char str[1000], uint32_t len) {
  switch (*((uint16_t *)str)) {
    case Fast_Msg_TH_Value_: {
      // std::cout << "Heartbeat" << std::endl;
    } break;

    case Fast_Msg_TK_Value_: {
      // Connection ack, translation : logged in.

      std::cout << "Login Ack" << std::endl;
    } break;

    case Fast_Msg_TL_Value_: {
      // Disconnection acknowledgement.
      std::cout << "Logout Ack" << std::endl;

    } break;

    case Fast_Msg_KE_Value_: {
      // Bug fix for ever growing seq_num_to_saos vector -S
      // Even if we don't use it, we need to extract it, to
      // reduce size of vectors maintaining the corresponding information.
      std::cout << "Order Conf" << std::endl;

    } break;

    case Fast_Msg_KM_Value_: {
      // Bug fix for ever growing seq_num_to_saos vector -S
      // Even if we don't use it, we need to extract it, to
      std::cout << "Modify Conf" << std::endl;

    } break;

    case Fast_Msg_KZ_Value_: {
      // Bug fix for ever growing seq_num_to_saos vector -S
      // Even if we don't use it, we need to extract it, to
      // reduce size of vectors maintaining the corresponding information.
      std::cout << "Cancel Conf" << std::endl;

    } break;

    case Fast_Msg_NT_Value_: {
      // This will probably return -1, since the seq_num was already removed
      // in the confirmation message we received earlier.
      // Bug fix for ever growing seq_num_to_saos vector -S
      // Even if we don't use it, we need to extract it, to
      // reduce size of vectors maintaining the corresponding information.
      std::cout << "Order Exec" << std::endl;

    } break;

    case Fast_Msg_NL_Value_: {
      // This will probably return -1, since the seq_num was already removed
      // in the confirmation message we received earlier.
      // Bug fix for ever growing seq_num_to_saos vector -S
      // Even if we don't use it, we need to extract it, to
      // reduce size of vectors maintaining the corresponding information.
      std::cout << "Order Exec" << std::endl;
    }

    case Fast_Msg_ER_Value_: {
      // Generic Error message, this may or may not be in response to a new order or cancel order msg.
      // We decide by reading the sequence no. which invoked the sending of this message.
      std::cout << "Reject" << std::endl;

    } break;

    case Fast_Msg_NZ_Value_: {
      // Bug fix for ever growing seq_num_to_saos vector -S
      // Even if we don't use it, we need to extract it, to
      // reduce size of vectors maintaining the corresponding information.
      std::cout << "Cancel Conf" << std::endl;

    } break;

    case Fast_Msg_NX_Value_: {
      // Execution Cancellation Notice => Trade bust
      // Good design dictates this be done elsewhere, but optimization requirements force me to do this here.
      // We sent out the server assigned sequence no. in the owner data region of the message.
      // Get it back.

      // Assuming that the server assigned sequence no. is never more than max_saos_length_ decimal digits.
      std::cout << "Trade Bust" << std::endl;

    } break;

    case Fast_Msg_NY_Value_: {
      // Leg Execution Cancellation Notice => Leg Trade bust
      // Good design dictates this be done elsewhere, but optimization requirements force me to do this here.
      // We sent out the server assigned sequence no. in the owner data region of the message.
      // Get it back.

      std::cout << "Leg Trade Bust" << std::endl;

    } break;

    case Fast_Msg_TE_Value_: {
      // Technical Error.
      // This means something was wrong with one of the following:
      // User_Connection, User_Disconnection, Disconnection_Instruction.
      // Don't really do anything. Just dump it to the log for me to take a look at later.
      // dbglogger_ << "TMXEngine => Technical error : " << msg_buf_ << "\n";
      // dbglogger_.DumpCurrentBuffer();
      std::cout << "Technical Error" << std::endl;

    } break;

    case Fast_Msg_TO_Value_: {
      // Out of sequence msg.
      // Specs say we must reconnect.
      // << "TMXEngine => Out of sequence : " << msg_buf_ << "\n";
      // dbglogger_ << "TMXEngine => Reconnecting.\n";
      // dbglogger_.DumpCurrentBuffer();

      std::cout << "Out Of Sequence. Reconnecting." << std::endl;

    } break;

    case Fast_Msg_TI_Value_: {
      std::cout << "End Of Transmission" << std::endl;
      // End of transmission.
      // We are now disconnected.
      // dbglogger_ << "TMXEngine => End of transmission : " << msg_buf_ << "\n";
      // dbglogger_ << "TMXEngine => Disconnecting.\n";
      // dbglogger_.DumpCurrentBuffer();

    } break;

    case Fast_Msg_TM_Value_: {
      // Disconnection instruction ack.
      std::cout << "Disconnection Instruction Ack" << std::endl;
      // dbglogger_ << "TMXEngine => Cancel on Exit setup successfully with TMX\n";
      // dbglogger_.DumpCurrentBuffer();

    } break;

    default: { std::cout << "Unknown" << std::endl; } break;
  }
}

void ParseAuditOut(HFSAT::BulkFileReader &reader) {
  char str[1000];
  size_t LEN_FILED_SIZE = sizeof(uint32_t);
  uint32_t message_len = 0;
  size_t read_len = reader.read(&message_len, LEN_FILED_SIZE);

  while (true) {
    if (read_len < LEN_FILED_SIZE) break;
    memset(str, 0, 1000);

    uint32_t total_message_len = LEN_FILED_SIZE + message_len + 1;  // 1 for delimiter

    uint32_t total_bytes_to_read = ((total_message_len / 4) + 1) * 4 - LEN_FILED_SIZE;

    read_len = reader.read(&str, total_bytes_to_read);
    std::cout << str << " " << read_len << " " << total_message_len << " " << total_bytes_to_read << std::endl;
    if (read_len < total_bytes_to_read) {
      break;
    }

    PrintOutMessage(str, total_bytes_to_read);

    read_len = reader.read(&message_len, LEN_FILED_SIZE);
    // std::cout << "ReadLen: " << read_len << " MessageLen: " << message_len << std::endl;
  }
}

void PrintOutMessage(char str[1000], uint32_t len) {
  if (str[0] == 'O' && str[1] == 'E') {
    std::cout << "New Order" << std::endl;
  } else if (str[0] == 'X' && str[1] == 'E') {
    std::string order_time(str + 2, 6);
    std::string trader_id(str + 8, 8);
    std::string seq_num(str + 16, 8);
    std::string group_code(str + 24, 2);
    std::string instrument_code(str + 26, 4);
    std::string order_id(str + 30, 8);

    std::cout << "Cancel Order Time: " << order_time << " TraderId: " << trader_id << " SeqNum: " << seq_num
              << " GroupCode: " << group_code << " InstCode: " << instrument_code << " OrderId: " << order_id
              << std::endl;
  } else if (str[0] == 'O' && str[1] == 'M') {
    std::cout << "Modify Order" << std::endl;
  } else if (str[0] == 'T' && str[1] == 'C') {
    std::cout << "User Connection" << std::endl;
  } else if (str[0] == 'T' && str[1] == 'D') {
    std::cout << "User Disconnection" << std::endl;
  } else if (str[0] == 'T' && str[1] == 'A') {
    std::cout << "Disconnect Instructions" << std::endl;
  } else {
    std::cout << "Unknown" << std::endl;
  }
}
