// =====================================================================================
//
//       Filename:  BMFEPOptimizedFixReplyProcessing.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/16/2014 11:19:16 AM
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
#include <unistd.h>
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define UINT32CASTVALUE *(uint32_t*)

#define DELIM (char)1

#define SKIP_OVER_TAG_8_WITH_VALUE_LENGTH 9      // 8=FIX.4.4
#define SKIP_OVER_TAG_9_WITHOUT_VALUE_LENGTH 2   // 9=
#define SKIP_OVER_TAG_35_WITH_VALUE_LENGTH 4     // 35=8
#define SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH 3  // 34=
#define SKIP_OVER_TAG_11_WIHTOUT_VALUE_LENGTH 3  // 11=
#define SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56_1_6 50
#define SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56 40
#define SKIP_OVER_SAFE_COMBINED_TAGS_39_40_44_48_54_55_59_60_75_150_151 50
#define SKIP_OVER_COMBINED_TAGS_14_17 15
#define SKIP_OVER_TAG_31_WITHOUT_VALUE_LENGTH 3
#define SKIP_OVER_COMBINED_TAGS_37_38_39_40_44_48_54_55_59_60_75_150 60
#define SKIP_OVER_SAFE_COMBINED_TAGS_14_17_37_38 30
#define SKIP_OVER_TAG_151_WITHOUT_VALUE_LENGTH 4
#define SKIP_OVER_TAG_112_WIHTOUT_VALUE_LENGTH 4
#define SKIP_OVER_TAG_198_WITHOUT_VALUE_LENGTH 4
#define SKIP_OVER_TAG_32_WITHOUT_VALUE_LENGTH 3
#define LENGTH_OF_DELIM 1  // '\001'

#define TAG_35_EXECUTIONREPORT_INT_VALUE 0x383D3533
#define TAG_11_INT_VALUE 0x3D313101
#define TAG_39_INT_VALUE 0x3D393301
#define TAG_39_NEW_ORDER_INT_VALUE 0x303D3933
#define TAG_39_PARTIAL_FILL_INT_VALUE 0x313D3933
#define TAG_39_FILL_INT_VALUE 0x323D3933
#define TAG_39_CANCEL_INT_VALUE 0x343D3933
#define TAG_39_EXPIRE_INT_VALUE 0x433D3933
#define TAG_39_REJECTED_INT_VALUE 0x383D3933
#define TAG_198_INT_VALUE 0x3D383931
#define TAG_31_INT_VALUE 0x3D313301
#define TAG_151_INT_VALUE 0x3D313531
#define TAG_112_INT_VALUE 0x3D323131
#define TAG_35_LOGON_RESPONSE_INT_VALUE 0x413D3533
#define TAG_35_HEARTBEAT_INT_VALUE 0x303D3533
#define TAG_35_TEST_REQUEST_INT_VALUE 0x313D3533
#define TAG_35_RESEND_REQUEST_INT_VALUE 0x323D3533
#define TAG_35_REJECT_INT_VALUE 0x333D3533
#define TAG_35_SEQUENCE_REST_INT_VALUE 0x343D3533
#define TAG_35_LOGOUT_INT_VALUE 0x353D3533
#define TAG_35_ORDER_CANCEL_REJECT_INT_VALUE 0x393D3533
#define TAG_35_BUSINESS_MESSAGE_REJECT_INT_VALUE 0x6A3D3533
#define TRAILER_TAG_INT_VALUE 0x3D303101

#define LENGTH_OF_TRAILER_TAG 8
#define MAX_DECIMAL_PLACES_SUPPORTED 8
#define MINIMUM_APPROXIMATE_LENGTH_OF_THE_TWO_OR_MORE_EXECUTION_REPORT 512

const double floating_price_decimal_lookup_[] = {1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001, 0.00000001};

namespace HFSAT {
namespace Utils {

class BMFEPOptimizedFixReplyProcessing {
 private:
  char* current_processing_ptr_;
  char* current_tag_11_sequence_number_ptr_;

  int32_t last_processed_exchange_reply_sequence_number_;
  int32_t current_server_assigned_sequnece_in_processing_;
  uint64_t current_exchange_sequence_number_;

