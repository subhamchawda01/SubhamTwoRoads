// =====================================================================================
//
//       Filename:  DataDefines.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/28/2015 04:49:14 PM
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

#include <cinttypes>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include "dvccode/Utils/md5.hpp"

#define hton16 htons
#define hton32 __builtin_bswap32
#define hton64 __builtin_bswap64

#define ntoh16 ntohs
#define ntoh32 __builtin_bswap32
#define ntoh64 __builtin_bswap64

#define NSE_MAX_SAOS_TO_METADATA_STORAGE 262144

#define NSE_REQUEST_START_OFFSET 0
#define NSE_RESPONSE_START_OFFSET 0

#define NSE_FIXED_DECIMAL_VALUE_FOR_PRICE_FACTOR 2
#define MAX_NSE_RESPONSE_BUFFER_SIZE 65536

//=========================================================================  NSE LENGTHS
//=====================================================//

#define PACKET_REQUEST_MD5SUM_SIZE 16
#define PACKET_RESPONSE_MD5SUM_SIZE 16
#define LENGTH_OF_MESSAGE_HEADER_ALPHACHAR_CHAR_FIELD 2
#define LENGTH_OF_MESSAGE_HEADER_TIMESTAMP_CHAR_FIELD 8
#define LENGTH_OF_INNERMESSAGE_HEADER_ALPHACHAR_CHAR_FIELD 2
#define LENGTH_OF_INNERMESSAGE_HEADER_TIMESTAMP_CHAR_FIELD 8
#define LENGTH_OF_LOGON_REQUEST_PASSWORD_CHAR_FIELD 8
#define LENGTH_OF_LOGON_REQUEST_TRADERNAME_CHAR_FIELD 26
#define LENGTH_OF_LOGON_REQUEST_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_LOGON_REQUEST_COLOUR_CHAR_FIELD 50
#define LENGTH_OF_LOGON_REQUEST_WSCLASSNAME_CHAR_FIELD 14
#define LENGTH_OF_LOGON_REQUEST_BROKERNAME_CHAR_FIELD 25
#define LENGTH_OF_LOGON_RESPONSE_PASSWORD_CHAR_FIELD 8
#define LENGTH_OF_LOGON_RESPONSE_TRADERNAME_CHAR_FIELD 26
#define LENGTH_OF_LOGON_RESPONSE_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_LOGON_RESPONSE_COLOUR_CHAR_FIELD 50
#define LENGTH_OF_LOGON_RESPONSE_WSCLASSNAME_CHAR_FIELD 14
#define LENGTH_OF_LOGON_RESPONSE_BROKERNAME_CHAR_FIELD 25

#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_CHAR_FIELD 6
#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_CHAR_FIELD 10
#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_COUNTERPARTYBROKERID_CHAR_FIELD 5
#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_ACCOUNTNUMBER_CHAR_FIELD 10
#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_CORDFILLER_CHAR_FIELD 24
#define LENGTH_OF_NSE_ORDERENTRY_REQUEST_SETTLOR_CHAR_FIELD 12

#define LENGTH_OF_NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_CHAR_FIELD 6
#define LENGTH_OF_NSE_CHANGE_REQUEST_CONTRACT_DESC_STRUCT_SYMBOL_CHAR_FIELD 10
#define LENGTH_OF_NSE_CHANGE_REQUEST_ACCOUNTNUMBER_CHAR_FIELD 10
#define LENGTH_OF_NSE_CHANGE_REQUEST_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_NSE_CHANGE_REQUEST_SETTLOR_CHAR_FIELD 12

#define LENGTH_OF_NSE_ORDER_RESPONSE_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_CHAR_FIELD 6
#define LENGTH_OF_NSE_ORDER_RESPONSE_CONTRACT_DESC_STRUCT_SYMBOL_CHAR_FIELD 10
#define LENGTH_OF_NSE_ORDER_RESPONSE_ACCOUNTNUMBER_CHAR_FIELD 10
#define LENGTH_OF_NSE_ORDER_RESPONSE_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_NSE_ORDER_RESPONSE_SETTLOR_CHAR_FIELD 12

