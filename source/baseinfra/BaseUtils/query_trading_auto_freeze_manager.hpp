// =====================================================================================
//
//       Filename:  query_trading_auto_freeze_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/08/2016 08:18:54 AM
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

#include <unistd.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define QUERY_TRADING_AUTO_FREEZE_SETTINGS_DIR "/home/pengine/prod/live_configs/"

namespace HFSAT {
namespace BaseUtils {

// Enum for freeze reasons
enum class FreezeEnforcedReason {
  kAllow = 0,
  kFreezeOnConsecutiveRejects,
  kFreezeOnTimeBasedRejects,
  kFreezeOnTotalRejects,
  kFreezeOnORSSecNotFound,
  kFreezeOnNoResponseFromORS,
  kFreezeOnRiskCheckHit,
  kFreezeThrottleBasedRejects,
  kFreezeOnORSMaxPos,
  kFreezeOnORSWorstPos,
  kFreezeOnMarginBreach
};

/**
 * ToString function
 * @param freeze_reason
 * @return
 */
inline const char* FreezeEnforcedReasonString(FreezeEnforcedReason const& freeze_reason) {
  switch (freeze_reason) {
    case FreezeEnforcedReason::kAllow:
      return "kAllow";
    case FreezeEnforcedReason::kFreezeOnConsecutiveRejects:
      return "kFreezeOnConsecutiveRejects";
    case FreezeEnforcedReason::kFreezeOnTimeBasedRejects:
      return "kFreezeOnTimeBasedRejects";
    case FreezeEnforcedReason::kFreezeOnTotalRejects:
      return "kFreezeOnTotalRejects";
    case FreezeEnforcedReason::kFreezeOnORSSecNotFound:
      return "kFreezeOnORSSecNotFound";
    case FreezeEnforcedReason::kFreezeOnNoResponseFromORS:
      return "kFreezeOnNoResponseFromORS";
    case FreezeEnforcedReason::kFreezeOnRiskCheckHit:
      return "kFreezeOnRiskCheckHit";
    case FreezeEnforcedReason::kFreezeThrottleBasedRejects:
      return "kFreezeThrottleBasedRejects";
    case FreezeEnforcedReason::kFreezeOnORSMaxPos:
      return "kFreezeOnORSMaxPos";
    case FreezeEnforcedReason::kFreezeOnORSWorstPos:
      return "kFreezeOnORSWorstPos";
    case FreezeEnforcedReason::kFreezeOnMarginBreach:
      return "kFreezeOnMarginBreach";
    default:
      return "kInvalid";
  }
}

class QueryTradingAutoFreezeManager {
 private:
  /// Number of rejects from exchange on which we should freeze
  int32_t getfreeze_consecutive_exchange_Rejects_treshold_;

  /// Overall moving window in which if we receive rejects consecutively we freeze
  int32_t getfreeze_time_based_rejects_duration_;

  /// Number of rejects threshold in given time above
  int32_t getfreeze_time_based_rejects_threshold_;

  /// Total number of rejects from exchange to freeze, not necessarilty consecutive
  int32_t getfreeze_total_number_of_exchange_rejects_threshold_;

  /// Expire the timer and resume trading time
  int32_t reject_based_getfreeze_expire_timer_in_msecs_;

  /// Duration in which if we don't receive ors-reply then freeze
  int32_t getfreeze_ors_noreply_duration_;

  /// Number of throttle threshold in given time
  int32_t getfreeze_time_based_throttle_threshold_;

  QueryTradingAutoFreezeManager()
      : getfreeze_consecutive_exchange_Rejects_treshold_(15),
        getfreeze_time_based_rejects_duration_(600000),  // 10 mins
        getfreeze_time_based_rejects_threshold_(30),
        getfreeze_total_number_of_exchange_rejects_threshold_(90),
        reject_based_getfreeze_expire_timer_in_msecs_(900000),  // 15 mins to expire
        getfreeze_ors_noreply_duration_(1000),
        getfreeze_time_based_throttle_threshold_(99000) {
    LoadReloadAndUpdateFreezeSettings();
  }

  QueryTradingAutoFreezeManager(QueryTradingAutoFreezeManager const& disabled_copy_constructor) = delete;