  double current_execution_price_;
  int32_t current_execution_price_factor_;
  uint32_t current_executed_qty_;
  uint32_t current_remaining_qty_;
  HFSAT::CpucycleProfiler& cpucycle_profiler_;

 public:
  BMFEPOptimizedFixReplyProcessing(HFSAT::CpucycleProfiler& _cpucycle_profiler_)
      : current_processing_ptr_(NULL),
        current_tag_11_sequence_number_ptr_(NULL),
        last_processed_exchange_reply_sequence_number_(-1),
        current_server_assigned_sequnece_in_processing_(-1),
        current_exchange_sequence_number_(0),
        current_execution_price_(0.0),
        current_execution_price_factor_(0),
        current_executed_qty_(0),
        current_remaining_qty_(0),
        cpucycle_profiler_(_cpucycle_profiler_)

  {}

 private:
  inline void ProcessLogonResponse(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_) {
    //        std::cout << " Process Logon \n" ;

    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessHeartbeat(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                               bool& _optimized_processing_failed_) {
    //        std::cout << " Processing Heartbeats \n" ;

    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessResendRequest(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                            bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessOrderCancelReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessBusinessMessageReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                           bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessLogout(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                            bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessTestRequest(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                 bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }

    this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56;

    while (TAG_112_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    if (this_processing_ptr == _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      _optimized_processing_failed_ = true;

      return;
    }

    this_processing_ptr += SKIP_OVER_TAG_112_WIHTOUT_VALUE_LENGTH;

    char place_holder_buffer[32];
    (void)place_holder_buffer;

    uint32_t index = 0;

    for (index = 0; DELIM != *this_processing_ptr; this_processing_ptr++) {
      place_holder_buffer[index++] = *this_processing_ptr;
    }

    place_holder_buffer[index] = '\0';
  }

  inline void ProcessSequenceReset(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_) {}

  inline void ProcessOrderConfirmation(char* _start_of_tag_39_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_39_ptr_ + SKIP_OVER_SAFE_COMBINED_TAGS_39_40_44_48_54_55_59_60_75_150_151;

    while (TAG_198_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    this_processing_ptr += SKIP_OVER_TAG_198_WITHOUT_VALUE_LENGTH;
    current_exchange_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      current_exchange_sequence_number_ = current_exchange_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessPartialAndCompleteFill(char* _start_of_tag_11_ptr_, char* _single_message_end_ptr_,
                                            bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_11_ptr_ + SKIP_OVER_COMBINED_TAGS_14_17;

    while (TAG_31_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_31_WITHOUT_VALUE_LENGTH);

    current_execution_price_ = 0;

    for (; (DELIM != *this_processing_ptr) && ((char)'.' != *this_processing_ptr); this_processing_ptr++) {
      current_execution_price_ = current_execution_price_ * 10 + (*this_processing_ptr - '0');
    }

    if ('.' == *this_processing_ptr) {
      this_processing_ptr++;
      current_execution_price_factor_ = 0;

      for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
        current_execution_price_ = current_execution_price_ * 10 + (*this_processing_ptr - '0');
        current_execution_price_factor_++;
      }

      if (current_execution_price_factor_ < 0 || current_execution_price_factor_ > MAX_DECIMAL_PLACES_SUPPORTED) {
        _optimized_processing_failed_ = true;
        return;
      }

      current_execution_price_ *= floating_price_decimal_lookup_[current_execution_price_factor_];
    }

    this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_32_WITHOUT_VALUE_LENGTH);
    current_executed_qty_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      current_executed_qty_ = current_executed_qty_ * 10 + (*this_processing_ptr - '0');
    }

    this_processing_ptr += SKIP_OVER_COMBINED_TAGS_37_38_39_40_44_48_54_55_59_60_75_150;

    while (TAG_151_INT_VALUE != UINT32CASTVALUE(this_processing_ptr)) this_processing_ptr++;

    this_processing_ptr += SKIP_OVER_TAG_151_WITHOUT_VALUE_LENGTH;
    current_remaining_qty_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      current_remaining_qty_ = current_remaining_qty_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessOrderCancellation(char* _start_of_tag_39_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_) {}

  inline void ProcessCancelReplacement(char* _start_of_tag_39_ptr_, char* _single_message_end_ptr_,
                                       bool& _optimized_processing_failed_) {
    char* this_processing_ptr = _start_of_tag_39_ptr_;

    this_processing_ptr += 14;

    while (0x3D343401 != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    if (this_processing_ptr == _single_message_end_ptr_) {
      _optimized_processing_failed_ = true;
      return;
    }

    this_processing_ptr += (LENGTH_OF_DELIM + 3);

    double order_replacement_price_ = 0;
    uint32_t order_replacement_price_factor_ = 0;

    for (; (DELIM != *this_processing_ptr) && ((char)'.' != *this_processing_ptr); this_processing_ptr++) {
      order_replacement_price_ = order_replacement_price_ * 10 + (*this_processing_ptr - '0');
    }

    if ('.' == *this_processing_ptr) {
      this_processing_ptr++;
      order_replacement_price_factor_ = 0;

      for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
        order_replacement_price_ = order_replacement_price_ * 10 + (*this_processing_ptr - '0');
        order_replacement_price_factor_++;
      }

      if (order_replacement_price_factor_ < 0 || order_replacement_price_factor_ > MAX_DECIMAL_PLACES_SUPPORTED) {
        _optimized_processing_failed_ = true;
        return;
      }

      order_replacement_price_ *= floating_price_decimal_lookup_[order_replacement_price_factor_];
    }

    this_processing_ptr += (LENGTH_OF_DELIM + 60);

    while (TAG_151_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    if (this_processing_ptr == _single_message_end_ptr_) {
      _optimized_processing_failed_ = true;
      return;
    }

    this_processing_ptr += (SKIP_OVER_TAG_151_WITHOUT_VALUE_LENGTH);

    current_remaining_qty_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      current_remaining_qty_ = current_remaining_qty_ * 10 + (*this_processing_ptr - '0');
    }

    this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_198_WITHOUT_VALUE_LENGTH);
    current_exchange_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      current_exchange_sequence_number_ = current_exchange_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }
  }

  inline void ProcessExchangeReject(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                    bool& _optimized_processing_failed_) {}

  inline void ProcessExecutionReport(char* _start_of_tag_35_ptr_, char* _single_message_end_ptr_,
                                     bool& _optimized_processing_failed_) {
    //        std::cout << " ProcessExecutionReport called \n" ;
    //        std::cout.flush () ;

    // Skipping Over
    // -> TAG 35 ( 35=8 )
    // -> TAG 34 ( 34= )
    char* this_processing_ptr = _start_of_tag_35_ptr_ + SKIP_OVER_TAG_35_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_34_WITHOUT_VALUE_LENGTH;

    // NOW We are at tag 34 value
    last_processed_exchange_reply_sequence_number_ = 0;

    for (; DELIM != *this_processing_ptr; this_processing_ptr++) {
      last_processed_exchange_reply_sequence_number_ =
          last_processed_exchange_reply_sequence_number_ * 10 + (*this_processing_ptr - '0');
    }

    this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_49_52_56_1_6;

    while (TAG_11_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    if (this_processing_ptr == _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      _optimized_processing_failed_ = true;

      return;
    }

    current_tag_11_sequence_number_ptr_ = this_processing_ptr;
    // Should Reach Here only if we have matched tag 11 [ (char)1 11= ]
    this_processing_ptr += (LENGTH_OF_DELIM + SKIP_OVER_TAG_11_WIHTOUT_VALUE_LENGTH);
    current_server_assigned_sequnece_in_processing_ = 0;

    for (; DELIM != *this_processing_ptr && this_processing_ptr < _single_message_end_ptr_; this_processing_ptr++) {
      current_server_assigned_sequnece_in_processing_ =
          current_server_assigned_sequnece_in_processing_ * 10 + (*this_processing_ptr - '0');
    }

    this_processing_ptr += DELIM;
    this_processing_ptr += SKIP_OVER_SAFE_COMBINED_TAGS_14_17_37_38;

    while (TAG_39_INT_VALUE != UINT32CASTVALUE(this_processing_ptr) && this_processing_ptr < _single_message_end_ptr_)
      this_processing_ptr++;

    if (this_processing_ptr == _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      _optimized_processing_failed_ = true;

      return;
    }

    this_processing_ptr += DELIM;

    switch (UINT32CASTVALUE(this_processing_ptr)) {
      case TAG_39_NEW_ORDER_INT_VALUE: {
        ProcessOrderConfirmation(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_39_PARTIAL_FILL_INT_VALUE:
      case TAG_39_FILL_INT_VALUE: {
        ProcessPartialAndCompleteFill(current_tag_11_sequence_number_ptr_, _single_message_end_ptr_,
                                      _optimized_processing_failed_);

      } break;

      case TAG_39_CANCEL_INT_VALUE:
      case TAG_39_EXPIRE_INT_VALUE: {
        ProcessOrderCancellation(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;
      case 0x353D3933: {
        ProcessCancelReplacement(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_39_REJECTED_INT_VALUE: {
        ProcessExchangeReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      default: {
        // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
        _optimized_processing_failed_ = true;

      } break;
    }
  }

  inline void ProcessSingleMessage(char* _single_message_start_ptr_, char* _single_message_end_ptr_,
                                   bool& _optimized_processing_failed_) {
    //        std::cout << " ProcessSingleMessage START WIH : " << _single_message_start_ptr_ << "\n" ;
    //        std::cout.flush () ;

    //        HFSAT::CpucycleProfiler::GetUniqueInstance ().Start ( 3 ) ;

    // Skipping Over
    // -> TAG 8 ( 8=FIX.4.4 )
    // -> TAG 9 ( 9= )
    char* this_processing_ptr = _single_message_start_ptr_ + SKIP_OVER_TAG_8_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
                                SKIP_OVER_TAG_9_WITHOUT_VALUE_LENGTH;

    //        std::cout << " NOW AT : " << this_processing_ptr << "\n" ;

    while (DELIM != *this_processing_ptr && this_processing_ptr < _single_message_end_ptr_) this_processing_ptr++;

    if (this_processing_ptr == _single_message_end_ptr_) {
      // Notify That The Optimized Procesing Has Failed, The caller can now switch back to older processing
      _optimized_processing_failed_ = true;

      return;
    }

    this_processing_ptr += LENGTH_OF_DELIM;

    //        std::cout << " MESSAGE TYPE : " << this_processing_ptr << "\n" ;

    //        std::cout << " NOW POINTER AT : " << this_processing_ptr << "\n" ;
    //        std::cout.flush () ;

    switch (UINT32CASTVALUE(this_processing_ptr)) {
      case TAG_35_LOGON_RESPONSE_INT_VALUE: {
        ProcessLogonResponse(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_HEARTBEAT_INT_VALUE: {
        ProcessHeartbeat(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_TEST_REQUEST_INT_VALUE: {
        ProcessTestRequest(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_RESEND_REQUEST_INT_VALUE: {
        ProcessResendRequest(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);
      } break;

      case TAG_35_REJECT_INT_VALUE: {
        ProcessReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_SEQUENCE_REST_INT_VALUE: {
        ProcessSequenceReset(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_ORDER_CANCEL_REJECT_INT_VALUE: {
        ProcessOrderCancelReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_BUSINESS_MESSAGE_REJECT_INT_VALUE: {
        ProcessBusinessMessageReject(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_LOGOUT_INT_VALUE: {
        ProcessLogout(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      case TAG_35_EXECUTIONREPORT_INT_VALUE: {
        ProcessExecutionReport(this_processing_ptr, _single_message_end_ptr_, _optimized_processing_failed_);

      } break;

      default: { _optimized_processing_failed_ = true; } break;
    }

    //        HFSAT::CpucycleProfiler::GetUniqueInstance ().End ( 3 ) ;
  }

  inline void ProcessExchangeReply(char* _exchange_reply_, char* _end_of_complete_message_ptr_,
                                   const int32_t& _msg_length_, bool& _optimized_processing_failed_) {
    //        std::cout << " Process Exchange Reply With Length : " << _msg_length_ << "\n" ;
    //        std::cout.flush () ;

    if (MINIMUM_APPROXIMATE_LENGTH_OF_THE_TWO_OR_MORE_EXECUTION_REPORT > _msg_length_) {
      ProcessSingleMessage(_exchange_reply_, _end_of_complete_message_ptr_, _optimized_processing_failed_);

    } else {
      for (char *this_processing_ptr = _exchange_reply_ + 0,
                *next_processing_ptr = strstr(this_processing_ptr, "\00110=");
           next_processing_ptr != NULL && this_processing_ptr < _end_of_complete_message_ptr_;
           this_processing_ptr = next_processing_ptr + LENGTH_OF_TRAILER_TAG,
                next_processing_ptr = strstr(this_processing_ptr, "\00110=")) {
        //            std::cout << " Calling ProcessSingleMessage With : " << this_processing_ptr << "\n" ;
        //            std::cout.flush () ;
        ProcessSingleMessage(this_processing_ptr, next_processing_ptr + LENGTH_OF_TRAILER_TAG,
                             _optimized_processing_failed_);
      }
    }

    //        HFSAT::CpucycleProfiler::GetUniqueInstance ().Start ( 2 ) ;
    //
    //        char * this_processing_ptr = _exchange_reply_ + SKIP_OVER_TAG_8_WITH_VALUE_LENGTH + LENGTH_OF_DELIM +
    //        SKIP_OVER_TAG_9_WITHOUT_VALUE_LENGTH ;
    //        char * next_processing_ptr = NULL ;
    //
    //        while ( DELIM != *this_processing_ptr && this_processing_ptr < _end_of_complete_message_ptr_ )
    //        this_processing_ptr ++ ;
    //        this_processing_ptr += LENGTH_OF_DELIM ;
    //
    //        HFSAT::CpucycleProfiler::GetUniqueInstance ().End ( 2 ) ;
    //
    //        if ( TAG_35_EXECUTIONREPORT_INT_VALUE == UINT32CASTVALUE ( this_processing_ptr ) &&
    //        MINIMUM_APPROXIMATE_LENGTH_OF_THE_TWO_OR_MORE_EXECUTION_REPORT > _msg_length_ ) {
    //
    //          ProcessSingleMessage ( _exchange_reply_, strstr ( _exchange_reply_, "\00110=" ) + 8,
    //          _optimized_processing_failed_ ) ;
    //
    //        }else {

    //        }

    //        char * this_processing_ptr = _exchange_reply_ + 0 ;
    //        char * next_processing_ptr = _exchange_reply_ + 0 ;
    //
    //        while ( this_processing_ptr <= ( _end_of_complete_message_ptr_ - LENGTH_OF_TRAILER_TAG ) ) {
    //
    //          if ( TRAILER_TAG_INT_VALUE == UINT32CASTVALUE ( this_processing_ptr ) ) {
    //
    //            //Send a single message for processing
    //            ProcessSingleMessage ( next_processing_ptr, this_processing_ptr + LENGTH_OF_TRAILER_TAG,
    //            _optimized_processing_failed_ ) ;
    //            //Move to next message
    //            next_processing_ptr = this_processing_ptr + LENGTH_OF_TRAILER_TAG ;
    //
    //            //Since We know it's going to be trailer tag now
    //            this_processing_ptr += LENGTH_OF_TRAILER_TAG ;
    //
    //          }
    //
    //          this_processing_ptr ++ ;
    //
    //        }
  }

 public:
  inline int32_t DecodeBMFMessage(char* _msg_char_buf_, const int32_t& _msg_length_,
                                  bool& _optimized_processing_failed_) {
    std::cout << " DECODEBMF " << _msg_length_ << " >> " << _msg_char_buf_ << "\n";
    std::cout.flush();

    //        std::cout << " Calling decode with : " << _msg_char_buf_ << " Length : " << _msg_length_ << "\n" ;

    //        HFSAT::CpucycleProfiler::GetUniqueInstance ().Start ( 1 ) ;

    // Set The Processing Pointer
    current_processing_ptr_ = _msg_char_buf_ + 0;

    // Pass The String As A Complete Char Arrary With Null Pad To Operate Over it, We are not modifying input here, we
    // are only marking end of the char array
    _msg_char_buf_[_msg_length_] = '\0';

    // First Check Whether We Have The Complete Message Or the Partial One, If the Last 8 chars are corrosponding to
    // trailer tag then we have the complete message,
    // However a complete message can still have n number of messages
    if (TRAILER_TAG_INT_VALUE == UINT32CASTVALUE(current_processing_ptr_ + _msg_length_ -
                                                 LENGTH_OF_TRAILER_TAG)) {  // This means a complete message

      //          std::cout << " ProcessExchange Reply Complete Message \n" ;
      // Pass A complete message for processing, mark the start and end pointer for it to identify where to start and
      // stop processing
      ProcessExchangeReply(_msg_char_buf_, _msg_char_buf_ + _msg_length_, _msg_length_, _optimized_processing_failed_);

      //          HFSAT::CpucycleProfiler::GetUniqueInstance ().End ( 1 ) ;

      // No Partial Leftover part
      return 0;
    }

    std::cout << "PROCESSING A PARTIAL MESSAGE \n";
    std::cout.flush();

    // Now We can have a complete message and some additional partial message part, or just a partial message
    // Check Whether we have any complete message to process in the entire buffer, starting from back end would be
    // optimized

    // Starting From 1 less than the point which is already tested above
    current_processing_ptr_ = _msg_char_buf_ + _msg_length_ - LENGTH_OF_TRAILER_TAG - 1;
    int32_t marking_end_point_of_complete_message = _msg_length_ - LENGTH_OF_TRAILER_TAG - 1;

    //        std::cout << " START AT : " << current_processing_ptr_ << "\n" ;
    //        std::cout.flush () ;

    // Until We hit the start from tail
    while (marking_end_point_of_complete_message >= 0) {
      //          std::cout << " NOW AT : " << current_processing_ptr_ << " STARTING VALUE : " << UINT32CASTVALUE (
      //          current_processing_ptr_ ) << " TRAILER TAG : " << TRAILER_TAG_INT_VALUE << "\n" ;
      //          std::cout.flush () ;

      // Check For Trailer tag uint value against each iteration
      if (TRAILER_TAG_INT_VALUE == UINT32CASTVALUE(current_processing_ptr_)) {
        //            std::cout << " Processing Exchange reply with length : " << _msg_length_ << "\n" ;
        //            std::cout.flush () ;

        std::cout << " PROCESSING A COMPLETE MESSAGE AS WELL \n";
        std::cout.flush();
        // Got the complete reply here
        ProcessExchangeReply(_msg_char_buf_, current_processing_ptr_ + LENGTH_OF_TRAILER_TAG, _msg_length_,
                             _optimized_processing_failed_);

        //            std::cout << " Partial Message Length : " << current_processing_ptr_ + LENGTH_OF_TRAILER_TAG << "
        //            LeftOver : " << _msg_length_ - ( marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG )
        //            << "\n" ;
        //            std::cout.flush () ;

        //        HFSAT::CpucycleProfiler::GetUniqueInstance ().Start ( 1 ) ;
        // Copy The left over part and return the offset for the next read to begin
        memmove(_msg_char_buf_, current_processing_ptr_ + LENGTH_OF_TRAILER_TAG,
                _msg_length_ - (marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG));
        //        HFSAT::CpucycleProfiler::GetUniqueInstance ().End ( 1 ) ;
        //            std::cout << " Buffer Remained : " << _msg_char_buf_ << " With Read Offset : " << _msg_length_ - (
        //            marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG )  << "\n" ;

        std::cout << " WE WILL BE RETURNING : "
                  << _msg_length_ - (marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG) << "\n";
        std::cout.flush();

        return (_msg_length_ - (marking_end_point_of_complete_message + LENGTH_OF_TRAILER_TAG));
      }

      current_processing_ptr_--;
      marking_end_point_of_complete_message--;
    }

    // Don't have any complete message in the buffer, simply return the length of the current read
    if (*current_processing_ptr_ == _msg_char_buf_[0]) {
      //          std::cout << " Don't Have Any Complete Message In this \n" ;
      return _msg_length_;
    }

    //        HFSAT::CpucycleProfiler::GetUniqueInstance ().End ( 1 ) ;

    // Return an invalid value
    return -1;
  }
};
}
}