#define LENGTH_OF_NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_INSTRUMENTNAME_CHAR_FIELD 6
#define LENGTH_OF_NSE_TRADE_RESPONSE_CONTRACT_DESC_STRUCT_SYMBOL_CHAR_FIELD 10
#define LENGTH_OF_NSE_TRADE_RESPONSE_ACCOUNTNUMBER_CHAR_FIELD 10
#define LENGTH_OF_NSE_TRADE_RESPONSE_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_NSE_TRADE_RESPONSE_SETTLOR_CHAR_FIELD 12
#define LENGTH_OF_NSE_TRADE_RESPONSE_PARTICIPANT_CHAR_FIELD 12

////////////////////////////////////////////////////////////////////////////////////////////////////////
////                                          CASH MARKET LENGTHS                                   ////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LENGTH_OF_NSE_CM_CONTRACT_SECINFO_STRUCT_SYMBOL_CHAR_FIELD 10
#define LENGTH_OF_NSE_CM_ACCOUNTNUMBER_CHAR_FIELD 10
#define LENGTH_OF_NSE_CM_BROKERID_CHAR_FIELD 5
#define LENGTH_OF_NSE_CM_SETTLOR_CHAR_FIELD 12
#define LENGTH_OF_CM_LOGON_REQUEST_BROKERNAME_CHAR_FIELD 26
#define LENGTH_OF_CM_LOGON_REQUEST_RESERVED_2_LENGTH 56

//=========================================================================  NSE Transaction Codes
//=====================================================// Extracted From API //@ravi