 public:
  /**
   * Read the config-file populate variable
   */
  void LoadReloadAndUpdateFreezeSettings() {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    std::ostringstream t_temp_oss;
    t_temp_oss << QUERY_TRADING_AUTO_FREEZE_SETTINGS_DIR << hostname << "_query_auto_freeze_settings.cfg";

    std::ifstream query_freeze_settings_stream;
    query_freeze_settings_stream.open(t_temp_oss.str().c_str(), std::ios::in);

    if (!query_freeze_settings_stream.is_open()) {
      return;
    }

#define MAX_LINE_BUFFER_SIZE 1024

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    memset((void*)line_buffer, 0, MAX_LINE_BUFFER_SIZE);

    while (query_freeze_settings_stream.good()) {
      query_freeze_settings_stream.getline(line_buffer, MAX_LINE_BUFFER_SIZE);
      std::string line_buffer_string = line_buffer;
      if (std::string::npos != line_buffer_string.find("#")) continue;

      HFSAT::PerishableStringTokenizer pst(line_buffer, MAX_LINE_BUFFER_SIZE);
      std::vector<const char*> const& tokens = pst.GetTokens();
      if (tokens.size() <= 5) continue;

      getfreeze_consecutive_exchange_Rejects_treshold_ =
          std::max(getfreeze_consecutive_exchange_Rejects_treshold_, atoi(tokens[0]));
      getfreeze_time_based_rejects_duration_ = std::max(getfreeze_time_based_rejects_duration_, atoi(tokens[1]));
      getfreeze_time_based_rejects_threshold_ = std::max(getfreeze_time_based_rejects_threshold_, atoi(tokens[2]));
      getfreeze_total_number_of_exchange_rejects_threshold_ =
          std::max(getfreeze_total_number_of_exchange_rejects_threshold_, atoi(tokens[3]));
      reject_based_getfreeze_expire_timer_in_msecs_ =
          std::max(60000, std::min(reject_based_getfreeze_expire_timer_in_msecs_, atoi(tokens[4])));

      if (tokens.size() > 5) {
        getfreeze_ors_noreply_duration_ = std::max(getfreeze_ors_noreply_duration_, atoi(tokens[5]));
      }

      if(tokens.size() > 6){
        getfreeze_time_based_throttle_threshold_ = std::max(getfreeze_time_based_throttle_threshold_, atoi(tokens[6]));
      }

      std::cout << "GETFREEZE_CONSECUTIVE_EXCHANGE_REJECTS_THRESHOLD : "
                << getfreeze_consecutive_exchange_Rejects_treshold_
                << " GETFREEZE_TIME_BASED_REJECTS_DURATION : " << getfreeze_time_based_rejects_duration_
                << " getfreeze_time_based_rejects_threshold_ : " << getfreeze_time_based_rejects_threshold_
                << " GETFREEZE_TOTAL_NUMBER_OF_EXCHANGE_REJECTS_THRESHOLD : "
                << getfreeze_total_number_of_exchange_rejects_threshold_
                << " REJECTBASED_GETFREEZE_EXPIRE_TIMER_IN_MSECS : " << reject_based_getfreeze_expire_timer_in_msecs_
                << " GETFREEZE_TIME_BASED_ORS_NOREPLY_THRESHOLD: " << getfreeze_ors_noreply_duration_ 
                << " GETFREEZE_TIME_BASED_THROTTLE_THRESHOLD : " << getfreeze_time_based_throttle_threshold_ << std::endl ;

      break;
    }

    query_freeze_settings_stream.close();

#undef MAX_LINE_BUFFER_SIZE
  }

  static QueryTradingAutoFreezeManager& GetUniqueInstance() {
    static QueryTradingAutoFreezeManager unique_instance;
    return unique_instance;
  }

  /**
   * Only in case of rejects, determine if we should freeze
   * @param no_of_consecutive_rejects_received
   * @param no_of_rejects_in_given_time
   * @param total_number_of_rejects_received
   * @return
   */
  FreezeEnforcedReason ShouldAllowOrders(int32_t const& no_of_consecutive_rejects_received,
                                         int32_t const& no_of_rejects_in_given_time,
                                         int32_t const& total_number_of_rejects_received,
                                         int32_t const& no_of_throttles_in_given_time = 0) {
    if (no_of_consecutive_rejects_received > getfreeze_consecutive_exchange_Rejects_treshold_) {
      return FreezeEnforcedReason::kFreezeOnConsecutiveRejects;
    } else if (no_of_rejects_in_given_time > getfreeze_time_based_rejects_threshold_) {
      return FreezeEnforcedReason::kFreezeOnTimeBasedRejects;
    } else if (total_number_of_rejects_received > getfreeze_total_number_of_exchange_rejects_threshold_) {
      return FreezeEnforcedReason::kFreezeOnTotalRejects;
    } else if (no_of_throttles_in_given_time > getfreeze_time_based_throttle_threshold_) {
      return FreezeEnforcedReason::kFreezeThrottleBasedRejects;
    }

    return FreezeEnforcedReason::kAllow;
  }

  /**
   * Getter
   * @return
   */
  int32_t GetTimeBasedRejectsDuration() { return getfreeze_time_based_rejects_duration_; }

  /**
   *Getter
   * @return
   */
  int32_t GetFreezeExpireTimer() { return reject_based_getfreeze_expire_timer_in_msecs_; }

  /**
   * Getter
   * @return
   */
  int32_t GetFreezeORSNoReplyDuration() { return getfreeze_ors_noreply_duration_; }
};
}
}
