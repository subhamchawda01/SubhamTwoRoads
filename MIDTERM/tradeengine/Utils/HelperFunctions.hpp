/**
    \file Utils/HelperFunctions.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include <cstdlib>
#include <numeric>

#define RATIOFOLDER "/spare/local/NseHftFiles/Ratio/"
#define VOLFOLDER "/spare/local/NseVolFiles/"

class HelperFunctions {
 public:
  // Functions returns a list containing ratios for num_days_ before date_ for particular tag_
  static std::map<int, double> GetRatioList(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                                            const std::string& dep_shc_, const std::string& indep_shc_,
                                            const std::string& tag_, int date_, int num_days_, double min_ratio_,
                                            double max_ratio_, bool include_current_date_ = false) {
    std::string ratio_filename_ = std::string(RATIOFOLDER) + "/" + tag_ + "/" + dep_shc_ + "_" + indep_shc_;
    std::map<int, double> ratio_map_;
    char line_[1024];

    std::ifstream ratio_file_;
    ratio_file_.open(ratio_filename_.c_str(), std::ifstream::in);

    if (!ratio_file_.is_open()) {
      _dbglogger_ << _watch_.tv() << " Could not open file " << ratio_filename_ << "\n";
      return ratio_map_;
    }
    int days_remaining_ = num_days_;

    // We will look into only upto the date that is as far as num days required in order
    // to prevent very stale values
    int min_date_ = GetPrevDayforSHC(dep_shc_, date_, num_days_);

    int expiry_date_dep_ = -1;
    if (HFSAT::NSESecurityDefinitions::IsEquity(dep_shc_) == false) {
      expiry_date_dep_ = HFSAT::NSESecurityDefinitions::ComputePreviousExpiry(date_, dep_shc_);
    }
    int expiry_date_indep_ = -1;
    if (HFSAT::NSESecurityDefinitions::IsEquity(indep_shc_) == false) {
      expiry_date_indep_ = HFSAT::NSESecurityDefinitions::ComputePreviousExpiry(date_, indep_shc_);
    }

    int closet_expiry_date_ = std::max(expiry_date_dep_, expiry_date_indep_);
    bool is_expiry_hit_dep_ = false;
    bool is_expiry_hit_indep_ = false;
    bool is_expiry_hit_ = false;

    while ((ratio_file_.good()) && (days_remaining_ > 0)) {
      memset(line_, 0, sizeof(line_));
      ratio_file_.getline(line_, sizeof(line_));

      HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
      const std::vector<const char*>& tokens_ = t_tokenizer_.GetTokens();

      if (tokens_.size() >= 2) {
        if (tokens_[0][0] == '#') {
          continue;
        }
      } else {
        continue;
      }

      char* p;
      int t_date_ = strtol(tokens_[0], &p, 10);
      char* q;
      double ratio_ = strtod(tokens_[1], &q);

      // Checking whether first token is int and second is double
      if ((*p) || (*q)) {
        continue;
      }

      // Assumption is that ratio is stored sorted by date in descending order
      if (t_date_ > date_) {
        continue;
      } else if (t_date_ < min_date_) {
        break;
      } else if (t_date_ <= closet_expiry_date_) {
        if (t_date_ <= expiry_date_dep_) {
          is_expiry_hit_dep_ = true;
        }
        if (t_date_ <= expiry_date_indep_) {
          is_expiry_hit_indep_ = true;
        }
        is_expiry_hit_ = true;
        closet_expiry_date_ = t_date_;
        break;
      } else {
        if ((!include_current_date_) && (t_date_ == date_)) {
          continue;
        }
        if ((ratio_ >= min_ratio_) && (ratio_ <= max_ratio_)) {
          if (ratio_map_.find(t_date_) == ratio_map_.end()) {
            ratio_map_[t_date_] = ratio_;
            days_remaining_--;
          }
        }
      }
    }

    if (is_expiry_hit_) {
      std::string prev_shc_dep_ = dep_shc_;
      std::string prev_shc_indep_ = indep_shc_;
      if (is_expiry_hit_dep_) {
        prev_shc_dep_ = HFSAT::NSESecurityDefinitions::GetFutureSHCInPreviousExpiry(dep_shc_);
      }
      if (is_expiry_hit_indep_) {
        prev_shc_indep_ = HFSAT::NSESecurityDefinitions::GetFutureSHCInPreviousExpiry(indep_shc_);
      }
      if ((!prev_shc_dep_.empty()) && (prev_shc_dep_.substr(0, 4) == "NSE_") && (!prev_shc_indep_.empty()) &&
          (prev_shc_indep_.substr(0, 4) == "NSE_")) {
        std::map<int, double> t_ratio_map_ =
            GetRatioList(_dbglogger_, _watch_, prev_shc_dep_, prev_shc_indep_, tag_, closet_expiry_date_,
                         days_remaining_, min_ratio_, max_ratio_, true);
        ratio_map_.insert(t_ratio_map_.begin(), t_ratio_map_.end());
      }
    }

    return ratio_map_;
  }

  static bool GetStartRatio(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_,
                            const std::string& indep_shc_, int date_, int num_days_, double min_ratio_,
                            double max_ratio_, double std_multiplier_, double& start_ratio_, double& diff_offset_) {
    int last_date_ = GetPrevDayforSHC(dep_shc_, date_);

    std::map<int, double> start_ratio_map_ = GetRatioList(_dbglogger_, _watch_, dep_shc_, indep_shc_, "StartRatio",
                                                          date_, 2 * num_days_ + 1, min_ratio_, max_ratio_, true);
    std::map<int, double> end_ratio_map_ = GetRatioList(_dbglogger_, _watch_, dep_shc_, indep_shc_, "EndRatio", date_,
                                                        2 * num_days_ + 1, min_ratio_, max_ratio_);

    // in case we don't have ratio for prev date, then we can't predict the current bid/ask ratio
    if (end_ratio_map_.find(last_date_) == end_ratio_map_.end()) {
      _dbglogger_ << _watch_.tv() << " Last Date: " << last_date_ << " End Ratio Not Available  SHC: " << dep_shc_
                  << "\n";
      return false;
    }

    int min_date_ = GetPrevDayforSHC(dep_shc_, last_date_, 2 * num_days_);

    std::vector<double> ratio_diff_vec_;
    int days_remaining_ = num_days_;

    int prev_date_ = last_date_;
    while ((prev_date_ >= min_date_) && (days_remaining_ > 0)) {
      int t_prev_date_ = GetPrevDayforSHC(dep_shc_, prev_date_);
      if ((end_ratio_map_.find(t_prev_date_) != end_ratio_map_.end()) &&
          (start_ratio_map_.find(prev_date_) != start_ratio_map_.end())) {
        _dbglogger_ << _watch_.tv() << " Date: " << prev_date_ << " StartRatio: " << start_ratio_map_[prev_date_]
                    << " Prev_Date: " << t_prev_date_ << " EndRatio: " << end_ratio_map_[t_prev_date_]
                    << " SHC: " << dep_shc_ << "\n";
        ratio_diff_vec_.push_back(start_ratio_map_[prev_date_] - end_ratio_map_[t_prev_date_]);
        days_remaining_--;
      }
      prev_date_ = t_prev_date_;
    }

    if (days_remaining_ > 0) {
      _dbglogger_ << _watch_.tv() << " Num Days for which Diff available " << (num_days_ - days_remaining_)
                  << " are less than num days required " << num_days_ << " SHC: " << dep_shc_ << "\n";
      return false;
    }

    std::sort(ratio_diff_vec_.begin(), ratio_diff_vec_.end());
    uint len = ratio_diff_vec_.size();

    if (len == 0) {
      start_ratio_ = end_ratio_map_[last_date_];
      diff_offset_ = 0;
    } else {
      double median_diff_ratio_ = ratio_diff_vec_[(len / 2)];
      if ((len % 2 == 0) && (len != 0)) {
        median_diff_ratio_ = (median_diff_ratio_ + ratio_diff_vec_[(len / 2) - 1]) / 2;
      }

      double sum = std::accumulate(ratio_diff_vec_.begin(), ratio_diff_vec_.end(), 0.0);
      double mean = sum / len;
      std::vector<double> diff(len);
      std::transform(ratio_diff_vec_.begin(), ratio_diff_vec_.end(), diff.begin(),
                     std::bind2nd(std::minus<double>(), mean));
      double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      double stdev_ = std::sqrt(sq_sum / len);

      start_ratio_ = end_ratio_map_[last_date_];
      diff_offset_ = std_multiplier_ * stdev_;
    }

    if (start_ratio_map_.find(date_) != start_ratio_map_.end()) {
      _dbglogger_ << _watch_.tv() << " RatioCompare SHC: " << dep_shc_ << " ActualRatio: " << start_ratio_map_[date_]
                  << " PredictedRatio: " << start_ratio_ << " Diff: " << diff_offset_ << "\n";
    } else {
      _dbglogger_ << _watch_.tv() << " PredictedRatio: " << start_ratio_ << "\n";
    }

    return true;
  }
  static std::map<int, double> GetVolList(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                                          const std::string& dep_shc_, const std::string& ticker_, int date_,
                                          int num_days_, int time_1_) {
    std::string vol_filename_ = std::string(VOLFOLDER) + "/" + ticker_;
    std::map<int, double> vol_map_;
    vol_map_.clear();
    char line_[1024];
    // date_list_.clear();
    std::ifstream vol_file_;
    vol_file_.open(vol_filename_.c_str(), std::ifstream::in);
    // _dbglogger_ << "vollist " << dep_shc_ << " " << time_1_ << "\n";
    if (!vol_file_.is_open()) {
      _dbglogger_ << _watch_.tv() << " Could not open file " << vol_filename_ << "\n";
      return vol_map_;
    }
    int days_remaining_ = num_days_;
    std::vector<int> date_list_;
    date_list_.clear();
    int times_ = num_days_;
    int dt_ = date_;
    while (times_ > 0) {
      dt_ = GetPrevDayforSHC(dep_shc_, dt_);
      date_list_.push_back(dt_);
      // _dbglogger_ << " datelist " << dt_ << "\n";
      times_--;
    }
    // int min_date_ =  dt_;
    int date_list_index_ = date_list_.size() - 1;
    while ((vol_file_.good()) && (days_remaining_ > 0)) {
      memset(line_, 0, sizeof(line_));
      vol_file_.getline(line_, sizeof(line_));

      HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
      const std::vector<const char*>& tokens_ = t_tokenizer_.GetTokens();

      if (tokens_.size() >= 2) {
        if (tokens_[0][0] == '#') {
          continue;
        }
      } else {
        continue;
      }

      char* p;
      int t_date_ = strtol(tokens_[0], &p, 10);
      char* q;
      int time_ = strtol(tokens_[1], &q, 10);
      char* r;
      double vol_ = strtod(tokens_[2], &r);

      while (date_list_[date_list_index_] < t_date_) {
        days_remaining_--;
        date_list_index_--;
      }

      if (date_list_[date_list_index_] == t_date_ && abs(time_ - time_1_) >= 0 && abs(time_ - time_1_) <= 3) {
        // _dbglogger_ << "VOL_TRAINING "  << ticker_ << " " << t_date_ << " " << time_ << " " << vol_ << "\n";
        days_remaining_--;
        date_list_index_--;
        vol_map_[t_date_] = vol_;
      }
    }
    return vol_map_;
    // We will look into only upto the date that is as far as num days required in order
    // to prevent very stale values
  }

  static bool GetAvgVol(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_,
                        const std::string& ticker_, int date_, int num_days_, int num_val_,
                        std::vector<double>& avg_vol_vec_) {
    int last_date_ = GetPrevDayforSHC(dep_shc_, date_);
    int min_from_midnight = _watch_.msecs_from_midnight() / 60000 + 10;
    int time_1_ = 245;
    if (min_from_midnight > time_1_) {
      time_1_ = min_from_midnight;
    }
    int total_instances_ = num_val_;
    while (total_instances_--) {
      std::map<int, double> vol_map_ = GetVolList(_dbglogger_, _watch_, dep_shc_, ticker_, date_, num_days_, time_1_);
      // in case we don't have ratio for prev date, then we can't predict the current bid/ask ratio
      if (vol_map_.find(last_date_) == vol_map_.end() && total_instances_ == num_val_ - 1) {
        _dbglogger_ << _watch_.tv() << " Last Date: " << last_date_ << " End Vol Not Available  SHC: " << dep_shc_
                    << "\n";
        return false;
      }
      int count = 0;
      double avg_vol_ = 0;
      std::vector<double> vol_vec_;
      for (std::map<int, double>::iterator it = vol_map_.begin(); it != vol_map_.end(); it++) {
        avg_vol_ += it->second;
        count++;
        vol_vec_.push_back(it->second);
      }

      // size_t n = (count-1)*0.5;
      // double adjust_ = (double)((count-1)*0.5) - (double)n;
      // // n--;
      // std::sort(vol_vec_.begin(),vol_vec_.end());
      // double median_vol_ = (1-adjust_)*vol_vec_[n] + adjust_*(vol_vec_[n+1]);

      avg_vol_ /= count;
      avg_vol_vec_.push_back(avg_vol_);
      // avg_vol_vec_.push_back(median_vol_);
      // _dbglogger_ << _watch_.tv() << " Vol values " << dep_shc_ << " " <<  avg_vol_ << " " << time_1_  << "\n";
      time_1_ += (595 - min_from_midnight) / num_val_;
      vol_map_.clear();
      vol_vec_.clear();
    }
    return true;
  }

  static int GetPrevDayforSHC(const std::string& shc_, int date_) {
    int prev_date_ = HFSAT::DateTime::CalcPrevDay(date_);
    while (HFSAT::HolidayManagerNoThrow::IsProductHoliday(shc_, prev_date_, true)) {
      prev_date_ = HFSAT::DateTime::CalcPrevDay(prev_date_);
    }
    return prev_date_;
  }

  static int GetPrevDayforSHC(const std::string& shc_, int date_, int t_times_) {
    int times_ = t_times_;
    int dt_ = date_;
    while (times_ > 0) {
      dt_ = GetPrevDayforSHC(shc_, dt_);
      times_--;
    }
    return dt_;
  }
};