#define SYSTEM_INFORMATION_IN 1600
#define SYSTEM_INFORMATION_OUT 1601
#define EXCH_PORTF_IN 1775
#define EXCH_PORTF_OUT 1776
#define RPRT_MARKET_STATS_OUT_RPT 1833
#define BOARD_LOT_IN 2000
#define NEG_ORDER_TO_BL 2008
#define NEG_ORDER_BY_CPID 2009
#define PRICE_MOD_IN 2013
#define ORDER_MOD_IN 2040
#define ORDER_MOD_REJECT 2042
#define ORDER_CANCEL_IN 2070
#define ORDER_CANCEL_REJECT 2072
#define ORDER_CONFIRMATION 2073
#define ORDER_MOD_CONFIRMATION 2074
#define ORDER_CANCEL_CONFIRMATION 2075
#define CANCEL_NEG_ORDER 2076
#define SP_BOARD_LOT_IN 2100
#define TWOL_BOARD_LOT_IN 2102
#define THRL_BOARD_LOT_IN 2104
#define SP_ORDER_CANCEL_IN 2106
#define SP_ORDER_MOD_IN 2118
#define SP_ORDER_CONFIRMATION 2124
#define TWOL_ORDER_CONFIRMATION 2125
#define THRL_ORDER_CONFIRMATION 2126
#define SP_ORDER_CXL_REJ_OUT 2127
#define SP_ORDER_CXL_CONFIRMATION 2130
#define TWOL_ORDER_CXL_CONFIRMATION 2131
#define THRL_ORDER_CXL_CONFIRMATION 2132
#define SP_ORDER_MOD_REJ_OUT 2133
#define SP_ORDER_MOD_CON_OUT 2136
#define SP_ORDER_ERROR 2154
#define TWOL_ORDER_ERROR 2155
#define THRL_ORDER_ERROR 2156
#define FREEZE_TO_CONTROL 2170
#define ON_STOP_NOTIFICATION 2212
#define TRADE_CONFIRMATION 2222
#define TRADE_ERROR 2223
#define ORDER_ERROR 2231
#define TRADE_CANCEL_CONFIRM 2282
#define TRADE_CANCEL_REJECT 2286
#define TRADE_MODIFY_CONFIRM 2287
#define TRADE_MODIFY_REJECT 2288
#define SIGN_ON_REQUEST_IN 2300
#define SIGN_ON_REQUEST_OUT 2301
#define SIGN_OFF_REQUEST_IN 2320
#define SIGN_OFF_REQUEST_OUT 2321
#define GR_REQUEST_IN 2400
#define GR_REQUEST_OUT 2401
#define BoxLogin_REQUEST_IN 23000
#define BoxLogin_REQUEST_OUT 23001
#define BOX_SIGN_OFF 20322
#define EX_PL_ENTRY_IN 4000
#define EX_PL_ENTRY_OUT 4001
#define EX_PL_CONFIRMATION 4002
#define EX_PL_MOD_IN 4005
#define EX_PL_MOD_CONFIRMATION 4007
#define EX_PL_CXL_IN 4008
#define EX_PL_CXL_OUT 4009
#define EX_PL_CXL_CONFIRMATION 4010
#define GIVEUP_APPROVED_IN 4500
#define GIVEUP_APPROVED_OUT 4501
#define GIVEUP_APP_CONFIRM 4502
#define GIVEUP_REJECTED_IN 4503
#define GIVEUP_REJECTED_OUT 4504
#define GIVEUP_REJ_CONFIRM 4505
#define GIVEUP_APPROVE_ALL 4513
#define CTRL_MSG_TO_TRADER 5295
#define TRADE_CANCEL_IN 5440
#define TRADE_CANCEL_OUT 5441
#define TRADE_MOD_IN 5445
#define SECURITY_OPEN_PRICE 6013
#define BCAST_JRNL_VCT_MSG 6501
#define BC_OPEN_MESSAGE 6511
#define BC_CLOSE_MESSAGE 6521
#define BC_PREOPEN_SHUTDOWN_MSG 6531
#define BC_CIRCUIT_CHECK 6541
#define BC_NORMAL_MKT_PREOPEN_ENDED 6571
#define DOWNLOAD_REQUEST 7000
#define HEADER_RECORD 7011
#define MESSAGE_RECORD 7021
#define TRAILER_RECORD 7031
#define MKT_MVMT_CM_OI_IN 7130
#define BCAST_MBO_MBP_UPDATE 7200
#define BCAST_MW_ROUND_ROBIN 7201
#define BCAST_TICKER_AND_MKT_INDEX 7202
#define BCAST_INDUSTRY_INDEX_UPDATE 7203
#define BCAST_SYSTEM_INFORMATION_OUT 7206
#define BCAST_ONLY_MBP 7208
#define BCAST_SECURITY_STATUS_CHG_PREOPEN 7210
#define BCAST_SPD_MBP_DELTA 7211
#define BCAST_TRADE_EXECUTION_RANGE 7220
#define UPDATE_LOCALDB_IN 7300
#define UPDATE_LOCALDB_DATA 7304
#define BCAST_SECURITY_MSTR_CHG 7305
#define BCAST_PART_MSTR_CHG 7306
#define UPDATE_LOCALDB_HEADER 7307
#define UPDATE_LOCALDB_TRAILER 7308
#define BCAST_SECURITY_STATUS_CHG 7320
#define PARTIAL_SYSTEM_INFORMATION 7321
#define BCAST_INSTR_MSTR_CHG 7324
#define BCAST_INDEX_MSTR_CHG 7325
#define BCAST_INDEX_MAP_TABLE 7326
#define BATCH_ORDER_CANCEL 9002
#define BCAST_TURNOVER_EXCEEDED 9010
#define BROADCAST_BROKER_REACTIVATED 9011
#define BOARD_LOT_IN_TR 20000
#define PRICE_CONFIRMATION_TR 20012
#define ORDER_MOD_IN_TR 20040
#define ORDER_MOD_REJECT_TR 20042
#define ORDER_QUICK_CANCEL_IN_TR 20060
#define ORDER_CANCEL_IN_TR 20070
#define ORDER_CANCEL_REJECT_TR 20072
#define ORDER_CONFIRMATION_TR 20073
#define ORDER_MOD_CONFIRMATION_TR 20074
#define ORDER_CXL_CONFIRMATION_TR 20075
#define TRADE_CONFIRMATION_TR 20222
#define ORDER_ERROR_TR 20231
#define NSE_HEARTBEAT 23506
#define TAP_INVITATION_MESSAGE 15000

