/**
    \file Utils/BarGenerator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#ifndef _BAR_GENERATOR_H
#define _BAR_GENERATOR_H

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include "dvccode/Utils/holiday_manager_utils.hpp"
#include "dvccode/Utils/holiday_manager.hpp"
#define BARDATAFOLDER "/spare/local/BarData"
// #define THEOKFOLDER "/spare/local/NseHftFiles/TheoK/"

struct BarInfo {
  int time_;
  double open_;
  double close_;
  double low_;
  double high_;
  double vol_;
  // BarInfo(int _time, double _open, double _close, double _low, double _high, double _vol) : time_(_time),
  // open_(_open), close_(_close), low_(_low), high_(_high) { },
  // BarInfo(){}
};

class BarGenerator {
 protected:
  std::vector<BarInfo> day_bar_info_;
  std::vector<BarInfo> granular_bar_info_;
  std::vector<BarInfo> MinuteBarInfo;
  std::vector<int> date_list_;
  double day_bollinger_;
  double day_bollinger_std_;
  double mr_hl_mean_;

 public:
  void GetBarData(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_,
                  const std::string& ticker_symbol_, int date_, int num_days_, int granularity_, double& adj_ratio_) {
    std::string bardata_filename_ = std::string(BARDATAFOLDER) + "/" + ticker_symbol_;
    // std::map<int,double> ratio_map_;
    char line_[1024];
    // std::cout << dep_shc_ << " " << ticker_symbol_ << " " << date_ << " " << num_days_ << " " << granularity_ <<
    // "\n";
    std::ifstream bar_data_file_;
    bar_data_file_.open(bardata_filename_.c_str(), std::ifstream::in);

    if (!bar_data_file_.is_open()) {
      _dbglogger_ << _watch_.tv() << " Could not open file " << bardata_filename_ << "\n";
      return;
    }

    int min_date_ = GetPrevDayforSHC(dep_shc_, date_, num_days_);

    int start_epoch_ = HFSAT::DateTime::GetTimeFromTZHHMM(min_date_, 0, "IST_0");
    int end_epoch_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_, 0, "IST_0");
    int day_index_ = date_list_.size() - 1;
    int day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
    int current_granularity_bar_ = day_start_;
    int day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 1530, "IST_1530");
    // std::cout << start_epoch_ << " " << end_epoch_ << " " << day_start_ << " " << day_end_ <<std::endl;

    double gran_open_ = 0, gran_close_ = 0, gran_low_ = 0, gran_high_ = 0, gran_vol_ = 0;
    double day_open_ = 0, day_close_ = 0, day_low_ = 0, day_high_ = 0, day_vol_ = 0;
    int gran_time_ = 0;
    bool eod_reached_ = true;
    BarInfo b;
    while (bar_data_file_.good()) {
      memset(line_, 0, sizeof(line_));
      bar_data_file_.getline(line_, sizeof(line_));
      HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
      const std::vector<const char*>& tokens_ = t_tokenizer_.GetTokens();
      if (tokens_.size() == 0 || tokens_[0][0] == '#') {
        continue;
      }
      char* p;
      char* q;
      int time_ = strtol(tokens_[0], &p, 10);
      // std::cout << " time " << time_ <<"\n";
      if (time_ < start_epoch_) {
        continue;
      } else if (time_ > end_epoch_) {
        eod_reached_ = false;
        if (gran_open_) {
          // b = new BarInfo(gran_time_,gran_open_,gran_close_,gran_low_,gran_high_,gran_vol_);
          b.time_ = gran_time_;
          b.open_ = gran_open_;
          b.close_ = gran_close_;
          b.low_ = gran_low_;
          b.high_ = gran_high_;
          b.vol_ = gran_vol_;
          // std::cout <<"GRAN0: "<< gran_time_ << " " <<gran_open_ << " "<< gran_close_ << " " << gran_low_ << " "<<
          // gran_high_ << " " << gran_vol_ << "\n";
          granular_bar_info_.push_back(b);
        }
        if (day_open_) {
          b.time_ = day_start_;
          b.open_ = day_open_;
          b.close_ = day_close_;
          b.low_ = day_low_;
          b.high_ = day_high_;
          b.vol_ = day_vol_;
          // b = new BarInfo(day_start_,day_open_,day_close_,day_low_,day_high_,day_vol_);
          // std::cout <<"DAY: " <<date_list_[day_index_] << " "<< day_start_ << " " <<day_open_ << " "<< day_close_ <<
          // " " << day_low_ << " "<< day_high_ << " " << day_vol_ << " " << day_end_ << "\n";
          day_bar_info_.push_back(b);
        }
        adj_ratio_ = strtod(tokens_[11], &q);

        break;
      }

      double open_ = strtod(tokens_[5], &q);
      double close_ = strtod(tokens_[6], &q);
      double low_ = strtod(tokens_[7], &q);
      double high_ = strtod(tokens_[8], &q);
      double vol_ = strtod(tokens_[9], &q);
      if(vol_ == 0){
        continue;
      }
      b.time_ = time_;
      b.open_ = open_;
      b.close_ = close_;
      b.low_ = low_;
      b.high_ = high_;
      b.vol_ = vol_;
      // std::cout << time_ << " " << open_ << " " << close_ << " " << low_ << " " << high_ << " "<<vol_  << " " << day_start_ << " " << day_end_ << "\n";
      MinuteBarInfo.push_back(b);

      if (time_ == day_end_) {
        if (!gran_open_) {
          gran_open_ = open_;
        }
        gran_close_ = close_;
        if (gran_low_ > low_) {
          gran_low_ = low_;
        }
        if (gran_high_ < high_) {
          gran_high_ = high_;
        }
        gran_vol_ += vol_;
        if (!day_open_) {
          day_open_ = open_;
        }
        day_close_ = close_;
        if (day_low_ > low_) {
          day_low_ = low_;
        }
        if (day_high_ < high_) {
          day_high_ = high_;
        }
        day_vol_ += vol_;
        if (gran_open_) {
          b.time_ = gran_time_;
          b.open_ = gran_open_;
          b.close_ = gran_close_;
          b.low_ = gran_low_;
          b.high_ = gran_high_;
          b.vol_ = gran_vol_;
          // std::cout <<"GRAN1: "<< gran_time_ << " " <<gran_open_ << " "<< gran_close_ << " " << gran_low_ << " "<<
          // gran_high_ << " " << gran_vol_ << "\n";
          granular_bar_info_.push_back(b);
          gran_open_ = 0;
        }
        if (day_open_) {
          b.time_ = day_start_;
          b.open_ = day_open_;
          b.close_ = day_close_;
          b.low_ = day_low_;
          b.high_ = day_high_;
          b.vol_ = day_vol_;
          // std::cout <<"DAY: " <<date_list_[day_index_] << " "<< day_start_ << " " <<day_open_ << " "<< day_close_ <<
          // " " << day_low_ << " "<< day_high_ << " " << day_vol_ << " " << day_end_ << "\n";
          day_bar_info_.push_back(b);
        }
        day_index_--;
        if (day_index_ < 0) {
          break;
        }
        day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
        current_granularity_bar_ = day_start_;
        day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 1530, "IST_1530");
        day_open_ = 0;
        day_close_ = 0;
        day_low_ = 0;
        day_high_ = 0;
        day_vol_ = 0;
        gran_time_ = day_start_;
      } else if (time_ > day_end_) {
        if (gran_open_) {
          b.time_ = gran_time_;
          b.open_ = gran_open_;
          b.close_ = gran_close_;
          b.low_ = gran_low_;
          b.high_ = gran_high_;
          b.vol_ = gran_vol_;
          // std::cout <<"GRAN2: "<< gran_time_ << " " <<gran_open_ << " "<< gran_close_ << " " << gran_low_ << " "<<
          // gran_high_ << " " << gran_vol_ << "\n";
          granular_bar_info_.push_back(b);
          gran_open_ = 0;
        }
        if (day_open_) {
          b.time_ = day_start_;
          b.open_ = day_open_;
          b.close_ = day_close_;
          b.low_ = day_low_;
          b.high_ = day_high_;
          b.vol_ = day_vol_;
          // std::cout <<"DAY: " <<date_list_[day_index_] << " "<< day_start_ << " " <<day_open_ << " "<< day_close_ <<
          // " " << day_low_ << " "<< day_high_ << " " << day_vol_ << " " << day_end_ << "\n";
          day_bar_info_.push_back(b);
        }

        day_index_--;
        if (day_index_ < 0) {
          break;
        }

        day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
        current_granularity_bar_ = day_start_;
        day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 1530, "IST_1530");
        while (day_end_ <= time_) {
          day_index_--;
          if (day_index_ < 0) {
            break;
          }
          day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
          current_granularity_bar_ = day_start_;
          day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 1530, "IST_1530");
        }
        if (day_index_ < 0) {
          break;
        }
        day_open_ = open_;
        day_close_ = close_;
        day_low_ = low_;
        day_high_ = high_;
        day_vol_ = vol_;
        gran_time_ = day_start_;
        gran_open_ = open_;
        gran_close_ = close_;
        gran_low_ = low_;
        gran_high_ = high_;
        gran_vol_ = vol_;
      }

      else {
        if (!day_open_) {
          day_open_ = day_start_;
          day_close_ = close_;
          day_low_ = low_;
          day_high_ = high_;
          day_vol_ = vol_;
        }

        day_close_ = close_;
        if (day_low_ > low_) {
          day_low_ = low_;
        }
        if (day_high_ < high_) {
          day_high_ = high_;
        }
        day_vol_ += vol_;

        if (time_ >= current_granularity_bar_ + granularity_) {
          // b = new BarInfo(gran_time_,gran_open_,gran_close_,gran_low_,gran_high_,gran_vol_);
          b.time_ = gran_time_;
          b.open_ = gran_open_;
          b.close_ = gran_close_;
          b.low_ = gran_low_;
          b.high_ = gran_high_;
          b.vol_ = gran_vol_;
          // std::cout <<"GRAN3: "<< gran_time_ << " " <<gran_open_ << " "<< gran_close_ << " " << gran_low_ << " "<<
          // gran_high_ << " " << gran_vol_ << "\n";
          granular_bar_info_.push_back(b);
          while(time_ >= current_granularity_bar_ + granularity_){
            current_granularity_bar_ += granularity_;
          }
          gran_time_ = current_granularity_bar_;
          gran_open_ = open_;
          gran_close_ = close_;
          gran_low_ = low_;
          gran_high_ = high_;
          gran_vol_ = vol_;
        } else if (!gran_open_) {
          gran_time_ = current_granularity_bar_;
          gran_open_ = open_;
          gran_close_ = close_;
          gran_low_ = low_;
          gran_high_ = high_;
          gran_vol_ = vol_;
        }

        else {
          gran_close_ = close_;
          if (gran_low_ > low_) {
            gran_low_ = low_;
          }
          if (gran_high_ < high_) {
            gran_high_ = high_;
          }
          gran_vol_ += vol_;
        }
      }
    }
    if (eod_reached_) {
      if (gran_open_) {
        // b = new BarInfo(gran_time_,gran_open_,gran_close_,gran_low_,gran_high_,gran_vol_);
        b.time_ = gran_time_;
        b.open_ = gran_open_;
        b.close_ = gran_close_;
        b.low_ = gran_low_;
        b.high_ = gran_high_;
        b.vol_ = gran_vol_;
        // std::cout <<"GRAN4: "<< gran_time_ << " " <<gran_open_ << " "<< gran_close_ << " " << gran_low_ << " "<<
        // gran_high_ << " " << gran_vol_ << "\n";
        granular_bar_info_.push_back(b);
      }
      if (day_open_) {
        b.time_ = day_start_;
        b.open_ = day_open_;
        b.close_ = day_close_;
        b.low_ = day_low_;
        b.high_ = day_high_;
        b.vol_ = day_vol_;
        // b = new BarInfo(day_start_,day_open_,day_close_,day_low_,day_high_,day_vol_);
        // std::cout <<"DAY: " <<date_list_[day_index_] << " "<< day_start_ << " " <<day_open_ << " "<< day_close_ << "
        // " << day_low_ << " "<< day_high_ << " " << day_vol_ << " " << day_end_ << "\n";
        day_bar_info_.push_back(b);
      }
      adj_ratio_ = 1;
    }
  }

  int GetPrevDayforSHC(const std::string& shc_, int date_) {
    int prev_date_ = HFSAT::DateTime::CalcPrevDay(date_);
    while (HFSAT::HolidayManagerNoThrow::IsProductHoliday(shc_, prev_date_, true)) {
      // std::cout << prev_date_ << std::endl;
      // date_list_.push_back(prev_date_);
      prev_date_ = HFSAT::DateTime::CalcPrevDay(prev_date_);
    }
    // std::cout << prev_date_ << std::endl;
    date_list_.push_back(prev_date_);
    return prev_date_;
  }

  int GetPrevDayforSHC(const std::string& shc_, int date_, int t_times_) {
    // date_list_.push_back(date_);
    int times_ = t_times_;
    int dt_ = date_;
    while (times_ > 0) {
      int prev_date_ = HFSAT::DateTime::CalcPrevDay(dt_);

      while (HFSAT::HolidayManagerNoThrow::IsProductHoliday(shc_, prev_date_, true)) {
        if(HFSAT::DateTime::IsWeekDay(prev_date_)){
          times_--;
          
        }
        // std::cout << prev_date_ << std::endl;
        // date_list_.push_back(prev_date_);
        prev_date_ = HFSAT::DateTime::CalcPrevDay(prev_date_);
      }
      if (times_ > 0) {
        date_list_.push_back(prev_date_);
        dt_ = prev_date_;
        // dt_ = GetPrevDayforSHC(shc_,dt_);
        times_--;
      }
    }
    return dt_;
  }
  int GetNumHolidays(const std::string& shc_, int date_, int t_times_) {
    // date_list_.push_back(date_);
    int times_ = t_times_;
    int dt_ = date_;
    int num_holidays_ = 0;
    while (times_ > 0) {
      int prev_date_ = HFSAT::DateTime::CalcPrevDay(dt_);
      while (HFSAT::HolidayManagerNoThrow::IsProductHoliday(shc_, prev_date_, true)) {
        // std::cout << prev_date_ << std::endl;
        // date_list_.push_back(prev_date_);
        if(HFSAT::DateTime::IsWeekDay(prev_date_)){
          times_--;
          num_holidays_++;
        }
        prev_date_ = HFSAT::DateTime::CalcPrevDay(prev_date_);
      }
      if (times_ > 0) {
        // date_list_.push_back(prev_date_);
        dt_ = prev_date_;
        // dt_ = GetPrevDayforSHC(shc_,dt_);
        times_--;
      }
    }
    return num_holidays_;
  }

  void getKeyFilters(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_,
                     const std::string& ticker_symbol_, int date_, int num_days_, double open_vol_ptile_,
                     int granularity_, int cc_lkbk_, int long_term_lkbk_, double& moment_cc_wt_mean_,
                     double& moment_cc_wt_stdev_, double& open_vol_avg_, double& long_term_vol_std_,
                     double& long_term_vol_mean_, double& prev_day_close_, double& adj_ratio_,
                     double& long_term_obv_std_, int obv_long_term_lkbk_ = 0) {
    GetBarData(_dbglogger_, _watch_, dep_shc_, ticker_symbol_, date_, num_days_, granularity_, adj_ratio_);
    int num_holidays_in_lkbk_ = GetNumHolidays(dep_shc_, date_, long_term_lkbk_);
    int num_holidays_in_obv_lkbk_ = 0;
    if (obv_long_term_lkbk_ != 0) {
      num_holidays_in_obv_lkbk_ = GetNumHolidays(dep_shc_, date_, obv_long_term_lkbk_);
    }

    std::vector<double> cc_vec_, long_term_volatility_vec_, obv_vec_, bollinger_vec_;
    int day_bar_index_ = day_bar_info_.size() - 1;
    int day_index_ = 0;
    int current_obv_ = 0;
    int gran_bar_index_ = granular_bar_info_.size() - 1;
    int open_vol_count_ = 0;
    double next_day_close_ = 0;
    double next_day_hl_ = 0;
    double current_normalized_obv_ = 0;
    open_vol_avg_ = 0;
    prev_day_close_ = 0;
    // std::cout << "BARDATA accumulate\n";
    // for(int i=0;i<=gran_bar_index_;i++){
    // 	std::cout << " GRAN : " << granular_bar_info_[i].time_ << " " << granular_bar_info_[i].vol_ <<std::endl;
    // }
    std::vector<double> open_bar_volume_vec_;

    int prev_day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[0], 915, "IST_915");

    while (day_bar_index_ >= 0 && gran_bar_index_ >= 0) {
      int day_bar_index_start_ = day_bar_info_[day_bar_index_].time_;
      int day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
      // std::cout <<day_start_ << " " <<  day_bar_index_ << " " << (int)date_list_.size()-1 <<std::endl;
      // std::cout << day_bar_index_start_ << " " << day_bar_info_[day_bar_index_].close_ << std::endl;
      // int day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_bar_index_], 1530, "IST_1530");
      while (granular_bar_info_[gran_bar_index_].time_ > day_start_) {
        current_obv_ += (granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_) *
                        granular_bar_info_[gran_bar_index_].vol_;
        // _dbglogger_ << " netvol " << granular_bar_info_[gran_bar_index_].time_ << " " << current_obv_ << " " <<
        // granular_bar_info_[gran_bar_index_].close_ << " " << granular_bar_info_[gran_bar_index_].open_ << " " <<
        // granular_bar_info_[gran_bar_index_].vol_ << "\n";
        gran_bar_index_--;
      }
      if (gran_bar_index_ >= 0 && granular_bar_info_[gran_bar_index_].time_ == day_start_ &&
          granular_bar_info_[gran_bar_index_].vol_) {
        // open_vol_avg_+= granular_bar_info_[gran_bar_index_].vol_;
        open_bar_volume_vec_.push_back(granular_bar_info_[gran_bar_index_].vol_);
        current_obv_ += (granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_) *
                        granular_bar_info_[gran_bar_index_].vol_;
        // _dbglogger_ << " netvol " << granular_bar_info_[gran_bar_index_].time_ << " " << current_obv_ << " " <<
        // granular_bar_info_[gran_bar_index_].close_ << " " << granular_bar_info_[gran_bar_index_].open_ << " " <<
        // granular_bar_info_[gran_bar_index_].vol_ << "\n";
        // _dbglogger_ << day_start_ << " " << granular_bar_info_[gran_bar_index_].vol_ << " \n";
        // _dbglogger_ << "ticker_symbol_ " << ticker_symbol_<<" vol: " << granular_bar_info_[gran_bar_index_].vol_
        // <<"\n";
        // std::cout << " GRAN : " << granular_bar_info_[gran_bar_index_].time_ << " " <<
        // granular_bar_info_[gran_bar_index_].vol_ <<std::endl;
        open_vol_count_++;
      } else {
        gran_bar_index_++;
        if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
          _dbglogger_ << day_bar_index_start_ << " ERROR : Opening volume not found for previous day"
                      << "\n";
          _dbglogger_.DumpCurrentBuffer();
        }
      }

      // int long_term_lkbk_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[long_term_lkbk_], 915, "IST_915");
      // if(day_bar_index_ >= (int)date_list_.size()-1-long_term_lkbk_) {
      if (obv_long_term_lkbk_ == 0 || day_index_ <= obv_long_term_lkbk_ - num_holidays_in_obv_lkbk_) {
        if (day_index_ <= long_term_lkbk_ - num_holidays_in_lkbk_) {
          // std::cout << "prev lookback " << day_bar_index_start_ << " " << long_term_lkbk_start_<<std::endl;
          if (day_bar_info_[day_bar_index_].time_ >= day_start_) {
            if (next_day_close_) {
              if ((int)cc_vec_.size() < cc_lkbk_) {
                cc_vec_.push_back(abs(next_day_close_ - day_bar_info_[day_bar_index_].close_));
                mr_hl_mean_ += next_day_hl_;
                // std::cout << day_bar_index_start_ << " " << next_day_close_ << " " <<
                // day_bar_info_[day_bar_index_].close_ << "\n";
                day_bollinger_ += next_day_close_;
                bollinger_vec_.push_back(next_day_close_);
              }
              long_term_volatility_vec_.push_back(abs(next_day_close_ - day_bar_info_[day_bar_index_].close_));
              obv_vec_.push_back(current_normalized_obv_ / day_bar_info_[day_bar_index_].close_);
              // _dbglogger_ << day_bar_info_[day_bar_index_].time_ << " " << obv_vec_[obv_vec_.size()-1] << "\n";
              // ";
            }
            next_day_close_ = day_bar_info_[day_bar_index_].close_;
            next_day_hl_ = day_bar_info_[day_bar_index_].high_ - day_bar_info_[day_bar_index_].low_;

            current_normalized_obv_ = abs(current_obv_ / day_bar_info_[day_bar_index_].vol_);
            if (!prev_day_close_) {
              prev_day_close_ = day_bar_info_[day_bar_index_].close_;
            }
          } else {
            // next_day_close_ = 0;
            if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
              _dbglogger_ << "ERROR : Closing price not found for previous day"
                          << "\n";
              _dbglogger_.DumpCurrentBuffer();
              return;
            }
            day_bar_index_++;
          }
        }
      }

      day_bar_index_--;
      gran_bar_index_--;
      day_index_++;
      current_obv_ = 0;
    }
    // std::cout << cc_vec_.size() << " " << long_term_volatility_vec_.size() << " " << prev_day_close_<< std::endl;
    // open_vol_avg_ /= open_vol_count_;
    size_t n = (open_bar_volume_vec_.size() - 1) * open_vol_ptile_ / 100;
    double adjust_ = (double)((open_bar_volume_vec_.size() - 1) * open_vol_ptile_ / 100.0) - (double)n;
    // n--;
    // std::cout << adjust_ << " " << open_bar_volume_vec_.size() << "percentile size \n";
    std::sort(open_bar_volume_vec_.begin(), open_bar_volume_vec_.end());
    open_vol_avg_ = (1 - adjust_) * open_bar_volume_vec_[n] + adjust_ * open_bar_volume_vec_[n + 1];

    double sum = std::accumulate(cc_vec_.begin(), cc_vec_.end(), 0.0);
    moment_cc_wt_mean_ = sum / cc_vec_.size();
    day_bollinger_ = day_bollinger_ / cc_vec_.size();
    mr_hl_mean_ = mr_hl_mean_ / cc_vec_.size();

    std::vector<double> diff_cc(cc_vec_.size());
    std::transform(cc_vec_.begin(), cc_vec_.end(), diff_cc.begin(),
                   std::bind2nd(std::minus<double>(), moment_cc_wt_mean_));
    double sq_sum = std::inner_product(diff_cc.begin(), diff_cc.end(), diff_cc.begin(), 0.0);
    moment_cc_wt_stdev_ = std::sqrt(sq_sum / (cc_vec_.size() - 1));

    sum = std::accumulate(long_term_volatility_vec_.begin(), long_term_volatility_vec_.end(), 0.0);
    double avg_ = sum / long_term_volatility_vec_.size();
    std::vector<double> longdiff_cc(long_term_volatility_vec_.size());
    std::transform(long_term_volatility_vec_.begin(), long_term_volatility_vec_.end(), longdiff_cc.begin(),
                   std::bind2nd(std::minus<double>(), avg_));
    sq_sum = std::inner_product(longdiff_cc.begin(), longdiff_cc.end(), longdiff_cc.begin(), 0.0);
    // _dbglogger_ << " sum ltvol: " << sum  << " " << sq_sum << " " << long_term_volatility_vec_.size()<<"\n";
    long_term_vol_std_ = std::sqrt(sq_sum / (long_term_volatility_vec_.size()));
    long_term_vol_mean_ = avg_;

    if (obv_long_term_lkbk_ != 0) {
      sum = std::accumulate(obv_vec_.begin(), obv_vec_.end(), 0.0);

      avg_ = sum / obv_vec_.size();
      std::vector<double> longdiff_obv(obv_vec_.size());
      std::transform(obv_vec_.begin(), obv_vec_.end(), longdiff_obv.begin(), std::bind2nd(std::minus<double>(), avg_));
      sq_sum = std::inner_product(longdiff_obv.begin(), longdiff_obv.end(), longdiff_obv.begin(), 0.0);
      // _dbglogger_ << " sum ltvol: " << sum  << " " << sq_sum << " " << long_term_volatility_vec_.size()<<"\n";
      long_term_obv_std_ = std::sqrt(sq_sum / (obv_vec_.size()));
    }

    sum = std::accumulate(bollinger_vec_.begin(), bollinger_vec_.end(), 0.0);
    avg_ = sum / bollinger_vec_.size();
    std::vector<double> longdiff_bollinger(bollinger_vec_.size());
    std::transform(bollinger_vec_.begin(), bollinger_vec_.end(), longdiff_bollinger.begin(),
                   std::bind2nd(std::minus<double>(), avg_));
    sq_sum = std::inner_product(longdiff_bollinger.begin(), longdiff_bollinger.end(), longdiff_bollinger.begin(), 0.0);
    day_bollinger_std_ = std::sqrt(sq_sum / (bollinger_vec_.size() - 1));

    _dbglogger_ << "Values " << ticker_symbol_ << " Mean: " << moment_cc_wt_mean_ << " StdDev: " << moment_cc_wt_stdev_
                << " HLMean: " << mr_hl_mean_ << " Longterm: " << long_term_vol_std_ << " " << long_term_vol_mean_
                << " " << long_term_obv_std_ << " Opening vol avg: " << open_vol_avg_ << "\n";
  }

  void getMACDParams(double& _day_bollinger_, int _num_bars_, int _sema_param_, int _lema_param_, int _signal_param_,
                     double& _sema_, double& _lema_, double& _macd_, double& _signal_, int& _last_crossover_) {
    // _num_bars_ *= 3;
    unsigned int start_gran_index_ = granular_bar_info_.size() - _num_bars_;
    double final_signal_ = 0;
    double sema_nr_ = 0, sema_dr_ = 0, lema_nr_ = 0, lema_dr_ = 0, signal_nr_ = 0, signal_dr_ = 0;
    double sema_factor_ = 1.0 - 2.0 / (_sema_param_ + 1), lema_factor_ = 1.0 - 2.0 / (_lema_param_ + 1),
           signal_factor_ = 1.0 - 2.0 / (_signal_param_ + 1);
    _sema_ = 0;
    _lema_ = 0;
    _macd_ = 0;
    _signal_ = 0;
    // std::cout << _num_bars_ << " " << _sema_param_ << " " << _lema_param_ << " " << _signal_param_ << std::endl;
    // std::cout << sema_factor_ << " " << lema_factor_ << " " << signal_factor_ << std::endl;
    while (start_gran_index_ <= granular_bar_info_.size() - 1) {
      double close_px_ = granular_bar_info_[start_gran_index_].close_;
      sema_nr_ = sema_nr_ * sema_factor_ + close_px_;
      sema_dr_ = sema_dr_ * sema_factor_ + 1;
      _sema_ = sema_nr_ / sema_dr_;

      lema_nr_ = lema_nr_ * lema_factor_ + close_px_;
      lema_dr_ = lema_dr_ * lema_factor_ + 1;
      _lema_ = lema_nr_ / lema_dr_;

      _macd_ = _sema_ - _lema_;

      signal_nr_ = signal_nr_ * signal_factor_ + _macd_;
      signal_dr_ = signal_dr_ * signal_factor_ + 1;
      _signal_ = signal_nr_ / signal_dr_;

      if (final_signal_ * (_macd_ - _signal_) < 0) {
        _last_crossover_ = 0;
      } else {
        _last_crossover_++;
      }
      // std::cout <<granular_bar_info_[start_gran_index_].time_ << " " << close_px_ << " " <<  _sema_ << " " << _lema_
      // << " " << _macd_ << " " << _signal_ << std::endl;
      final_signal_ = _macd_ - _signal_;
      start_gran_index_++;
    }
    _day_bollinger_ = day_bollinger_;
  }

  void getMRParams(double& _day_bollinger_std_, double& _mr_hl_mean_) {
    _day_bollinger_std_ = day_bollinger_std_;
    _mr_hl_mean_ = mr_hl_mean_;
  }
  void getGapRevertValues(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_, const std::string& ticker_symbol_, int date_,
       int num_days_, int granularity_, double& adj_ratio_,int long_term_lkbk_,double& long_term_vol_mean_, double& prev_day_close_, double& open_obv_max_){//, double& historic_theok_) {

    GetBarData(_dbglogger_, _watch_, dep_shc_, ticker_symbol_, date_, long_term_lkbk_ + num_days_, granularity_, adj_ratio_);
    int num_holidays_in_lkbk_ = GetNumHolidays(dep_shc_, date_, long_term_lkbk_);
    int num_holidays_in_obv_lkbk_ = GetNumHolidays(dep_shc_, date_, num_days_*2);
    std::vector<double>  long_term_volatility_vec_;
    // std::vector<double> cc_vec_, long_term_volatility_vec_, obv_vec_, bollinger_vec_;
    int day_bar_index_ = day_bar_info_.size() - 1;
    int day_index_ = 0;
    int gran_bar_index_ = granular_bar_info_.size() - 1;
    int open_vol_count_ = 0;
    double next_day_hl_ = 0;
    double next_day_close_ = 0;
    prev_day_close_ = 0;
    std::vector<double> open_obv_vec_;
    // std::cout << "BARDATA accumulate\n";
    // for(int i=0;i<=gran_bar_index_;i++){
    //  std::cout << " GRAN : " << granular_bar_info_[i].time_ << " " << granular_bar_info_[i].vol_ <<std::endl;
    // }
    // std::vector<double> open_bar_volume_vec_;

    int prev_day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[0], 915, "IST_915");

    while (day_bar_index_ >= 0 && gran_bar_index_ >= 0) {
      int day_bar_index_start_ = day_bar_info_[day_bar_index_].time_;
      int day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
      // std::cout <<day_start_ << " " <<  day_bar_index_ << " " << (int)date_list_.size()-1 <<std::endl;
      while (granular_bar_info_[gran_bar_index_].time_ > day_start_) {
        gran_bar_index_--;
      }
      if (day_index_ <= long_term_lkbk_ - num_holidays_in_lkbk_ ) {
        // std::cout << "prev lookback " << day_bar_index_start_ << " " << long_term_lkbk_start_<<std::endl;
        if (day_bar_info_[day_bar_index_].time_ >= day_start_) {
          if (next_day_close_ && next_day_hl_) {
            
            long_term_volatility_vec_.push_back(sqrt(abs(next_day_close_ - day_bar_info_[day_bar_index_].close_) *next_day_hl_)) ;
          }
          next_day_close_ = day_bar_info_[day_bar_index_].close_;
          next_day_hl_ = day_bar_info_[day_bar_index_].high_ - day_bar_info_[day_bar_index_].low_;
        }
      }
      // std::cout << day_bar_index_start_ << " " << day_bar_info_[day_bar_index_].close_ << std::endl;
      // std::cout << gran_bar_index_ << " " << granular_bar_info_[gran_bar_index_].time_ << " " << granular_bar_info_[gran_bar_index_].vol_ <<"\n";
      // int day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_bar_index_], 1530, "IST_1530");
      if (gran_bar_index_ >= 0 && granular_bar_info_[gran_bar_index_].time_ == day_start_ &&
          granular_bar_info_[gran_bar_index_].vol_) {
        // open_vol_avg_+= granular_bar_info_[gran_bar_index_].vol_;
        if(open_vol_count_ < num_days_){
          if(granular_bar_info_[gran_bar_index_].close_ > granular_bar_info_[gran_bar_index_].open_){
            open_obv_vec_.push_back((granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_)*granular_bar_info_[gran_bar_index_].vol_);
          }
          else{
           open_obv_vec_.push_back(-1*(granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_)*granular_bar_info_[gran_bar_index_].vol_); 
          }
          open_vol_count_++;
        }
      } else {
        gran_bar_index_++;
        if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
          // std::cout << day_bar_index_ << " " << day_bar_index_start_ << " " << prev_day_start_ << "\n";
          _dbglogger_ << day_bar_index_start_ << " ERROR : Opening volume not found for previous day"
                      << "\n";
          _dbglogger_.DumpCurrentBuffer();
        }
      }

      // int long_term_lkbk_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[long_term_lkbk_], 915, "IST_915");
      // if(day_bar_index_ >= (int)date_list_.size()-1-long_term_lkbk_) {
      if (day_index_ <= num_days_ - num_holidays_in_obv_lkbk_ && !prev_day_close_) {
        // std::cout << "prev lookback " << day_bar_index_start_ << " " << long_term_lkbk_start_<<std::endl;
        if (day_bar_info_[day_bar_index_].time_ >= day_start_) {
          prev_day_close_ = day_bar_info_[day_bar_index_].close_;
        } else {
          // next_day_close_ = 0;
          if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
            _dbglogger_ << "ERROR : Closing price not found for previous day"
                        << "\n";
            _dbglogger_.DumpCurrentBuffer();
            return;
          }
          day_bar_index_++;
        }
      }

      day_bar_index_--;
      gran_bar_index_--;
      day_index_++;
    }
    open_obv_max_ = *std::max_element(open_obv_vec_.begin(),open_obv_vec_.end());
    // _dbglogger_ << "open move : "<< open_move_max_ << " prev_close " << prev_day_close_<< "\n";
    double sum = std::accumulate(long_term_volatility_vec_.begin(), long_term_volatility_vec_.end(), 0.0);
    long_term_vol_mean_ = sum / long_term_volatility_vec_.size();
  }

  void getGapValues(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_, const std::string& ticker_symbol_, int date_,
       int num_days_, int granularity_, double& adj_ratio_, double& prev_day_close_, double& open_move_max_){//, double& historic_theok_) {

    GetBarData(_dbglogger_, _watch_, dep_shc_, ticker_symbol_, date_, num_days_*2, granularity_, adj_ratio_);
    int num_holidays_in_lkbk_ = GetNumHolidays(dep_shc_, date_, num_days_*2);

    // std::vector<double> cc_vec_, long_term_volatility_vec_, obv_vec_, bollinger_vec_;
    int day_bar_index_ = day_bar_info_.size() - 1;
    int day_index_ = 0;
    int gran_bar_index_ = granular_bar_info_.size() - 1;
    int open_vol_count_ = 0;
    prev_day_close_ = 0;
    std::vector<double> open_move_vec_;
    // std::cout << "BARDATA accumulate\n";
    // for(int i=0;i<=gran_bar_index_;i++){
    //  std::cout << " GRAN : " << granular_bar_info_[i].time_ << " " << granular_bar_info_[i].vol_ <<std::endl;
    // }
    // std::vector<double> open_bar_volume_vec_;

    int prev_day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[0], 915, "IST_915");

    while (day_bar_index_ >= 0 && gran_bar_index_ >= 0) {
      int day_bar_index_start_ = day_bar_info_[day_bar_index_].time_;
      int day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
      // std::cout <<day_start_ << " " <<  day_bar_index_ << " " << (int)date_list_.size()-1 <<std::endl;
      while (granular_bar_info_[gran_bar_index_].time_ > day_start_) {
        gran_bar_index_--;
      }
      // std::cout << day_bar_index_start_ << " " << day_bar_info_[day_bar_index_].close_ << std::endl;
      // std::cout << gran_bar_index_ << " " << granular_bar_info_[gran_bar_index_].time_ << " " << granular_bar_info_[gran_bar_index_].vol_ <<"\n";
      // int day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_bar_index_], 1530, "IST_1530");
      if (gran_bar_index_ >= 0 && granular_bar_info_[gran_bar_index_].time_ == day_start_ &&
          granular_bar_info_[gran_bar_index_].vol_) {
        // open_vol_avg_+= granular_bar_info_[gran_bar_index_].vol_;
        if(open_vol_count_ < num_days_){
          if(granular_bar_info_[gran_bar_index_].close_ > granular_bar_info_[gran_bar_index_].open_){
            open_move_vec_.push_back((granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_)/granular_bar_info_[gran_bar_index_].open_);
          }
          else{
           open_move_vec_.push_back(-1*(granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_)/granular_bar_info_[gran_bar_index_].open_); 
          }
          open_vol_count_++;
        }
      } else {
        gran_bar_index_++;
        if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
          // std::cout << day_bar_index_ << " " << day_bar_index_start_ << " " << prev_day_start_ << "\n";
          _dbglogger_ << day_bar_index_start_ << " ERROR : Opening volume not found for previous day"
                      << "\n";
          _dbglogger_.DumpCurrentBuffer();
        }
      }

      // int long_term_lkbk_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[long_term_lkbk_], 915, "IST_915");
      // if(day_bar_index_ >= (int)date_list_.size()-1-long_term_lkbk_) {
      if (day_index_ <= num_days_ - num_holidays_in_lkbk_ && !prev_day_close_) {
        // std::cout << "prev lookback " << day_bar_index_start_ << " " << long_term_lkbk_start_<<std::endl;
        if (day_bar_info_[day_bar_index_].time_ >= day_start_) {
          prev_day_close_ = day_bar_info_[day_bar_index_].close_;
        } else {
          // next_day_close_ = 0;
          if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
            _dbglogger_ << "ERROR : Closing price not found for previous day"
                        << "\n";
            _dbglogger_.DumpCurrentBuffer();
            return;
          }
          day_bar_index_++;
        }
      }

      day_bar_index_--;
      gran_bar_index_--;
      day_index_++;
    }
    open_move_max_ = *std::max_element(open_move_vec_.begin(),open_move_vec_.end());
    // _dbglogger_ << "open move : "<< open_move_max_ << " prev_close " << prev_day_close_<< "\n";
  }

  void getPriceReturns(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_, const std::string& ticker_symbol_, int date_,
       int num_days_, int granularity_, double& adj_ratio_, std::vector<double>& return_vec_, double& prev_day_close_, double& mean_volume_){//, double& historic_theok_) {

    GetBarData(_dbglogger_, _watch_, dep_shc_, ticker_symbol_, date_, num_days_, granularity_, adj_ratio_);

    int day_index_ = 0;
    // int current_obv_ = 0;
    int gran_bar_index_ = granular_bar_info_.size() - 1;
    mean_volume_ = 0;
    // std::cout << "BARDATA accumulate\n";
    // for(int i=0;i<=gran_bar_index_;i++){
    //  std::cout << " GRAN : " << granular_bar_info_[i].time_ << " " << granular_bar_info_[i].vol_ <<std::endl;
    // }

    // int prev_day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[0], 1500, "IST_1500");
    while((unsigned int)day_index_ < date_list_.size()){
      int day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
      int corr_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 1500, "IST_1500");
      int day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 1530, "IST_1530");
      int curr_gran_ = day_end_ - granularity_;

      while (gran_bar_index_ >= 0) {
        // std::cout << granular_bar_info_[gran_bar_index_].time_ << " " << granular_bar_info_[gran_bar_index_].open_ << " " 
        // << granular_bar_info_[gran_bar_index_].close_ << " " << curr_gran_ << " " << corr_start_ << std::endl;    
        while(granular_bar_info_[gran_bar_index_].time_< curr_gran_ && curr_gran_ >= corr_start_){
          return_vec_.push_back(0);
          // std::cout << " empty\n";
          curr_gran_ -= granularity_;
        }
        if(granular_bar_info_[gran_bar_index_].time_ >= corr_start_){
          double return_ = (granular_bar_info_[gran_bar_index_].close_ - granular_bar_info_[gran_bar_index_].open_)/granular_bar_info_[gran_bar_index_].open_;
          if(!prev_day_close_){
            prev_day_close_ = granular_bar_info_[gran_bar_index_].close_ ;
          }
          return_vec_.push_back(return_);
          mean_volume_ += granular_bar_info_[gran_bar_index_].vol_;
          curr_gran_ -= granularity_;
          // std::cout << ticker_symbol_ << " " << return_vec_.size()<<std::endl;
        }
        else{
          if(granular_bar_info_[gran_bar_index_].time_ < day_start_){
            break;
          }
        }
        gran_bar_index_--;

      }
      day_index_++;
      // std::cout << day_start_ << " " << date_list_.size() << std::endl;
    }
    mean_volume_ /= num_days_;
    // std::cout << ticker_symbol_ << " " << return_vec_.size()<<std::endl;
    // historic_theok_ = 0;
    // std::string theok_filename_ = std::string(THEOKFOLDER) + "/" + dep_shc_ ;
    // std::ifstream theok_file_;
    // theok_file_.open(theok_filename_.c_str(), std::ifstream::in);
    // char line_[1024];
    // if (!theok_file_.is_open()) {
    //   _dbglogger_ << _watch_.tv() << " Could not open file " << theok_filename_ << "\n";
    //   // return historic_theok_;
    // }
    // else{
    //   int min_date_ = GetPrevDayforSHC(dep_shc_, date_, 1);

    //   while ((theok_file_.good())) {
    //     memset(line_, 0, sizeof(line_));
    //     theok_file_.getline(line_, sizeof(line_));

    //     HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    //     const std::vector<const char*>& tokens_ = t_tokenizer_.GetTokens();

    //     if (tokens_.size() >= 2) {
    //       if (tokens_[0][0] == '#') {
    //         continue;
    //       }
    //     } else {
    //       continue;
    //     }

    //     char* p;
    //     int t_date_ = strtol(tokens_[0], &p, 10);
    //     char* q;
    //     double ratio_ = strtod(tokens_[1], &q);

    //     // Checking whether first token is int and second is double
    //     if ((*p) || (*q)) {
    //       continue;
    //     }

    //     // Assumption is that ratio is stored sorted by date in descending order
    //     if (t_date_ >= date_) {
    //       continue;
    //     } else if (t_date_ < min_date_) {
    //       break;
    //     }
    //     else{
    //       // historic_theok_ = ratio_;
    //       historic_theok_ = 0;
    //     }
      // }
    // }
  }   

  void getMAValues(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, const std::string& dep_shc_,
                     const std::string& ticker_symbol_, int date_, std::vector<unsigned int> day_ma_lkbk_, std::vector<double>& day_moving_avg_,
                     int granularity_, int long_term_lkbk_, double& long_term_vol_std_,
                     double& prev_day_close_, double& adj_ratio_) {
    //dbglogger_, watch_, secondary_smv_->shortcode(), ticker_name_, watch_.YYYYMMDD(),day_ma_lkbk_,day_moving_avg_,granularity_,adj_ratio_,prev_day_close_);
    GetBarData(_dbglogger_, _watch_, dep_shc_, ticker_symbol_, date_, 2*long_term_lkbk_ + day_ma_lkbk_[day_ma_lkbk_.size()-1], granularity_, adj_ratio_);
    int num_holidays_in_lkbk_ = GetNumHolidays(dep_shc_, date_, long_term_lkbk_ );
    // int bigger_holiday_lkbk_= GetNumHolidays(dep_shc_, date_, long_term_lkbk_ +day_ma_lkbk_[day_ma_lkbk_.size()-1]);
    std::vector<double> long_term_volatility_vec_;
    std::vector<double> rolling_day_ma_;
    int day_bar_index_ = day_bar_info_.size() - 1;
    int day_index_ = 0;
    int gran_bar_index_ = granular_bar_info_.size() - 1;
    prev_day_close_ = 0;
    // std::cout << "BARDATA accumulate\n";
    // for(int i=0;i<=gran_bar_index_;i++){
    //  std::cout << " GRAN : " << granular_bar_info_[i].time_ << " " << granular_bar_info_[i].vol_ <<std::endl;
    // }
    std::vector<double> open_bar_volume_vec_;

    int prev_day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[0], 915, "IST_915");

    for(unsigned int i=0;i<day_ma_lkbk_.size();i++){
      rolling_day_ma_.push_back(0);
    }

    for (unsigned int day_i_ = day_bar_info_.size() - 1;day_i_>=day_bar_info_.size() - day_ma_lkbk_[day_ma_lkbk_.size()-1]-1;day_i_--){
      for(unsigned int i=0;i<day_ma_lkbk_.size();i++){
        if(day_i_ >= day_bar_info_.size() - day_ma_lkbk_[i]){
          rolling_day_ma_[i] += day_bar_info_[day_i_].close_;
          day_moving_avg_[i] += day_bar_info_[day_i_].close_;
        }
        else if(day_i_ == day_bar_info_.size() - day_ma_lkbk_[i]-1){
          rolling_day_ma_[i] /= day_ma_lkbk_[i];
          day_moving_avg_[i] /= day_ma_lkbk_[i];
        }
      }
    }

    while (day_bar_index_ >= 0 && gran_bar_index_ >= 0) {
      int day_bar_index_start_ = day_bar_info_[day_bar_index_].time_;
      int day_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_index_], 915, "IST_915");
      // std::cout <<"close " << day_start_ << " " <<  day_bar_index_ << " " << num_holidays_in_lkbk_  << " " <<day_bar_info_[day_bar_index_].close_  <<std::endl;
      // std::cout << day_bar_index_start_ << " " << day_bar_info_[day_bar_index_].close_ << std::endl;
      // int day_end_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[day_bar_index_], 1530, "IST_1530");
      while (granular_bar_info_[gran_bar_index_].time_ > day_start_) {
        
        gran_bar_index_--;
      }
      if (gran_bar_index_ >= 0 && granular_bar_info_[gran_bar_index_].time_ == day_start_ &&
          granular_bar_info_[gran_bar_index_].vol_) {
        
        
      } else {
        gran_bar_index_++;
        if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
          _dbglogger_ << day_bar_index_start_ << " ERROR : Opening volume not found for previous day"
                      << "\n";
          _dbglogger_.DumpCurrentBuffer();
        }
      }

      // int long_term_lkbk_start_ = HFSAT::DateTime::GetTimeFromTZHHMM(date_list_[long_term_lkbk_], 915, "IST_915");
      // if(day_bar_index_ >= (int)date_list_.size()-1-long_term_lkbk_) {
      if (day_index_ < long_term_lkbk_ - num_holidays_in_lkbk_ && day_bar_index_-day_ma_lkbk_[day_ma_lkbk_.size()-1] >=0) {
        // std::cout << "prev lookback " << day_bar_index_start_ << " " << long_term_lkbk_start_<<std::endl;
        if (day_bar_info_[day_bar_index_].time_ >= day_start_) {
          double ma_diff_ = *std::max_element(rolling_day_ma_.begin(),rolling_day_ma_.end()) - *std::min_element(rolling_day_ma_.begin(),rolling_day_ma_.end());
          long_term_volatility_vec_.push_back(ma_diff_*ma_diff_*day_bar_info_[day_bar_index_].vol_/(day_bar_info_[day_bar_index_].high_ - day_bar_info_[day_bar_index_].low_ + 0.0000005));
          for(unsigned int i=0;i<day_ma_lkbk_.size();i++){
          //   std::cout << "roll " << day_ma_lkbk_[i] << " " << rolling_day_ma_[i] << " " << day_bar_info_[day_bar_index_].close_ << " " <<  day_bar_info_[day_bar_index_-day_ma_lkbk_[i]].close_<<"\n";
            rolling_day_ma_[i] -= (day_bar_info_[day_bar_index_].close_ - day_bar_info_[day_bar_index_-day_ma_lkbk_[i]].close_)/day_ma_lkbk_[i];
          }
          // _dbglogger_ << "vol vec: " << long_term_volatility_vec_[long_term_volatility_vec_.size()-1] << " "<< day_bar_info_[day_bar_index_].vol_  
          // << " " << ma_diff_*ma_diff_ << " " << day_bar_info_[day_bar_index_].high_ - day_bar_info_[day_bar_index_].low_ + 0.0000005 << "\n";

            // _dbglogger_ << day_bar_info_[day_bar_index_].time_ << " " << obv_vec_[obv_vec_.size()-1] << "\n";
            // ";

          if (!prev_day_close_) {
            prev_day_close_ = day_bar_info_[day_bar_index_].close_;
          }
        } else {
          // next_day_close_ = 0;
          if (day_bar_index_ == (int)day_bar_info_.size() - 1 && day_bar_index_start_ <= prev_day_start_) {
            _dbglogger_ << "ERROR : Closing price not found for previous day"
                        << "\n";
            _dbglogger_.DumpCurrentBuffer();
            return;
          }
          day_bar_index_++;
        }
      }

      day_bar_index_--;
      gran_bar_index_--;
      day_index_++;
    }

    double sum = std::accumulate(long_term_volatility_vec_.begin(), long_term_volatility_vec_.end(), 0.0);
    double avg_ = sum / long_term_volatility_vec_.size();
    std::vector<double> longdiff_cc(long_term_volatility_vec_.size());
    std::transform(long_term_volatility_vec_.begin(), long_term_volatility_vec_.end(), longdiff_cc.begin(),
                   std::bind2nd(std::minus<double>(), avg_));
    double sq_sum = std::inner_product(longdiff_cc.begin(), longdiff_cc.end(), longdiff_cc.begin(), 0.0);
    // _dbglogger_ << " sum ltvol: " << sum  << " " << sq_sum << " " << long_term_volatility_vec_.size()<<"\n";
    long_term_vol_std_ = std::sqrt(sq_sum / (long_term_volatility_vec_.size()));
    // long_term_vol_mean_ = avg_;

    
    

    _dbglogger_ << "Values " << ticker_symbol_ << " Longterm: " << long_term_vol_std_  << " MA: ";
    for(unsigned int i=0;i<day_ma_lkbk_.size();i++){
      _dbglogger_ << day_moving_avg_[i] << " ";
    }
    _dbglogger_ << "\n";
  }
};

#endif  // _BAR_GENERATOR_H
