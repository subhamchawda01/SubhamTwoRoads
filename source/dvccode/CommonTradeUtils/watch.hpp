/**
   \file dvccode/CommonTradeUtils/watch.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_COMMONTRADEUTILS_WATCH_H
#define BASE_COMMONTRADEUTILS_WATCH_H

#include <vector>
#include <string.h>

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/CommonTradeUtils/new_midnight_listener.hpp"

namespace HFSAT {

#define TWOMINUTESSECS 120
#define FIVEMINUTESSECS 300
#define TENMINUTESSECS 600
#define TWOMINUTESMSECS 120000
#define TENMINUTESMSECS 600000
#define WATCH_DEFAULT_PAGE_WIDTH_MSECS 100
#define WATCH_BIG_PAGE_WIDTH_MSECS 1000
#define WATCH_FIFTEEN_SECOND_PAGE_WIDTH_MSECS 15000
#define WATCH_THIRTY_SECOND_PAGE_WIDTH_MSECS 30000
#define WATCH_ONE_MINUTE_PAGE_WIDTH_MSECS 60000
#define WATCH_FIFTEEN_MINUTES_PAGE_WIDTH_MSECS 900000
#define WATCH_SMALL_PAGE_WIDTH_MSECS 1
#define MAXMSECINDAY 86400000
/// external callback to update time
#define CLOCK_PERIOD 1

/// Time Keeper listens to all input messages and keeps the most current time received.
/// This is probably faster than calling gettimeofday every time a message is to be processed but the effect should be
/// similar
///
/// Time Manager from input messages.
/// Also maintains midnight tv to calculate time in msecs_from_midnight_
class Watch : public ExternalTimeListener {
  typedef std::vector<NewMidnightListener*> NewMidnightListenerVec;
  typedef std::vector<TimePeriodListener*> TimePeriodListenerVec;

 protected:
  // external vars
  DebugLogger& dbglogger_;
  // internal
  ttime_t tv_;
  time_t last_midnight_sec_;
  int64_t usecs_from_midnight_;
  int msecs_from_midnight_;
  int YYYYMMDD_;

  mutable NewMidnightListenerVec
      new_midnight_listener_vec_;  // so that listeners can update this, haivng only const access
  mutable TimePeriodListenerVec time_period_listener_vec_;         ///< listen every 100 msecs
  mutable TimePeriodListenerVec big_time_period_listener_vec_;     ///< listen every 1 second
  mutable TimePeriodListenerVec fifteen_sec_period_listener_vec_;  ///< listen every 15 seconds
  mutable TimePeriodListenerVec thirty_sec_period_listener_vec_;   ///< listen every 30 seconds
  TimePeriodListener* p_small_time_period_listener_;               ///< listen every 1 second ... typically simtrader
  TimePeriodListenerVec small_time_period_listener_vec_;           ///< listen every 1 msec
  mutable TimePeriodListenerVec one_min_period_listener_vec_;      ///< listen every 1 minutes
  mutable TimePeriodListenerVec fifteen_min_period_listener_vec_;  ///< listen every 15 minutes

  bool stpl_present_;

#define WATCH_DATE_STRING_LEN 16
#define WATCH_TIME_STRING_LEN 16
  char date_string_[WATCH_DATE_STRING_LEN];  // changed from 9 since this would not have helped in alignment
  char time_string_[WATCH_TIME_STRING_LEN];  // changed from 14 since this would not have helped in alignment

  int last_new_page_msecs_;
  int last_big_page_msecs_;
  int last_fifteen_sec_page_msecs_;
  int last_thirty_sec_page_msecs_;
  int last_small_page_msecs_;
  int last_one_min_page_msecs_;
  int last_fifteen_min_page_msecs_;
  int day_offset_;

  std::vector<int> security_id_to_last_update_time_vec_;

 public:
  /** @brief keeps time based on the time info in the evenst coming in
   *
   * @param _dbglogger_
   * @param pre_event_YYYYMMDD_ to have a valid answer before any events come in
   */
  Watch(DebugLogger& _dbglogger_, const int pre_event_YYYYMMDD_)
      : dbglogger_(_dbglogger_),
        // tv_ ( 0 ),
        last_midnight_sec_(DateTime::GetTimeMidnightUTC(pre_event_YYYYMMDD_)),
        usecs_from_midnight_(0),
        msecs_from_midnight_(0),
        YYYYMMDD_(pre_event_YYYYMMDD_),
        new_midnight_listener_vec_(),
        time_period_listener_vec_(),
        big_time_period_listener_vec_(),
        fifteen_sec_period_listener_vec_(),
        thirty_sec_period_listener_vec_(),
        p_small_time_period_listener_(NULL),
        small_time_period_listener_vec_(),
        one_min_period_listener_vec_(),
        fifteen_min_period_listener_vec_(),
        stpl_present_(false),
        last_new_page_msecs_(0),
        last_big_page_msecs_(0),
        last_fifteen_sec_page_msecs_(0),
        last_thirty_sec_page_msecs_(0),
        last_small_page_msecs_(0),
        last_one_min_page_msecs_(0),
        last_fifteen_min_page_msecs_(0),
        day_offset_(0),
        security_id_to_last_update_time_vec_() {
    tv_.tv_sec = 0;
    tv_.tv_usec = 0;

    {
      bzero(date_string_, WATCH_DATE_STRING_LEN);
      snprintf(date_string_, WATCH_DATE_STRING_LEN, "%4d%2d%2d", (int)(pre_event_YYYYMMDD_ / 10000),
               (int)((pre_event_YYYYMMDD_ / 100) % 100), (int)(pre_event_YYYYMMDD_ % 100));
      // date_string_[8] = '\0';
    }

    {
      bzero(time_string_, WATCH_TIME_STRING_LEN);
      snprintf(time_string_, WATCH_TIME_STRING_LEN, "00:00:00 000");
      // time_string_[13] = '\0';
    }
  }

  ~Watch(){};

  inline ttime_t tv() const { return tv_; }
  std::string tv_ToString() const;
  inline time_t last_midnight_sec() const { return last_midnight_sec_; }
  inline int msecs_from_midnight() const { return msecs_from_midnight_; }
  inline int64_t usecs_from_midnight() const { return usecs_from_midnight_; }
  inline int last_fifteen_min_page_msecs() const { return last_fifteen_min_page_msecs_; }
  inline unsigned day_offset() const { return day_offset_; }
  inline int YYYYMMDD() const { return YYYYMMDD_; }
  inline int YYYY() const { return (YYYYMMDD_ / 10000); }
  inline const char* date_string() const { return date_string_; }
  inline const char* time_string() {
    unsigned int hh = std::min(24, (int)(std::max(0, msecs_from_midnight_ - day_offset_) / 3600000));
    unsigned int mm = std::min(60, (int)((std::max(0, msecs_from_midnight_ - day_offset_) - hh * 3600000) / 60000));
    unsigned int ss =
        std::min(60, (int)((std::max(0, msecs_from_midnight_ - day_offset_) - (hh * 3600000) - (mm * 60000)) / 1000));
    unsigned int msec = std::min(
        1000, (int)((std::max(0, msecs_from_midnight_ - day_offset_) - (hh * 3600000) - (mm * 60000)) - (ss * 1000)));

    bzero(time_string_, WATCH_TIME_STRING_LEN);
    snprintf(time_string_, WATCH_TIME_STRING_LEN, "%02d:%02d:%02d %03d", hh, mm, ss, msec);
    // time_string_[13] = '\0';
    return time_string_;
  }

  inline const std::string const_time_string() const {
    char t_time_string_[WATCH_TIME_STRING_LEN];  // changed from 14 since this would not have helped in alignment

    unsigned int hh = std::min(24, (int)(std::max(0, msecs_from_midnight_ - day_offset_) / 3600000));
    unsigned int mm = std::min(60, (int)((std::max(0, msecs_from_midnight_ - day_offset_) - hh * 3600000) / 60000));
    unsigned int ss =
        std::min(60, (int)((std::max(0, msecs_from_midnight_ - day_offset_) - (hh * 3600000) - (mm * 60000)) / 1000));
    unsigned int msec = std::min(
        1000, (int)((std::max(0, msecs_from_midnight_ - day_offset_) - (hh * 3600000) - (mm * 60000)) - (ss * 1000)));

    bzero(t_time_string_, WATCH_TIME_STRING_LEN);
    snprintf(t_time_string_, WATCH_TIME_STRING_LEN, "%02d:%02d:%02d %03d", hh, mm, ss, msec);
    // t_time_string_[13] = '\0';

    return std::string(t_time_string_);
  }

  const std::string UTCTimeString() const;
  const std::string NYTimeString() const;
  const std::string IndTimeString() const;

  inline void ResetWatch(int UTC_YYMMDD_) {
    int diff_dates = DateTime::GetDiffDates(YYYYMMDD_, UTC_YYMMDD_);

    YYYYMMDD_ = UTC_YYMMDD_;

    last_midnight_sec_ = DateTime::GetTimeMidnightUTC(YYYYMMDD_);
    msecs_from_midnight_ += diff_dates * 86400 * 1000;
    usecs_from_midnight_ += (int64_t)diff_dates * 86400ll * 1000ll * 1000ll;
    last_new_page_msecs_ = msecs_from_midnight_;
    last_big_page_msecs_ = msecs_from_midnight_;
    last_fifteen_sec_page_msecs_ = msecs_from_midnight_;
    last_thirty_sec_page_msecs_ = msecs_from_midnight_;
    last_small_page_msecs_ = msecs_from_midnight_;
    last_one_min_page_msecs_ = msecs_from_midnight_;
    last_fifteen_min_page_msecs_ = msecs_from_midnight_;

    if (dbglogger_.CheckLoggingLevel(WATCH_INFO)) {
      dbglogger_ << tv_ << " Initial msecs_from_midnight_ " << (int)msecs_from_midnight_ << " current: " << tv_
                 << " midnight: " << (int)last_midnight_sec_ << DBGLOG_ENDL_FLUSH;
    }
  }

  inline void OnTimeReceived_actual(const ttime_t new_tv_, int sec_id_) {
    if (tv_.tv_sec == 0) {
      // first time
      tv_ = new_tv_;

      last_midnight_sec_ = DateTime::GetTimeMidnightUTC(YYYYMMDD_);
      msecs_from_midnight_ = (((int)tv_.tv_sec - (int)last_midnight_sec_) * 1000) + (int)(tv_.tv_usec / 1000);
      usecs_from_midnight_ = (((int64_t)tv_.tv_sec - (int64_t)last_midnight_sec_) * 1000000) + (int64_t)(tv_.tv_usec);
      last_new_page_msecs_ = msecs_from_midnight_;
      last_big_page_msecs_ = msecs_from_midnight_;
      last_fifteen_sec_page_msecs_ = msecs_from_midnight_;
      last_thirty_sec_page_msecs_ = msecs_from_midnight_;
      last_small_page_msecs_ = msecs_from_midnight_;
      last_one_min_page_msecs_ = msecs_from_midnight_;
      last_fifteen_min_page_msecs_ = msecs_from_midnight_;

      if (dbglogger_.CheckLoggingLevel(WATCH_INFO)) {
        dbglogger_ << tv_ << " Initial msecs_from_midnight_ " << (int)msecs_from_midnight_ << " current: " << tv_
                   << " midnight: " << (int)last_midnight_sec_ << DBGLOG_ENDL_FLUSH;
      }
    } else {
      if (tv_ < new_tv_) {  // allowing for events not in chronological order .
        // hence enter updation area only if the new time new_tv_
        // is more than current max time received : tv_

        msecs_from_midnight_ = (((int)(new_tv_.tv_sec - last_midnight_sec_)) * 1000) + (int)(new_tv_.tv_usec / 1000);
        usecs_from_midnight_ =
            (((int64_t)new_tv_.tv_sec - (int64_t)last_midnight_sec_) * 1000000) + (int64_t)(new_tv_.tv_usec);

        if (msecs_from_midnight_ >= MAXMSECINDAY && day_offset_ == 0) {
          tv_ = new_tv_;
          if (dbglogger_.CheckLoggingLevel(WATCH_ERROR)) {
            dbglogger_ << tv_ << " Exceed MAXMSECINDAY! msecs_from_midnight_ " << msecs_from_midnight_
                       << " >= MAXMSECINDAY " << DBGLOG_ENDL_FLUSH;
          }
          day_offset_ += MAXMSECINDAY;
          // last_midnight_sec_ = DateTime::AddOneDay ( last_midnight_sec_ ) ;
          // msecs_from_midnight_ -= MAXMSECINDAY ;
          // last_new_page_msecs_ = msecs_from_midnight_;
          // last_big_page_msecs_ = msecs_from_midnight_;
          // last_fifteen_sec_page_msecs_ = msecs_from_midnight_;
          // last_small_page_msecs_ = msecs_from_midnight_;
          YYYYMMDD_ = DateTime::Get_UTC_YYYYMMDD_from_ttime(new_tv_);
          UpdateNewMidnightListeners();
        }

        tv_ = new_tv_;

        if (msecs_from_midnight_ - last_new_page_msecs_ >= WATCH_DEFAULT_PAGE_WIDTH_MSECS) {
          if ((!time_period_listener_vec_.empty())) {
            unsigned int num_pages_to_add_ =
                (unsigned int)floor((msecs_from_midnight_ - last_new_page_msecs_) / WATCH_DEFAULT_PAGE_WIDTH_MSECS);
            last_new_page_msecs_ =
                (msecs_from_midnight_ / WATCH_DEFAULT_PAGE_WIDTH_MSECS) * WATCH_DEFAULT_PAGE_WIDTH_MSECS;
            UpdateTimePeriodListeners(num_pages_to_add_);
          }
          if (msecs_from_midnight_ - last_big_page_msecs_ >= WATCH_BIG_PAGE_WIDTH_MSECS) {
            if ((!big_time_period_listener_vec_.empty())) {
              unsigned int num_big_pages_to_add_ =
                  (unsigned int)floor((msecs_from_midnight_ - last_big_page_msecs_) / WATCH_BIG_PAGE_WIDTH_MSECS);
              last_big_page_msecs_ = (msecs_from_midnight_ / WATCH_BIG_PAGE_WIDTH_MSECS) * WATCH_BIG_PAGE_WIDTH_MSECS;
              UpdateBigTimePeriodListeners(num_big_pages_to_add_);
            }
            if (msecs_from_midnight_ - last_fifteen_sec_page_msecs_ >= WATCH_FIFTEEN_SECOND_PAGE_WIDTH_MSECS) {
              if ((!fifteen_sec_period_listener_vec_.empty())) {
                unsigned int num_fifteen_sec_pages_to_add_ = (unsigned int)floor(
                    (msecs_from_midnight_ - last_fifteen_sec_page_msecs_) / WATCH_FIFTEEN_SECOND_PAGE_WIDTH_MSECS);
                last_fifteen_sec_page_msecs_ = (msecs_from_midnight_ / WATCH_FIFTEEN_SECOND_PAGE_WIDTH_MSECS) *
                                               WATCH_FIFTEEN_SECOND_PAGE_WIDTH_MSECS;
                UpdateFifteenSecondPeriodListeners(num_fifteen_sec_pages_to_add_);
              }
              if (msecs_from_midnight_ - last_thirty_sec_page_msecs_ >= WATCH_THIRTY_SECOND_PAGE_WIDTH_MSECS) {
                if (!thirty_sec_period_listener_vec_.empty()) {
                  unsigned int num_thirty_sec_pages_to_add_ = (unsigned int)floor(
                      (msecs_from_midnight_ - last_thirty_sec_page_msecs_) / WATCH_THIRTY_SECOND_PAGE_WIDTH_MSECS);
                  last_thirty_sec_page_msecs_ = (msecs_from_midnight_ / WATCH_THIRTY_SECOND_PAGE_WIDTH_MSECS) *
                                                WATCH_THIRTY_SECOND_PAGE_WIDTH_MSECS;
                  UpdateThirtySecondPeriodListeners(num_thirty_sec_pages_to_add_);
                }
                if (msecs_from_midnight_ - last_one_min_page_msecs_ >= WATCH_ONE_MINUTE_PAGE_WIDTH_MSECS) {
                  if (!one_min_period_listener_vec_.empty()) {
                    unsigned int num_one_min_pages_to_add_ = (unsigned int)floor(
                        (msecs_from_midnight_ - last_one_min_page_msecs_) / WATCH_ONE_MINUTE_PAGE_WIDTH_MSECS);
                    last_one_min_page_msecs_ =
                        (msecs_from_midnight_ / WATCH_ONE_MINUTE_PAGE_WIDTH_MSECS) * WATCH_ONE_MINUTE_PAGE_WIDTH_MSECS;
                    UpdateOneMinutePeriodListeners(num_one_min_pages_to_add_);
                  }
                  if (msecs_from_midnight_ - last_fifteen_min_page_msecs_ >= WATCH_FIFTEEN_MINUTES_PAGE_WIDTH_MSECS &&
                      (!fifteen_min_period_listener_vec_.empty())) {
                    unsigned int num_fifteen_min_pages_to_add_ = (unsigned int)floor(
                        (msecs_from_midnight_ - last_fifteen_min_page_msecs_) / WATCH_FIFTEEN_MINUTES_PAGE_WIDTH_MSECS);
                    last_fifteen_min_page_msecs_ = (msecs_from_midnight_ / WATCH_FIFTEEN_MINUTES_PAGE_WIDTH_MSECS) *
                                                   WATCH_FIFTEEN_MINUTES_PAGE_WIDTH_MSECS;
                    UpdateFifteenMinutePeriodListeners(num_fifteen_min_pages_to_add_);
                  }
                }
              }
            }
          }
        }

        if (stpl_present_) {
          int last_small_page_msecs_for_secid_;
          // for each sec id, we are maintaining the last small page msecs, so that if two updates from different
          // sec id comes within 1 msec, we notify the watch. Sec id by default is assumed to be -1, so that, it
          // doesn't affect other usages besides filesources, and in that case, the last_small_page_msecs_ is used
          // to compare the difference if greater than 1 msec.
          // security_id_to_last_update_time_vec_ is a vector to store small_page_msecs for each sec id, which is
          // initialized as packets keep arriving.
          if (sec_id_ != -1) {
            if (sec_id_ >= int(security_id_to_last_update_time_vec_.size()))
              for (int j = int(security_id_to_last_update_time_vec_.size()); j <= sec_id_; j++)
                security_id_to_last_update_time_vec_.push_back(0);
            last_small_page_msecs_for_secid_ = security_id_to_last_update_time_vec_[sec_id_];
          } else
            last_small_page_msecs_for_secid_ = last_small_page_msecs_;

          if (msecs_from_midnight_ - last_small_page_msecs_for_secid_ >= WATCH_SMALL_PAGE_WIDTH_MSECS) {
            unsigned int num_small_pages_to_add_ = (unsigned int)(msecs_from_midnight_ - last_small_page_msecs_);
            last_small_page_msecs_ =
                (msecs_from_midnight_ / WATCH_SMALL_PAGE_WIDTH_MSECS) * WATCH_SMALL_PAGE_WIDTH_MSECS;
            if (sec_id_ != -1) security_id_to_last_update_time_vec_[sec_id_] = last_small_page_msecs_;
            UpdateSmallTimePeriodListeners(num_small_pages_to_add_);
          }
        }
      } else {
        if (new_tv_ < tv_) {
          if (dbglogger_.CheckLoggingLevel(WATCH_INFO)) {
            dbglogger_ << tv_ << " Out of order event at " << new_tv_ << " , turn back by "
                       << ((int)tv_.tv_sec - (int)new_tv_.tv_sec) +
                              (((double)tv_.tv_usec - (double)new_tv_.tv_usec) / 1000000)
                       << '\n';
            dbglogger_.CheckToFlushBuffer();
          }
        }
      }
    }
  }

  inline void OnTimeReceived(const ttime_t new_tv_, int sec_id_ = -1) { OnTimeReceived_actual(new_tv_, sec_id_); }

  void subscribe_OnNewMidNight(NewMidnightListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(new_midnight_listener_vec_, _this_listener_);
    }
  }

  void subscribe_TimePeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(time_period_listener_vec_, _this_listener_);
    }
  }

  void subscribe_BigTimePeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(big_time_period_listener_vec_, _this_listener_);
    }
  }

  void subscribe_FifteenSecondPeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(fifteen_sec_period_listener_vec_, _this_listener_);
    }
  }

  void subscribe_ThirtySecondPeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(thirty_sec_period_listener_vec_, _this_listener_);
    }
  }

  void subscribe_first_SmallTimePeriod(TimePeriodListener* _this_listener_) {
    if (_this_listener_ != NULL) {
      if ((p_small_time_period_listener_ == NULL) && (small_time_period_listener_vec_.empty())) {
        p_small_time_period_listener_ = _this_listener_;
      } else {
        if (p_small_time_period_listener_) {
          VectorUtils::UniqueVectorAdd(small_time_period_listener_vec_, p_small_time_period_listener_);
          p_small_time_period_listener_ = NULL;
        }
        VectorUtils::UniqueVectorAddFirst(small_time_period_listener_vec_, _this_listener_);
      }
      stpl_present_ = true;
    }
  }

  void subscribe_SmallTimePeriod(TimePeriodListener* _this_listener_) {
    if (_this_listener_ != NULL) {
      if ((p_small_time_period_listener_ == NULL) && (small_time_period_listener_vec_.empty())) {
        p_small_time_period_listener_ = _this_listener_;
      } else {
        if (p_small_time_period_listener_) {
          VectorUtils::UniqueVectorAdd(small_time_period_listener_vec_, p_small_time_period_listener_);
          p_small_time_period_listener_ = NULL;
        }
        VectorUtils::UniqueVectorAdd(small_time_period_listener_vec_, _this_listener_);
      }
      stpl_present_ = true;
    }
  }

  void subscribe_OneMinutePeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(one_min_period_listener_vec_, _this_listener_);
    }
  }

  void subscribe_FifteenMinutesPeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAdd(fifteen_min_period_listener_vec_, _this_listener_);
    }
  }

  void subscribe_first_FifteenMinutesPeriod(TimePeriodListener* _this_listener_) const {
    if (_this_listener_ != NULL) {
      VectorUtils::UniqueVectorAddFirst(fifteen_min_period_listener_vec_, _this_listener_);
    }
  }

 protected:
  inline void UpdateNewMidnightListeners() {
    for (auto i = 0u; i < new_midnight_listener_vec_.size(); i++) {
      new_midnight_listener_vec_[i]->OnNewMidNight();
    }
  }

  inline void UpdateTimePeriodListeners(const int num_pages_to_add_) {
    for (auto i = 0u; i < time_period_listener_vec_.size(); i++) {
      time_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
    }
  }

  inline void UpdateBigTimePeriodListeners(const int num_pages_to_add_) {
    for (auto i = 0u; i < big_time_period_listener_vec_.size(); i++) {
      big_time_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
    }
  }

  inline void UpdateFifteenSecondPeriodListeners(const int num_pages_to_add_) {
    for (auto i = 0u; i < fifteen_sec_period_listener_vec_.size(); i++) {
      fifteen_sec_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
    }
  }

  inline void UpdateThirtySecondPeriodListeners(const int num_pages_to_add_) {
    for (auto i = 0u; i < thirty_sec_period_listener_vec_.size(); i++) {
      thirty_sec_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
    }
  }

  inline void UpdateSmallTimePeriodListeners(const int num_pages_to_add_) {
    if (p_small_time_period_listener_) {
      p_small_time_period_listener_->OnTimePeriodUpdate(num_pages_to_add_);
    } else {
      for (auto i = 0u; i < small_time_period_listener_vec_.size(); i++) {
        small_time_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
      }
    }
  }

  inline void UpdateOneMinutePeriodListeners(const int num_pages_to_add_) {
    for (auto i = 0u; i < one_min_period_listener_vec_.size(); i++) {
      one_min_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
    }
  }

  inline void UpdateFifteenMinutePeriodListeners(const int num_pages_to_add_) {
    for (auto i = 0u; i < fifteen_min_period_listener_vec_.size(); i++) {
      fifteen_min_period_listener_vec_[i]->OnTimePeriodUpdate(num_pages_to_add_);
    }
  }
};

// These defines are not needed outside.
// although we are sending const char * in one case.
#undef WATCH_DATE_STRING_LEN
#undef WATCH_TIME_STRING_LEN
}
#endif  // BASE_COMMONTRADEUTILS_WATCH_H