//================================================================   NSE Response Structs

namespace HFSAT {
namespace NSE {

struct ProcessedPacketHeader {
  int16_t packet_length;
  int32_t packet_sequnece_number;
  int16_t packet_message_count;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "PACKET_LENGTH -> " << packet_length << " PACKET_SEQUENCE_NUMBER -> " << packet_sequnece_number
               << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedResponseHeader {
  int16_t transaction_code;
  int32_t logtime;
  char alphachar[3];
  int32_t trader_id;
  int16_t error_code;
  char timestamp[9];
  char timestamp1[9];
  char timestamp2[9];
  int16_t message_length;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "HEADER_TXN_CODE -> " << transaction_code << " HEADER_LOGITME -> " << logtime << " HEADER_ALPHACHAR "
               << alphachar << "TRADER_ID -> " << trader_id << " HEADER_ERROR_CODE -> " << error_code
               << " HEADER_TIMESTAMP -> " << timestamp << " HEADER_TIMESTAMP1 -> " << timestamp1
               << " HEADER_TIMESTAMP2 -> " << timestamp2 << " HEADER_LENGTH -> " << message_length << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedGRResponse {

  int16_t box_id;
  char broker_id[6];
  char ip[16];
  int32_t port;
  char signon_key[8];

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "BOX -> " << (int32_t)box_id << " " 
               << "BROKER -> " << broker_id << " " 
               << "IP -> " << ip << " " 
               << "PORT -> " << port << " " 
               << "KEY -> " << signon_key << std::endl ;

    return t_temp_oss.str();
  }
};

struct ProcessedBoxLoginResponse {

  int16_t box_id;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "BOX -> " << (int32_t)box_id << std::endl ;
    return t_temp_oss.str();
  }
};

struct ProcessedBoxSignOffResponse {

  int16_t box_id;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "BOX -> " << (int32_t)box_id << std::endl ;
    return t_temp_oss.str();
  }
};


struct ProcessedLogonResponse {
  int32_t user_id;
  char trader_name[27];
  int32_t last_password_changed_date_time;
  char broker_id[6];
  int16_t branch_id;
  int32_t end_time;
  int16_t user_type;
  double sequence_number;
  char broker_status;
  int16_t broker_eligibility_per_mkt;
  int16_t member_type;
  char clearing_status;
  char broker_name[26];

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "LOGIN_USERID -> " << user_id << " LOGIN_TRADER -> " << trader_name << " LOGIN_LAST_PSWD_DATE -> "
               << last_password_changed_date_time << " LOGIN_BROKER -> " << broker_id << " LOGIN BRANCH -> "
               << branch_id << " LOGIN_ENDTIME -> " << end_time << " LOGIN_USERTYPE -> " << user_type
               << " LOGIN_SEQUENCE -> " << sequence_number << " LOGIN_BROKER_STATUS -> " << broker_status
               << " LOGIN_broker_eligibility_per_mkt -> " << broker_eligibility_per_mkt << " LOGIN_member_type -> "
               << member_type << " LOGIN_clearing_status -> " << clearing_status << " LOGIN_broker_name -> "
               << broker_name << "\n";

    return t_temp_oss.str();
  }
};

struct st_market_status {
  int16_t normal;
  int16_t oddlot;
  int16_t spot;
  int16_t auction;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "ST_MARKET_STATUS -> normal : " << normal << " "
               << "oddlot : " << oddlot << " "
               << "spot : " << spot << " "
               << "auction : " << auction << " ";

    return t_temp_oss.str();
  }
};

struct st_ex_market_status {
  int16_t normal;
  int16_t oddlot;
  int16_t spot;
  int16_t auction;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "ST_EX_MARKET_STATUS -> normal : " << normal << " "
               << "oddlot : " << oddlot << " "
               << "spot : " << spot << " "
               << "auction : " << auction << " ";

    return t_temp_oss.str();
  }
};

