// =====================================================================================
//
//       Filename:  shared_logging_defines.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/29/2014 03:09:09 PM
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

#define LOGGING_MANAGER_SHM_KEY ((key_t)0x100)        // key for shared memory
#define LOGGING_MANAGER_SEMAPHORE_KEY ((key_t)0x101)  // key for producer id generation

#define LOGGING_SEGMENTS_START_KEY 100001
#define LOGGING_SEGMENTS_QUEUE_SIZE 8192
#define MAX_LOG_BUFFER_SIZE 512

#define TEXT_BUFFER_SIZE 100

namespace HFSAT {
namespace CDef {

enum BufferContentType { ORSTrade = 1, QueryTrade, UnstructuredText };

struct UnstructuredTextStruct {
  char buffer[TEXT_BUFFER_SIZE + 1];

  std::string ToString() {
    std::ostringstream oss;
    oss << buffer;
    return oss.str();
  }
};

struct ORSTradeStruct {
  char symbol_[16];
  int32_t trade_type_;
  uint32_t size_executed_;
  double price_;
  int32_t saos_;
  int64_t exch_order_sequence_;
  int32_t saci_;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    char delim = (char)1;

    char print_price[20] = {'\0'};
    sprintf(print_price, "%.7f", price_);

    timeval time;
    gettimeofday(&time, NULL);

    t_temp_oss << symbol_ << delim << trade_type_ << delim << size_executed_ << delim << print_price << delim << saos_
               << delim << exch_order_sequence_ << delim << time.tv_sec << "." << std::setw(6) << std::setfill('0')
               << time.tv_usec << delim << saci_ << delim;

    return t_temp_oss.str();
  }
};

struct QueryTradeStruct {
  char security_name_[48];  // should be in sync with dvctrade/SmartOrderRouting/base_pnl.hpp::numbered_secname
  int32_t watch_tv_sec_;
  int32_t watch_tv_usec_;
  uint32_t trade_size_;
  double trade_price_;
  uint32_t new_position_;
  int32_t open_unrealized_pnl_;
  int32_t total_pnl_;
  uint32_t bestbid_size_;
  double bestbid_price_;
  double bestask_price_;
  uint32_t bestask_size_;
  double mult_risk_;
  int32_t mult_base_pnl_;
  double port_risk_;
  int32_t port_base_pnl_;
  char open_or_flat_;
  char trade_type_;
  char pad_[2];

  std::string ToString() {
    char buffer[1024];

    if ('O' == open_or_flat_) {
      sprintf(buffer, "%10d.%06d OPEN %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d", watch_tv_sec_,
              watch_tv_usec_, security_name_, trade_type_, trade_size_, trade_price_, new_position_,
              open_unrealized_pnl_, total_pnl_, bestbid_size_, bestbid_price_, bestask_price_, bestask_size_,
              mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);

      return std::string(buffer);

    } else if ('F' == open_or_flat_) {
      sprintf(buffer, "%10d.%06d FLAT %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d %.3f %d", watch_tv_sec_,
              watch_tv_usec_, security_name_, trade_type_, trade_size_, trade_price_, new_position_,
              open_unrealized_pnl_, total_pnl_, bestbid_size_, bestbid_price_, bestask_price_, bestask_size_,
              mult_risk_, mult_base_pnl_, port_risk_, port_base_pnl_);

      return std::string(buffer);

    } else {
      return "INVALID MESSAGE TYPE";
    }

    return "INVALID MESSAGE TYPE";
  }
};

struct ClientMetaData {
  int32_t client_pid_;
  int32_t assigned_client_id_;
  int32_t client_provided_logging_id_;
  int32_t assigned_shm_key_;
  char logging_directory_path_[512];
  char logfilename_[256];
};

struct ClientIdPid {
  int32_t client_pid_;
  int32_t assigned_client_id_;
};

struct LogBuffer {
  BufferContentType content_type_;

  union {
    QueryTradeStruct query_trade_;
    ORSTradeStruct ors_trade_;
    UnstructuredTextStruct text_data_;

  } buffer_data_;

  std::string ToString() {
    switch (content_type_) {
      case ORSTrade: {
        return buffer_data_.ors_trade_.ToString();

      } break;

      case QueryTrade: {
        return buffer_data_.query_trade_.ToString();

      } break;

      case UnstructuredText: {
        return buffer_data_.text_data_.ToString();
      } break;

      default: { } break; }

    return "UNEXPECTED MESSAGE TYPE";
  }
};
}
}
