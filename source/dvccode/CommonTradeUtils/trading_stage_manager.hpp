/**
   \file dvccode/CommonTradeUtils/trading_stage_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef BASE_COMMONTRADEUTILS_TRADING_STAGE_MANAGER_HPP
#define BASE_COMMONTRADEUTILS_TRADING_STAGE_MANAGER_HPP

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#define OFFSET_MFM 2000  // 2 secs ... subtract from freeze_start time and add to freeze_end time ?

// To freeze trading during no-cancel / pre-open periods
// Also remove orders from top level which is thin

namespace HFSAT {
class TradingStageManager {
 public:
  static TradingStageManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                const std::string& _dep_shortcode_, const ExchSource_t _exch_source_) {
    static TradingStageManager* p_unique_instance_ = NULL;
    if (p_unique_instance_ == NULL) {
      p_unique_instance_ = new TradingStageManager(_dbglogger_, _watch_, _dep_shortcode_, _exch_source_);
    }
    return p_unique_instance_;
  }

  inline bool ShouldFreezeNow() {
    int curr_time = watch_.msecs_from_midnight();
    for (size_t i = 0; i < freeze_start_mfm_vec_.size(); ++i)
      if ((curr_time >= freeze_start_mfm_vec_[i]) && (curr_time <= freeze_end_mfm_vec_[i])) return (true);
    return (false);
  }

 private:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const std::string dep_shortcode_;
  ExchSource_t this_smv_exch_source_;
  std::vector<int> freeze_start_mfm_vec_;
  std::vector<int> freeze_end_mfm_vec_;

  TradingStageManager(DebugLogger& t_dbglogger_, const Watch& t_watch_, const std::string& t_dep_shortcode_,
                      const ExchSource_t t_exch_source_)
      : dbglogger_(t_dbglogger_),
        watch_(t_watch_),
        dep_shortcode_(t_dep_shortcode_),
        this_smv_exch_source_(t_exch_source_),
        freeze_start_mfm_vec_(),
        freeze_end_mfm_vec_() {
    if (!dep_shortcode_.compare(0, 3, "BAX")) {  // BAX family , consider BAX_*
      // freeze_start_mfm_vec_.push_back ( 42300000 ) ; // 0745 TORONTO => 1145 GMT => 42 300 000 mfm // KP :: not any
      // more
      // freeze_end_mfm_vec_.push_back ( 43200000 ) ; // 0800 => 1200 => 43 200 000 mfm
    } else if (dep_shortcode_.find("SP_VX") == 0 || dep_shortcode_.find("VX_") == 0) {
      const char* t_start_time_ = "EST_1615";
      const char* t_end_time_ = "EST_1630";
      int t_freeze_start_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
          watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(t_start_time_ + 4), t_start_time_));
      int t_freeze_end_ = GetMsecsFromMidnightFromHHMMSS(
          DateTime::GetUTCHHMMSSFromTZHHMMSS(watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(t_end_time_ + 4), t_end_time_));
      freeze_start_mfm_vec_.push_back(t_freeze_start_);
      freeze_end_mfm_vec_.push_back(t_freeze_end_);
    } else if (dep_shortcode_.find("DI1") == 0 && this_smv_exch_source_ == HFSAT::kExchSourceBMF) {
      const char* t_start_time_ = "BRT_1559";
      const char* t_end_time_ = "BRT_1650";
      int t_freeze_start_ = GetMsecsFromMidnightFromHHMMSS(DateTime::GetUTCHHMMSSFromTZHHMMSS(
          watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(t_start_time_ + 4), t_start_time_));
      int t_freeze_end_ = GetMsecsFromMidnightFromHHMMSS(
          DateTime::GetUTCHHMMSSFromTZHHMMSS(watch_.YYYYMMDD(), DateTime::GetHHMMSSTime(t_end_time_ + 4), t_end_time_));
      freeze_start_mfm_vec_.push_back(t_freeze_start_);
      freeze_end_mfm_vec_.push_back(t_freeze_end_);
    } else {
      // nothing
      // add other products or read from some file / paramfile
    }

    for (size_t i = 0; i < freeze_start_mfm_vec_.size(); ++i) {
      freeze_start_mfm_vec_[i] -= OFFSET_MFM;  // this is an assumption, for VX
      freeze_end_mfm_vec_[i] += OFFSET_MFM;
    }
  }
};
}

#endif /* BASE_COMMONTRADEUTILS_TRADING_STAGE_MANAGER_HPP */