struct st_pl_market_status {
  int16_t normal;
  int16_t oddlot;
  int16_t spot;
  int16_t auction;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "ST_PL_MARKET_STATUS -> normal : " << normal << " "
               << "oddlot : " << oddlot << " "
               << "spot : " << spot << " "
               << "auction : " << auction << " ";

    return t_temp_oss.str();
  }
};

struct ProcessedSystemInformationResponse {
  st_market_status st_mkt_status;
  st_ex_market_status st_ex_mkt_status;
  st_pl_market_status st_pl_mkt_status;
  int16_t call_auction_1;  // required in cash market
  int16_t call_auction_2;  // required in cash market
  char update_portfolio;
  int32_t market_index;
  int16_t default_settlement_period;
  int16_t default_settlement_period_spot;
  int16_t default_settlement_period_auction;
  int16_t competitor_period;
  int16_t solicitor_period;
  int16_t warning_percent;
  int16_t volume_freeze_percent;
  int16_t snap_quote_time;
  int32_t board_lot_quantity;
  int32_t tick_size;
  int16_t maximum_gtc_days;
  int16_t st_stock_eligibility_indicators;
  int16_t disclosed_quantity_percentage_allowed;
  int32_t risk_free_interest_rate;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << " ProcessedSystemInformationResponse -> " << st_mkt_status.ToString() << " "
               << st_ex_mkt_status.ToString() << " " << st_pl_mkt_status.ToString() << " "
               << "update_portfolio : " << update_portfolio << " "
               << "market_index : " << market_index << " "
               << "default_settlement_period : " << default_settlement_period << " "
               << "default_settlement_period_auction : " << default_settlement_period_auction << " "
               << "competitor_period : " << competitor_period << " "
               << "solicitor_period : " << solicitor_period << " "
               << "warning_percent : " << warning_percent << " "
               << "volume_freeze_percent : " << volume_freeze_percent << " "
               << "snap_quote_time : " << snap_quote_time << " "
               << "board_lot_quantity : " << board_lot_quantity << " "
               << "tick_size : " << tick_size << " "
               << "maximum_gtc_days : " << maximum_gtc_days << " "
               << "st_stock_eligibility_indicators : " << st_stock_eligibility_indicators << " "
               << "disclosed_quantity_percentage_allowed : " << disclosed_quantity_percentage_allowed << " "
               << "risk_free_interest_rate : " << risk_free_interest_rate << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedUpdateLocalDatabaseResponse {};

struct st_sec_eligibilty_per_market {
  int8_t reserved_eligibilty;
  int16_t status;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "St_sec_eligibilty_per_market_struct -> "
               << "reserved: " << (reserved_eligibilty >> 1) << " "
               << "eligibility: " << (reserved_eligibilty & 1) << " "
               << "status: " << status << "\n";
    return t_temp_oss.str();
  };
};

struct ProcessedSeurityUpdateInfoResponse {
  int32_t token;
  int16_t permitted_to_trade;
  double issued_capital;
  int32_t warning_quantity;
  int32_t freeze_quantity;
  int16_t issue_rate;
  int32_t issue_start_date;
  int32_t margin_percentage;
  int32_t minimum_lot_quantity;
  int32_t board_lot_quantity;
  int32_t tick_size;
  char name[26];
  int8_t reserved;
  int32_t low_price_range;
  int32_t high_price_range;
  int32_t expiry_date;
  char asset_instrument[7];
  char asset_name[11];
  char asset_token[5];
  int32_t local_update_date_time;
  char delete_flag;
  int32_t base_price;

  // ST_SEC_ELIGIBILITY
  st_sec_eligibilty_per_market sec_eligibility_status[4];

  // ST_ELIGIBILITY_INDICATORS
  int8_t eligibility_info;
  int8_t eligibility_reserved;

