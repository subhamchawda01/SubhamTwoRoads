/**
   \file FuturesUtils/DI1Utils.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef BASE_FUTURESUTILS_DI1UTILS_HPP
#define BASE_FUTURESUTILS_DI1UTILS_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>  //atoi
//#include <iomanip>  //setfill

#include <map>
#include <vector>

#include <math.h>  // pow
#include <boost/date_time/gregorian/gregorian.hpp>

#include "dvccode/CDef/defines.hpp"

#define DAY_COUNT 252

namespace HFSAT {

class DI1Utils {
 public:
  DI1Utils(int, std::string, std::string, std::string);

  // static functions
  static inline std::string _generic_to_specific_(int _input_date_, std::string _shc_) { return _shc_; }

  static inline std::string _specific_to_generic_(int _input_date_, std::string _shc_) { return std::string(""); }

  static inline int _get_term_(int _input_date_, std::string _shc_) {
    if (_shc_.find("DI1") != std::string::npos)  // BR_DI_1 we have a generic_ticker
    {
      _shc_ = _generic_to_specific_(_input_date_, _shc_);
    }

    if (_shc_.find("DI1") != std::string::npos)  // DI1F14 we have a specific_ticker
    {
      char _ltd_date_[8] = {0};

      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      const boost::gregorian::date_duration one_day_date_duration(1);

      int ltd_mm = ExchMonthCode.find(_shc_[3]);
      ltd_mm++;
      int ltd_yy = atoi(_shc_.substr(4).c_str());

      if (sprintf(_ltd_date_, "20%02d%02d01", ltd_yy, ltd_mm) > 0) {
        std::stringstream ss;
        ss << _input_date_;

        boost::gregorian::date sdate_ =
            boost::gregorian::from_undelimited_string(ss.str());  // my compiler  doesnt know to_string ?
        boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(_ltd_date_);

        // TODO store holidays in a file just in file and read to a map
        while ((edate_.day_of_week() == boost::gregorian::Saturday) ||
               (edate_.day_of_week() == boost::gregorian::Sunday)) {
          edate_ = edate_ + one_day_date_duration;
        }

        return ((edate_ - sdate_).days());
      }
    }

    return -1;
  }

  static inline int _get_reserves_(int _input_date_, std::string _shc_) {
    int reserves_ = 0;
    if (_shc_.find("DI1") != std::string::npos)  // BR_DI_1 we have a generic_ticker
    {
      _shc_ = _generic_to_specific_(_input_date_, _shc_);
    }
    if (_shc_.find("DI1") != std::string::npos)  // DI1F14 we have a specific_ticker
    {
      char _ltd_date_[8] = {0};

      const std::string ExchMonthCode("FGHJKMNQUVXZ");
      const boost::gregorian::date_duration one_day_date_duration(1);

      int ltd_mm = ExchMonthCode.find(_shc_[3]);
      ltd_mm++;
      int ltd_yy = atoi(_shc_.substr(4).c_str());

      if (sprintf(_ltd_date_, "20%02d%02d01", ltd_yy, ltd_mm) > 0) {
        std::stringstream ss;
        ss << _input_date_;

        boost::gregorian::date sdate_ =
            boost::gregorian::from_undelimited_string(ss.str());  // my compiler  doesnt know to_string ?
        boost::gregorian::date edate_ = boost::gregorian::from_undelimited_string(_ltd_date_);

        // TODO store holidays in a file just in file and read to a map
        while ((edate_.day_of_week() == boost::gregorian::Saturday) ||
               (edate_.day_of_week() == boost::gregorian::Sunday)) {
          edate_ = edate_ - one_day_date_duration;
        }
        while (sdate_ != edate_) {
          if ((edate_.day_of_week() == boost::gregorian::Saturday) ||
              (edate_.day_of_week() == boost::gregorian::Sunday)) {
          } else {
            reserves_++;
          }
          sdate_ = sdate_ + one_day_date_duration;
        }
        return reserves_;
      }
    }
    return -1;
  }

  static inline double _get_dollar_value_(int _input_date_, std::string _shc_, double price_) {
    double unit_price_ = 0;
    double term_ = double(_get_reserves_(_input_date_, _shc_)) / DAY_COUNT;

    if (term_ > 0.000) {
      unit_price_ = 100000 / std::pow((price_ / 100 + 1), term_) / price_;
    }
    return unit_price_;
  }

  static inline double _linear_interpolate_(int _x_, int _x1_, int _x2_, double _y1_, double _y2_) {
    double _y_ = 0;

    if (_x2_ == _x1_) {
      _y_ = (_y2_ + _y1_) / 2;
    } else {
      double _slope_ = (_y2_ - _y1_) / (_x2_ - _x1_);
      _y_ = _y1_ + (_x_ - _x1_) * _slope_;
    }
    return _y_;
  }

  static inline double _linear_interpolate_using_estimate_(int _x_, double _slope_, int _x1_, double _y1_) {
    double _y_ = 0;
    _y_ = _y1_ + (_x_ - _x1_) * _slope_;

    return _y_;
  }

  static inline double _estimate_slope_factor_(std::vector<double> px1_, std::vector<double> px2_) {
    double sum_ = 0;
    double pdiff_ = 0;
    double cdiff_ = 0;

    if (px1_.size() == px2_.size() && px1_.size() > 1) {
      pdiff_ = px2_[0] - px1_[0];

      for (unsigned int i = 1; i < px1_.size(); i++) {
        cdiff_ = px2_[i] - px1_[i];
        sum_ += cdiff_ - pdiff_;
        pdiff_ = cdiff_;
      }
      return (sum_ / (px1_.size() - 1) * DAY_COUNT * 100);
    }
    return -1;
  }

  inline void _reset_signal_() { k_ = 1000000; }

  // from an object
  double _get_target_price_(double, double);
  double _get_signal_(double, double);

 private:
  // defines an object

  int input_date_;
  std::string shc1_;
  std::string shc2_;
  std::string shc_;

  double k_;

  int term1_;
  int term2_;
  int term_;
};
}

#endif
