// =====================================================================================
//
//       Filename:  sample_data_util.cpp
//
//    Description:  Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/15/2016 12:38:39 PM
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

#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvccode/CommonTradeUtils/rollover_utils.hpp"

#include "dvccode/Utils/hybrid_sec.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

namespace SampleDataUtil {

// global_start_tz_hhmm_str_ and global_end_tz_hhmm_str_ saves the start_time and end_time strings (with tz)
std::string global_start_tz_hhmm_str_ = "";
std::string global_end_tz_hhmm_str_ = "";

std::map<std::string, std::map<int, double>> samplefileloadded_;
std::map<int, int> date_to_starttime_tz_;
std::map<int, int> date_to_endtime_tz_;

bool livetrading_ = false;

const int avg_numdays_[] = {120, 180, 250};

void SetLiveTrading(bool t_livetrading_) { livetrading_ = t_livetrading_; }

int GetSlotFromMfm(int mfm_) { return (int)ceil(mfm_ / 900000.0); }

int GetSlotFromHHMM(int hhmm_) {
  int curr_mins_ = (hhmm_ / 100) * 60 + (hhmm_ % 100);
  return (int)ceil(curr_mins_ / 15.0);
}

std::string GetSampleFileName(std::string feature_tag_) {
  static std::map<std::string, std::string> fname_map_{{"VOL", "RollingSumVolume300.txt"},
                                                       {"STDEV", "RollingStdev300.txt"},
                                                       {"L1SZ", "RollingAvgL1Size300.txt"},
                                                       {"L1EVPerSec", "L1EventsPerSecond.txt"},
                                                       {"TREND", "SimpleTrend300.txt"},
                                                       {"ORDSZ", "RollingAvgOrdSize300.txt"},
                                                       {"TRADES", "RollingSumTrades300.txt"},
                                                       {"SSTREND", "StableScaledTrend300.txt"},
                                                       {"TOR", "TurnOverRate300.txt"},
                                                       {"BidAskSpread", "MovingAvgBidAskSpread300.txt"},
                                                       {"AvgPrice", "AvgPrice300.txt"},
                                                       {"DELTA", "OptionsGreek300_1.txt"},
                                                       {"GAMMA", "OptionsGreek300_2.txt"},
                                                       {"VEGA", "OptionsGreek300_3.txt"},
                                                       {"THETA", "OptionsGreek300_4.txt"},
                                                       {"TrendStdev", "RollingTrendStdev300.txt"}};
  if (fname_map_.find(feature_tag_) != fname_map_.end()) {
    return fname_map_[feature_tag_];
  }

  std::string fname_ = "";

  std::vector<std::string> tokens_;
  PerishableStringTokenizer::StringSplit(feature_tag_, ' ', tokens_);

  if (tokens_.size() == 2 && tokens_[0] == "CORR") {
    fname_ = std::string("RollingCorrelation300_") + tokens_[1] + ".txt";
  } else {
    fname_ = feature_tag_ + ".txt";
  }

  fname_map_[feature_tag_] = fname_;

  return fname_;
}

/**
 *
 * @param feature_file_path_
 * @param feature_values_
 * @return
 */
int LoadSampleFile(std::string feature_file_path_, std::map<int, double> &feature_values_) {
  if (samplefileloadded_.find(feature_file_path_) != samplefileloadded_.end()) {
    feature_values_ = samplefileloadded_[feature_file_path_];
    return 1;
  } else {
    std::ifstream samples_infile_;
    samples_infile_.open(feature_file_path_.c_str(), std::ifstream::in);

    if (samples_infile_.is_open()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      while (samples_infile_.good()) {
        bzero(readline_buffer_, kL1AvgBufferLen);
        samples_infile_.getline(readline_buffer_, kL1AvgBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() == 2) {
          feature_values_[GetSlotFromHHMM(atoi(tokens_[0]))] = atof(tokens_[1]);
        }
      }
      samplefileloadded_[feature_file_path_] = feature_values_;
      return 1;
    }
    samplefileloadded_[feature_file_path_] = feature_values_;
  }
  return 0;
}

void set_global_start_tz(std::string tz_hhmm_) { global_start_tz_hhmm_str_ = tz_hhmm_; }

void set_global_end_tz(std::string tz_hhmm_) { global_end_tz_hhmm_str_ = tz_hhmm_; }

/**
 * saves into map, the (diff b/w utc and start_time_tz) and (diff b/w utc and end_time_tz) for date and current_date
 * @param date_
 * @param current_date_
 */
void GetTZDiff(int date_, int current_date_) {
  if (date_to_starttime_tz_.find(current_date_) == date_to_starttime_tz_.end()) {
    time_t start_time_tz_ = DateTime::GetTimeFromTZHHMM(current_date_, 0, global_start_tz_hhmm_str_.c_str());
    time_t start_time_utc_ = DateTime::GetTimeMidnightUTC(current_date_);
    date_to_starttime_tz_[current_date_] = start_time_tz_ - start_time_utc_;
  }
  if (date_to_endtime_tz_.find(current_date_) == date_to_endtime_tz_.end()) {
    time_t end_time_tz_ = DateTime::GetTimeFromTZHHMM(current_date_, 0, global_end_tz_hhmm_str_.c_str());
    time_t end_time_utc_ = DateTime::GetTimeMidnightUTC(current_date_);
    date_to_endtime_tz_[current_date_] = end_time_tz_ - end_time_utc_;
  }

  if (date_to_starttime_tz_.find(date_) == date_to_starttime_tz_.end()) {
    time_t start_time_tz_ = DateTime::GetTimeFromTZHHMM(date_, 0, global_start_tz_hhmm_str_.c_str());
    time_t start_time_utc_ = DateTime::GetTimeMidnightUTC(date_);
    date_to_starttime_tz_[date_] = start_time_tz_ - start_time_utc_;
  }
  if (date_to_endtime_tz_.find(date_) == date_to_endtime_tz_.end()) {
    time_t end_time_tz_ = DateTime::GetTimeFromTZHHMM(date_, 0, global_end_tz_hhmm_str_.c_str());
    time_t end_time_utc_ = DateTime::GetTimeMidnightUTC(date_);
    date_to_endtime_tz_[date_] = end_time_tz_ - end_time_utc_;
  }
}

int GetTZDiffStartTime(int date_) { return date_to_starttime_tz_[date_]; }

int GetTZDiffEndTime(int date_) { return date_to_endtime_tz_[date_]; }
/**
 *
 * @param shortcode_
 * @param date_
 * @param start_mfm_
 * @param end_mfm_
 * @param feature_tag_
 * @param feature_values_
 * @param current_date_
 */

void LoadSamplesForDay(std::string shortcode_, int date_, int start_mfm_, int end_mfm_, std::string feature_tag_,
                       std::map<int, double> &feature_values_, int current_date_) {
  bool load_prev_day_ = false;

  // get the timezone difference with utc for start_time and end_time
  // if this diff is different for today(current_date_) and the date_
  // offset start_mfm and end_mfm by ( tz_diff(date_) - tz_diff(current_date_) )
  GetTZDiff(date_, current_date_);
  int st_tz_diff_ = date_to_starttime_tz_[date_] - date_to_starttime_tz_[current_date_];
  int et_tz_diff_ = date_to_endtime_tz_[date_] - date_to_endtime_tz_[current_date_];

  start_mfm_ += st_tz_diff_ * 1000;
  end_mfm_ += et_tz_diff_ * 1000;

  int start_slot_ = GetSlotFromMfm(start_mfm_);
  int end_slot_ = GetSlotFromMfm(end_mfm_);
  if (end_slot_ > 96) {
    load_prev_day_ = true;
    end_slot_ -= 96;
  }
  if (start_slot_ < 0) {
    load_prev_day_ = true;
    start_slot_ += 96;
  }

  static HybridSecurityManager hybrid_security_manager_(date_);

  std::string feature_file_ = GetSampleFileName(feature_tag_);
  std::string shc_sname_ = shortcode_;
  double scale_ = 1;
  shc_sname_ = RollOverUtils::GetNearestMajorExpiry(shortcode_);

  if (hybrid_security_manager_.IsHybridSecurity(shortcode_)) {
    shc_sname_ = hybrid_security_manager_.GetActualSecurityFromHybrid(shortcode_);

    if (!feature_file_.compare(0, 8, "AvgPrice") || !feature_file_.compare(0, 12, "RollingStdev") ||
        !feature_file_.compare(0, 11, "SimpleTrend")) {
      SafeArray<double> price_to_yield_map_ = hybrid_security_manager_.GetYieldsForHybridShortcode(shortcode_);
      double last_act_price_ = SampleDataUtil::GetLastSampleBeforeDate(shc_sname_, date_, "AvgPrice300", 10, 0);
      if (last_act_price_ != 0) {
        double min_price_increment_ = SecurityDefinitions::GetContractMinPriceIncrement(shc_sname_, date_);
        int last_act_int_price_ = (int)round(last_act_price_ / min_price_increment_);
        double yield_min_price_increment_ =
            fabs(price_to_yield_map_[last_act_int_price_] - price_to_yield_map_[last_act_int_price_ - 1]);
        scale_ = yield_min_price_increment_ / min_price_increment_;
      }
    }
  }

  if (!load_prev_day_) {
    std::string feature_file_path_ = std::string(SAMPLEDATA_DIR);
    if (getenv("LIVE_TRADING_SAMPLEDATA")) feature_file_path_ = std::string(LIVE_SAMPLEDATA_DIR);
    feature_file_path_ += "/" + shc_sname_ + "/" + std::to_string(date_) + "/" + feature_file_;
    if (HFSAT::FileUtils::exists(feature_file_path_)) {
      std::map<int, double> t_feature_values_;
      LoadSampleFile(feature_file_path_, t_feature_values_);
      if (t_feature_values_.size() > 0) {
        for (auto &it : t_feature_values_) {
          if (it.first >= start_slot_ && it.first <= end_slot_) {
            feature_values_[it.first] = scale_ * it.second;
          }
        }
      }
    }
  } else {
    std::string feature_file_path_ = std::string(SAMPLEDATA_DIR);
    if (getenv("LIVE_TRADING_SAMPLEDATA")) feature_file_path_ = std::string(LIVE_SAMPLEDATA_DIR);
    feature_file_path_ += "/" + shc_sname_ + "/" + std::to_string(date_) + "/" + feature_file_;
    if (HFSAT::FileUtils::exists(feature_file_path_)) {
      std::map<int, double> t_feature_values_;
      LoadSampleFile(feature_file_path_, t_feature_values_);
      if (t_feature_values_.size() > 0) {
        for (auto &it : t_feature_values_) {
          if (it.first <= end_slot_) {
            feature_values_[it.first] = scale_ * it.second;
          }
        }
      }
    }
    feature_file_path_ = std::string(SAMPLEDATA_DIR);
    if (getenv("LIVE_TRADING_SAMPLEDATA")) feature_file_path_ = std::string(LIVE_SAMPLEDATA_DIR);
    feature_file_path_ += "/" + shc_sname_ + "/" + std::to_string(date_) + "/" + feature_file_;
    if (HFSAT::FileUtils::exists(feature_file_path_)) {
      std::map<int, double> t_feature_values_;
      LoadSampleFile(feature_file_path_, t_feature_values_);
      if (t_feature_values_.size() > 0) {
        for (auto &it : t_feature_values_) {
          if (it.first >= start_slot_) {
            feature_values_[it.first] = scale_ * it.second;
          }
        }
      }
    }
  }
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param numdays_
 * @param start_mfm_
 * @param end_mfm_
 * @param feature_tag_
 * @param feature_avg_
 * @param exit_on_error_
 * @return
 */
double LoadSamplesAvgFile(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                          std::string feature_tag_, std::map<int, double> &feature_avg_, bool exit_on_error_) {
  int start_slot_ = GetSlotFromMfm(start_mfm_);
  int end_slot_ = GetSlotFromMfm(end_mfm_);
  if (end_slot_ > 96) {
    end_slot_ -= 96;
  }

  static HybridSecurityManager hybrid_security_manager_(date_);

  std::string feature_file_ = GetSampleFileName(feature_tag_);
  std::string shc_sname_ = shortcode_;
  double scale_ = 1;

  shc_sname_ = RollOverUtils::GetNearestMajorExpiry(shortcode_);

  if (hybrid_security_manager_.IsHybridSecurity(shortcode_)) {
    shc_sname_ = hybrid_security_manager_.GetActualSecurityFromHybrid(shortcode_);

    if (!feature_file_.compare(0, 8, "AvgPrice") || !feature_file_.compare(0, 12, "RollingStdev") ||
        !feature_file_.compare(0, 11, "SimpleTrend")) {
      SafeArray<double> price_to_yield_map_ = hybrid_security_manager_.GetYieldsForHybridShortcode(shortcode_);
      double last_act_price_ = SampleDataUtil::GetLastSampleBeforeDate(shc_sname_, date_, "AvgPrice300", 10, 0);
      if (last_act_price_ != 0) {
        double min_price_increment_ = SecurityDefinitions::GetContractMinPriceIncrement(shc_sname_, date_);
        int last_act_int_price_ = (int)round(last_act_price_ / min_price_increment_);
        double yield_min_price_increment_ =
            fabs(price_to_yield_map_[last_act_int_price_] - price_to_yield_map_[last_act_int_price_ - 1]);
        scale_ = yield_min_price_increment_ / min_price_increment_;
      }
    }
  }

  std::string feature_file_path_ = std::string(SAMPLEDATA_DIR);
  if (getenv("LIVE_TRADING_SAMPLEDATA")) feature_file_path_ = std::string(LIVE_SAMPLEDATA_DIR);
  feature_file_path_ += "/" + shc_sname_ + "/AvgSamples/" + std::to_string(numdays_) + "/" + feature_file_;

  double sum_avg_values_ = 0;
  int count_avg_values_ = 0;

  if (HFSAT::FileUtils::exists(feature_file_path_)) {
    std::map<int, double> t_feature_values_;
    LoadSampleFile(feature_file_path_, t_feature_values_);

    if (t_feature_values_.size() > 0) {
      for (auto &it : t_feature_values_) {
        if ((start_slot_ <= end_slot_ && it.first >= start_slot_ && it.first <= end_slot_) ||
            (start_slot_ > end_slot_ && (it.first >= start_slot_ || it.first <= end_slot_))) {
          feature_avg_[it.first] = it.second;
          sum_avg_values_ += scale_ * it.second;
          count_avg_values_++;
        }
      }
    }
  }

  if (count_avg_values_ > 0) {
    return (sum_avg_values_ / count_avg_values_);
  } else {
    std::string err_msg = "SampleData Avg File not found: " + feature_tag_ + " For shortcode: " + shortcode_ +
                          " for numdays " + std::to_string(numdays_) + ", start_mfm:" + std::to_string(start_mfm_) +
                          ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
    }
    return 0;
  }
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param numdays_
 * @param start_mfm_
 * @param end_mfm_
 * @param feature_tag_
 * @param feature_avg_
 * @param exit_on_error_
 * @return
 */
double GetAvgForPeriod(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                       std::string feature_tag_, std::map<int, double> &feature_avg_, bool exit_on_error_) {
  int numdays_visited_ = 0;
  std::map<int, double> feature_sum_net_;
  std::map<int, int> feature_counts_;

  int current_date_ = date_;
  date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
  if (date_ == INVALID_DATE) {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ +
                          " since date is exchange or product start date" + ", start_mfm:" +
                          std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
    }
    return 0;
  }

  int avg_numdays_lenth_ = sizeof(avg_numdays_) / sizeof(int);

  if (numdays_ >= avg_numdays_[0]) {
    for (int idx = avg_numdays_lenth_ - 1; idx >= 0; idx--) {
      if (numdays_ >= avg_numdays_[idx]) {
        numdays_ = avg_numdays_[idx];
        break;
      }
    }
    if (livetrading_) {
      return LoadSamplesAvgFile(shortcode_, date_, numdays_, start_mfm_, end_mfm_, feature_tag_, feature_avg_,
                                exit_on_error_);
    }
  } else {
    numdays_ = std::min(numdays_, 60);
  }

  while (numdays_visited_ < numdays_) {
    std::map<int, double> feature_values_;
    LoadSamplesForDay(shortcode_, date_, start_mfm_, end_mfm_, feature_tag_, feature_values_, current_date_);

    for (auto &it : feature_values_) {
      if (feature_sum_net_.find(it.first) == feature_sum_net_.end()) {
        feature_sum_net_[it.first] = 0;
        feature_counts_[it.first] = 0;
      }
      feature_sum_net_[it.first] += it.second;
      feature_counts_[it.first] += 1;
    }
    // std::cout << numdays_visited_ << " " << date_ << " " << feature_tag_ << " " << feature_values_.size() <<
    // std::endl;
    numdays_visited_++;
    date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
    if (date_ == INVALID_DATE) break;
  }

  if (numdays_visited_ < numdays_) numdays_ = numdays_visited_;
  int min_count_ = std::max(1, (int)(numdays_ / 3));

  double sum_avg_values_ = 0, count_avg_values_ = 0;
  for (auto &it : feature_sum_net_) {
    // std::cout << feature_tag_ << " " << it.first << ": " << it.second << " " << feature_counts_[it.first] <<
    // std::endl;
    if (feature_counts_[it.first] < min_count_) {
      continue;
    }

    feature_avg_[it.first] = it.second / feature_counts_[it.first];
    sum_avg_values_ += feature_avg_[it.first];
    count_avg_values_++;
  }
  if (count_avg_values_ > 0) {
    return (sum_avg_values_ / count_avg_values_);
  } else {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ + " for atleast " +
                          std::to_string(min_count_) + " days in the last " + std::to_string(numdays_) + " days" +
                          ", start_mfm:" + std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
    }
    return 0;
  }
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param numdays_
 * @param start_mfm_
 * @param end_mfm_
 * @param feature_tag_
 * @param exit_on_error_
 * @return
 */
double GetAvgForPeriod(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                       std::string feature_tag_, bool exit_on_error_) {
  std::map<int, double> feature_avg_;
  return GetAvgForPeriod(shortcode_, date_, numdays_, start_mfm_, end_mfm_, feature_tag_, feature_avg_, exit_on_error_);
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param numdays_
 * @param feature_tag_
 * @param exit_on_error_
 * @return
 */
double GetAvgForPeriod(std::string shortcode_, int date_, int numdays_, std::string feature_tag_, bool exit_on_error_) {
  int start_mfm_ = 0;
  int end_mfm_ = 86400000;
  return GetAvgForPeriod(shortcode_, date_, numdays_, start_mfm_, end_mfm_, feature_tag_, exit_on_error_);
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param numdays_
 * @param start_mfm_
 * @param end_mfm_
 * @param feature_tag_
 * @param percentile_
 * @param exit_on_error_
 * @return
 */
double GetPercentileForPeriod(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                              std::string feature_tag_, double percentile_, bool exit_on_error_) {
  int numdays_visited_ = 0;
  int current_date_ = date_;
  date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
  if (date_ == INVALID_DATE) {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ +
                          " since date is exchange or product start date" + ", start_mfm:" +
                          std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
      return 0;
    }
  }

  numdays_ = std::min(numdays_, 60);
  std::vector<double> feature_vals_;

  while (numdays_visited_ < numdays_) {
    std::map<int, double> feature_values_;
    LoadSamplesForDay(shortcode_, date_, start_mfm_, end_mfm_, feature_tag_, feature_values_, current_date_);

    for (auto &it : feature_values_) {
      feature_vals_.push_back(it.second);
    }
    numdays_visited_++;
    date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
    if (date_ == INVALID_DATE) break;
  }

  if (numdays_visited_ < numdays_) numdays_ = numdays_visited_;
  std::sort(feature_vals_.begin(), feature_vals_.end());

  percentile_ = std::min(1.0, percentile_);
  size_t percentile_index_ = std::max(0, (int)(feature_vals_.size() * percentile_) - 1);

  if (feature_vals_.size() > percentile_index_) {
    return feature_vals_[percentile_index_];
  } else {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ + " start_mfm:" +
                          std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_) + " in the last " +
                          std::to_string(numdays_) + " days";
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
    }
    return 0;
  }
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param numdays_
 * @param start_mfm_
 * @param end_mfm_
 * @param feature_tag_
 * @param exit_on_error_
 * @return
 */
double GetAvgForDayTotal(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                         std::string feature_tag_, bool exit_on_error_) {
  int numdays_visited_ = 0;
  std::map<int, double> feature_daysums_;
  double feature_sum_ = 0;
  double day_count_ = 0;

  int current_date_ = date_;
  date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
  if (date_ == INVALID_DATE) {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ +
                          " since date is exchange or product start date" + ", start_mfm:" +
                          std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
      return 0;
    }
  }