  // ST_PURPOSE
  int16_t purpose;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedSeurityUpdateInfoResponse -> "
               << "token : " << token << " "
               << "permitted_to_trade : " << permitted_to_trade << " "
               << "issued_capital : " << issued_capital << " "
               << "warning_quantity : " << warning_quantity << " "
               << "freeze_quantity : " << freeze_quantity << " "
               << "issue_rate : " << issue_rate << " "
               << "issue_start_date : " << issue_start_date << " "
               << "margin_percentage : " << margin_percentage << " "
               << "minimum_lot_quantity : " << minimum_lot_quantity << " "
               << "board_lot_quantity : " << board_lot_quantity << " "
               << "tick_size : " << tick_size << " "
               << "name : " << name << " "
               << "reserved : " << reserved << " "
               << "low_price_range : " << low_price_range << " "
               << "high_price_range : " << high_price_range << " "
               << "expiry_date : " << expiry_date << " "
               << "asset_instrument : " << asset_instrument << " "
               << "asset_name : " << asset_name << " "
               << "asset_token : " << asset_token << " "
               << "local_update_date_time : " << local_update_date_time << " "
               << "delete_flag : " << delete_flag << " "
               << "base_price : " << base_price << " "
               << "sec_eligibility_status[0] : " << sec_eligibility_status[0].ToString() << " "
               << "sec_eligibility_status[1] : " << sec_eligibility_status[1].ToString() << " "
               << "sec_eligibility_status[2] : " << sec_eligibility_status[2].ToString() << " "
               << "sec_eligibility_status[3] : " << sec_eligibility_status[3].ToString() << " "
               << "eligibility_indicators-> "
               << "elig_reserved: " << (eligibility_info >> 3) << " "
               << "elig_minimum_fill: " << ((eligibility_info >> 2) & 1) << " "
               << "elig_AON: " << ((eligibility_info >> 1) & 1) << " "
               << "elig_reserved_1: " << eligibility_reserved << " "
               << "Purpose-> "
               << "PIAllowed: " << ((purpose >> 2) & 1) << "\n";

    return t_temp_oss.str();
  }
};

struct token_and_eligibility_st {
  int32_t token;
  int16_t status[4];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "token_and_eligibility-> "
               << "token: " << token << " "
               << "status[0]" << status[0] << " "
               << "status[1]" << status[1] << " "
               << "status[2]" << status[2] << " "
               << "status[3]" << status[3] << "\n";
    return t_temp_oss.str();
  }
};

struct ProcessedSecurityStatusUpdateInfo {
  int16_t number_of_records;
  token_and_eligibility_st token_and_eligibility[35];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedSeurityUpdateInfoResponse -> "
               << "number_of_records: " << number_of_records << " ";
    for (int i = 0; i < number_of_records; i++) {
      t_temp_oss << token_and_eligibility[i].ToString() << " ";
    }
    return t_temp_oss.str();
  }
};

struct ProcessedParticipantUpdateInfo {
  char participant_id[13];
  char participant_name[26];
  char participant_status;
  int32_t participant_update_date_time;
  char delete_flag;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedParticipantUpdateInfo -> "
               << "participant_id: " << participant_id << " "
               << "participant_name: " << participant_name << " "
               << "participant_status: " << participant_status << " "
               << "participant_update_date_time: " << participant_update_date_time << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedInstrumentUpdateInfo {
  int32_t instrument_id;
  int32_t instrument_update_date_time;
  char instrument_name[7];
  char instrument_description[26];
  char delete_flag;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedInstrumentUpdateInfo -> "
               << "instrument_id: " << instrument_id << " "
               << "instrument_name: " << instrument_name << " "
               << "instrument_description: " << instrument_description << " "
               << "instrument_update_date_time: " << instrument_update_date_time << " "
               << "delete_flag: " << delete_flag << "\n";

    return t_temp_oss.str();
  }
};

struct index_details {
  int32_t token;
  int32_t last_update_date_time;
  char index_name[16];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "index_details -> "
               << "index_name: " << index_name << " "
               << "token: " << token << " "
               << "last_update_date_time: " << last_update_date_time << "\n";
    return t_temp_oss.str();
  }
};

struct ProcessedDownloadIndex {
  int16_t number_of_records;
  index_details indexes[17];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedDownloadIndex -> "
               << "number_of_records: " << number_of_records << " ";
    for (int i = 0; i < number_of_records; i++) {
      t_temp_oss << indexes[i].ToString() << " ";
    }
    return t_temp_oss.str();
  }
};

