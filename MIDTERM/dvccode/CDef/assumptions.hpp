// =====================================================================================
//
//       Filename:  assumptions.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  06/23/2015 12:14:12 PM
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
#ifndef ASSUMPTIONS_H
#define ASSUMPTIONS_H
#pragma once
#include <iostream>

#define DEF_MAX_SEC_ID 4096
#define ORS_MAX_NUM_OF_CLIENTS 4095
#define SACI_MANTISSA_HEX 0x0000FFFF  // Since we are multipying the base client id with 2^16
#define MAX_ORS_REPLY_DROP_RECOVERY_STORAGE 256
#define MAX_CXL_INITIATED_REQUESTS_TAGGING_SAOS_RANGE 0x40000

#define NSE_MAX_RECOVERY_PACKET_SIZE 8388608
#define MAX_RECOVERY_PACKET_SIZE 8388608
#define NSE_RECOVERY_HOST_IP "127.0.0.1"
#define NSE_RECOVERY_HOST_PORT 44011
#define NSE_MAX_FD_SUPPORTED 1024

#define NSE_EQ_CHANNELS_ADDRESS_PATTERN "239.60"
#define NSE_FO_CHANNELS_ADDRESS_PATTERN "239.70"
#define NSE_CD_CHANNELS_ADDRESS_PATTERN "239.80"
#define NSE_TER_CHANNELS_ADDRESS_PATTERN "239.55"

#define NSE_TER_FO_PORT 55021
#define NSE_TER_CD_PORT 55071

#define NSE_EQ_SEGMENT_MARKING 'E'
#define NSE_FO_SEGMENT_MARKING 'F'
#define NSE_CD_SEGMENT_MARKING 'C'
#define NSE_INVALID_SEGMENT 'I'

// NSE acceptable max params
#define NSE_MAX_ACCEPTABLE_POS 120
#define NSE_MAX_ACCEPTABLE_ORDER_SIZE 40

#define NSE_RECOVERY_HANDSHAKE_LENGTH 5

#define NSE_MIDTERM_ORDER_SERVER_IP "10.23.115.61"
#define NSE_MIDTERM_ORDER_SERVER_PORT 45021

#define NSE_ORDER_SEND_PORT 45005

#define NSE_MIDTERM_DATA_BUFFER_LENGTH 65536

#define NSE_MTERM_ORDER_ROUTING_SERVER_PORT 45321

// Hostwide control message to getflat
#define HOSTWIDE_GETFLAT_TRADER_ID -1111

#define RECOVERY_HOST_PORT 44011
#define MAX_RECOVERY_PACKET_SIZE 8388608
#define RECOVERY_HANDSHAKE_LENGTH 16  // Will be exchange symbol for most exchanges

// Invalid execution price (assuming that a trade will never happen at this price
#define INVALID_EXEC_PRICE -100000.0

#define OSE_INVALID_PRICE 2147483648

#define CONSOLE_ORDERS_START_SEQUENCE 90000000

#define RECOVERY_REQUEST_EXCHANGE_SRC_LENGTH 4
#define RECOVERY_REQUEST_DUMMY_FLAGS_LENGTH 16
#define RECOVERY_REQUEST_BODY_LENGTH 24
#define RECOVERY_REQUEST_MESSAGE_LENGTH \
  (RECOVERY_REQUEST_EXCHANGE_SRC_LENGTH + RECOVERY_REQUEST_DUMMY_FLAGS_LENGTH + RECOVERY_REQUEST_BODY_LENGTH)

// Retail IPs and Port

#define RETAIL_CO_LO_IP "10.230.63.12"
#define RETAIL_NY_IP "10.23.74.54"
#define RETAIL_SERVER_LISTENER_TO_QUERY_PORT 48000
#define RETAIL_FP_LISTENER_TO_QUERY_PORT 37000
#define RETAIL_FP_RECV_GUI_EXEC_PORT 37001
#define RETAIL_SERVER_AVBL_ORDER_NOTIFIER_PORT 49000
#define RETAIL_SERVER_ORDER_MANAGER_PORT 49001

// Default dummy values for order_price and order_size which are passed to the subroutines in case of ETIEngine.cpp file
#define ETI_DUMMY_ORDER_PRICE_VALUE -99999.0
#define ETI_DUMMY_ORDER_SIZE_VALUE -99999

// this is the date from which we have started using order feed version of TMX order book
#define DATE_OF_NEW_VERSION_OF_TMX_ORDER_BOOK 20171013

// this is the date from which we have started using order feed version of MICEX order book
#define DATE_OF_NEW_VERSION_OF_MICEX_ORDER_BOOK 20180129

#define PATH_TO_TAG_MAP_FILE "/spare/local/logs/tradelogs/"

// this is the date from which we have started using order feed version of RTS order book
#define DATE_OF_NEW_VERSION_OF_RTS_ORDER_BOOK 20171206
#define CME_MDS_CONTRACT_TEXT_SIZE 12

#endif