  numdays_ = std::min(numdays_, 60);
  std::vector<double> feature_vals_;
  while (numdays_visited_ < numdays_) {
    std::map<int, double> feature_values_;
    LoadSamplesForDay(shortcode_, date_, start_mfm_, end_mfm_, feature_tag_, feature_values_, current_date_);

    if (feature_values_.size() > 0) {
      feature_daysums_[date_] = 0;
      for (auto &it : feature_values_) {
        feature_daysums_[date_] += it.second;
      }
      feature_sum_ += feature_daysums_[date_];
      day_count_++;
    }
    numdays_visited_++;
    date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
    if (date_ == INVALID_DATE) break;
  }

  if (numdays_visited_ < numdays_) numdays_ = numdays_visited_;
  int min_count_ = std::max(1, (int)(numdays_ / 3));

  if (day_count_ >= min_count_) {
    return (feature_sum_ / day_count_);
  } else {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ + " for atleast " +
                          std::to_string(min_count_) + " days in the last " + std::to_string(numdays_) + " days" +
                          " start_mfm:" + std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
    }
    return 0;
  }
}

/**
 *
 * @param shortcode_
 * @param date_
 * @param feature_tag_
 * @param numdays_
 * @param exit_on_error_
 * @return
 */
double GetLastSampleBeforeDate(std::string shortcode_, int date_, std::string feature_tag_, int numdays_,
                               bool exit_on_error_) {
  int start_mfm_ = 0;
  int end_mfm_ = 86400000;
  int numdays_visited_ = 0;

  int current_date_ = date_;
  date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
  if (date_ == INVALID_DATE) {
    std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ +
                          " since date is exchange or product start date" + ", start_mfm:" +
                          std::to_string(start_mfm_) + ", end_mfm:" + std::to_string(end_mfm_);
    if (exit_on_error_) {
      ExitVerbose(kSampleDataError, err_msg.c_str());
    } else {
      std::cout << "ERROR: " << err_msg << std::endl;
      return 0;
    }
  }

  while (numdays_visited_ < numdays_) {
    std::map<int, double> feature_values_;
    LoadSamplesForDay(shortcode_, date_, start_mfm_, end_mfm_, feature_tag_, feature_values_, current_date_);

    if (feature_values_.size() > 0) {
      return feature_values_.rbegin()->second;
    }
    numdays_visited_++;
    date_ = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForProduct(shortcode_, date_);
    if (date_ == INVALID_DATE) break;
  }

  if (numdays_visited_ < numdays_) numdays_ = numdays_visited_;
  std::string err_msg = "SampleData not found: " + feature_tag_ + " For shortcode: " + shortcode_ +
                        " for any slot in the last " + std::to_string(numdays_) + " days";
  if (exit_on_error_) {
    ExitVerbose(kSampleDataError, err_msg.c_str());
  } else {
    std::cout << "ERROR: " << err_msg << std::endl;
  }
  return 0;
}

void ClearSampleDataUtilsVars() {
  samplefileloadded_.clear();
  date_to_starttime_tz_.clear();
  date_to_endtime_tz_.clear();
  global_start_tz_hhmm_str_ = "";
  global_end_tz_hhmm_str_ = "";
}
}
}