struct bcast_index_map_details {
  char bcast_name[27];
  char changed_name[11];
  char delete_flag;
  int32_t last_update_date_time;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "bcast_index_map_details -> "
               << "bcast_name: " << bcast_name << " "
               << "changed_name: " << changed_name << " "
               << "delete_flag: " << delete_flag << " "
               << "last_update_date_time: " << last_update_date_time << "\n";
    return t_temp_oss.str();
  }
};

struct ProcessedDownloadIndexMap {
  int16_t number_of_records;
  bcast_index_map_details indexes[10];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedDownloadIndexMap -> "
               << "number_of_records: " << number_of_records << " ";
    for (int i = 0; i < number_of_records; i++) {
      t_temp_oss << indexes[i].ToString() << " ";
    }
    return t_temp_oss.str();
  }
};

struct ProcessedOrderResponse {
  int32_t entry_date_time;
  int32_t last_modified_date_time;
  int32_t size;
  int32_t disclosed_size;
  int64_t order_number;
  int64_t last_activity_reference;
  int32_t price;
  int16_t order_terms;
  char instrument_name[7];
  char symbol[11];
  char series[3];
  int32_t expiry_date;
  int32_t strike_price;
  char option_type[3];
  int32_t saos;
  int16_t reason_code;
  int16_t error_code;
  char mod_cxl_by;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedOrderResponse-> "
               << "order_number: " << order_number << " "
               << "instrument_name: " << instrument_name << " "
	       << "last_activity_reference : " << last_activity_reference << " "
               << "symbol: " << symbol << " "
               << "series: " << series << " "
               << "size: " << size << " "
               << "disclosed_size: " << disclosed_size << " "
               << "price: " << price << " "
               << "expiry_date: " << expiry_date << " "
               << "option_type: " << option_type << " "
               << "strike_price: " << strike_price << " "
               << "entry_date_time: " << entry_date_time << " "
               << "last_modified_date_time: " << last_modified_date_time << " "
               << "saos: " << saos << " "
               << "reason_code: " << reason_code << " "
               << "error code: " << error_code << " "
               << "order_attributes: " << order_terms << " "
               << "mod_cxl_by: " << (int32_t)mod_cxl_by << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedTradeConfirmationResponse {
  char instrument_name[7];
  char symbol[11];
  char series[3];
  int32_t token;
  int32_t size_executed;
  int32_t size_remaining;
  int64_t order_number;
  int64_t last_activity_reference;
  int32_t price;
  int32_t expiry_date;
  int32_t strike_price;
  int16_t buy_sell;
  int32_t activity_time;
  char option_type[3];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedTradeConfirmationResponse-> "
               << "order_number: " << order_number << " "
               << "token: " << token << "\n"
               << "instrument_name: " << instrument_name << " "
               << "symbol: " << symbol << " "
               << "series: " << series << " "
               << "size_executed: " << size_executed << " "
               << "size_remaining: " << size_remaining << " "
               << "price: " << price << " "
               << "buy_sell: " << buy_sell << " "
               << "expiry_date: " << expiry_date << " "
               << "option_type: " << option_type << " "
               << "activity_time: " << activity_time << " "
               << "last_activity_reference : " << last_activity_reference << " "
               << "strike_price: " << strike_price << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedOrderResponseNonTR {
  int32_t saos;
  int16_t error_code;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedOrderResponseNonTR-> "
               << "saos: " << saos << " error_code " << error_code << "\n";

    return t_temp_oss.str();
  }
};

struct ProcessedSpreadOrderResponse {
  int32_t entry_date_time;
  int32_t last_modified_date_time;
  int32_t size;
  int64_t order_number;
  int32_t price;
  int32_t saos;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ProcessedSpreadOrderResponse-> "
               << "order_number: " << order_number << " "
               << "size: " << size << " "
               << "price: " << price << " "
               << "entry_date_time: " << entry_date_time << " "
               << "last_modified_date_time: " << last_modified_date_time << " "
               << "saos: " << saos << "\n";

    return t_temp_oss.str();
  }
};

#pragma pack(push, 1)
struct InstrumentDesc {
  int32_t token_;
  char instrument_type_[6];
  char symbol_[10];
  int32_t expiry_date_;
  int32_t strike_price_;
  char option_type_[2];  // CM uses this as series info in structs

  InstrumentDesc() {
    token_ = 0;
    memset(instrument_type_, ' ', 6);
    memset(symbol_, ' ', 10);
    expiry_date_ = 0;
    strike_price_ = 0;
    memset(option_type_, ' ', 2);
  }

  std::string ToString() {
    std::stringstream ss;
    ss << " Token: " << ntoh32(token_);
    ss << " InstrumentType: ";
    for (int i = 0; i < 6; i++) {
      ss << instrument_type_[i];
    }
    ss << " Symbol: ";
    for (int i = 0; i < 10; i++) {
      ss << symbol_[i];
    }
    ss << " ExpiryDate: " << ntoh32(expiry_date_);
    ss << " StrikePrice: " << (int)ntoh32(strike_price_);
    ss << " OptionType: ";
    for (int i = 0; i < 2; i++) {
      ss << option_type_[i];
    }
    std::string str = ss.str();
    return str;
  }

  inline void SetInstrumentDesc(int32_t token, char const *instrument_type, char const *symbol, int32_t const &expiry,
                                int32_t const &strike, char const *option_type) {
    token_ = hton32(token);
    strncpy(instrument_type_, instrument_type, 6);
    strncpy(symbol_, symbol, 10);
    expiry_date_ = hton32(expiry);
    strike_price_ = hton32(strike);
    strncpy(option_type_, option_type, 2);
  }

  int GetToken() { return ntoh32(token_); }

  int GetTwiddledToken() { return token_; }
  char const *GetInstrumentDescAsBuffer() const { return (char const *)&token_; }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SpreadInstrumentDesc {
  InstrumentDesc id1;
  InstrumentDesc id2;
  inline void SetSpreadInstrumentDesc(int32_t token1, char const *instrument_type1, char const *symbol1,
                                      int32_t const &expiry1, int32_t const &strike1, char const *option_type1,
                                      int32_t token2, char const *instrument_type2, char const *symbol2,
                                      int32_t const &expiry2, int32_t const &strike2, char const *option_type2) {
    id1.SetInstrumentDesc(token1, instrument_type1, symbol1, expiry1, strike1, option_type1);
    id2.SetInstrumentDesc(token2, instrument_type2, symbol2, expiry2, strike2, option_type2);
  }
  int GetToken1() { return ntoh32(id1.token_); }
  int GetToken2() { return ntoh32(id2.token_); }
  char const *GetSpreadInstrumentDescAsBuffer1() const { return (char const *)&id1.token_; }
  char const *GetSpreadInstrumentDescAsBuffer2() const { return (char const *)&id2.token_; }
  std::string ToString() {
    std::stringstream ss;
    ss << " ID1:: " << id1.ToString() << "\n";
    ss << " ID2:: " << id2.ToString() << "\n";
    std::string str = ss.str();
    return str;
  }
};
#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////
// 					Cash Market Defines 							   //
/////////////////////////////////////////////////////////////////////////

struct SecurityInfoCashMarket {
  char symbol_[10];
  char series_[2];

  SecurityInfoCashMarket() {
    memset(symbol_, ' ', 10);
    memset(series_, ' ', 2);
  }

  std::string ToString() {
    std::stringstream ss;

    ss << "Symbol:";
    for (int i = 0; i < 10; i++) {
      ss << symbol_[i];
    }
    ss << ",";

    ss << "Series:";
    for (int i = 0; i < 2; i++) {
      ss << series_[i];
    }
    std::string str = ss.str();
    return str;
  }

  inline void SetSecurityInfo(char const *symbol, char const *series) {
    strncpy(symbol_, symbol, 10);
    strncpy(series_, series, 2);
  }

  char const *GetSecurityInfoCashMarketAsBuffer() const { return (char const *)&symbol_; }
};
}
}
