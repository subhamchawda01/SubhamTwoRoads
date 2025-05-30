/*
 * online_lr_weights.hpp
 *
 *  Created on: 16-Sep-2015
 *      Author: raghuram
 */

#ifndef MODELMATH_ONLINE_LR_WEIGHTS_HPP_
#define MODELMATH_ONLINE_LR_WEIGHTS_HPP_

#include <sys/time.h>
#include <stdlib.h>
#include <functional>
#include <numeric>
#include <fstream>
#include <iostream>
#include <cstdio>

#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvctrade/kalman/ekfilter.hpp"


/*
It inherits EKFilter class
for Documentation refer to
"http://kalman.sourceforge.net/doc/index.html"
This class listens to Indicator update and dumps this data for every sec and creates reg data .
this reg data is used to update weights and compute sum_vars
*/

namespace Kalman {

class Online_LR_Weights : public Kalman::EKFilter<double, 1>, public HFSAT::TimePeriodListener {
 public:
  Online_LR_Weights(int _num_indicators_, std::vector<double> _init_weights_, std::vector<double> _init_weights_var_,
                    std::vector<double> _indicator_var_, double _observation_var_, int _pred_dur_,
                    int _weights_update_dur_, int _num_samples_per_update_, const HFSAT::Watch& _watch_,
                    double _thres_factor_, std::string _model_filename_);

  Online_LR_Weights() { num_indicators_ = 0; }
  ~Online_LR_Weights() { eod_work(); }
  void update_indicator(double _new_val_, int _index_);
  void add_indicator_params(double _init_weight_, double _init_weight_var_, double _indicator_var_, int _index_);
  void initialize_filter();

  // protected:
  void makeA();
  void makeH();
  void makeV();
  void makeR();
  void makeW();
  void makeQ();
  void makeProcess();
  void makeMeasure();

  void update_weights();
  void update_pred_val();

  void create_reg_data();
  void dump_indicators();
  void periodic_weights_update();

  void OnTimePeriodUpdate(const int num_pages_);

  std::vector<double>& get_current_weights() { return (current_weights_); }
  double get_pred_val() {
    update_pred_val();
    return (pred_val_);
  }
  void read_from_over_night_file();
  void eod_work();

  int num_indicators_ = 0;
  std::vector<bool> ready_vec_;
  unsigned int num_ready_ = 0;
  double observation_var_ = 0;
  std::vector<double> indicator_var_vec_;
  std::vector<double> indicator_val_vec_;
  std::vector<double> init_weights_;
  std::vector<double> current_weights_;
  std::vector<double> current_weights_var_;
  double observed_val_ = 0;
  double pred_val_ = 0;
  int pred_dur_ = 0;
  int weights_update_dur_ = 0;
  int num_samples_per_update_ = 0;
  int num_time_period_updates_ = 0;
  std::string over_night_file_name_;

  bool any_update_ = false;
  Matrix init_covar_;
  std::vector<double> curr_vals_;
  const HFSAT::Watch* watch_;
  double thres_factor_;
  static int id;
  std::string model_filename_;

  std::vector<std::vector<double> > raw_data_matrix_;
  std::vector<std::vector<double> > bucketed_data_matrix_;
  std::vector<std::vector<double> > reg_data_matrix_;
};

int Online_LR_Weights::id = 0;
Online_LR_Weights::Online_LR_Weights(int _num_indicators_, std::vector<double> _init_weights_,
                                     std::vector<double> _init_weights_var_, std::vector<double> _indicator_var_,
                                     double _observation_var_, int _pred_dur_, int _weights_update_dur_,
                                     int _num_samples_per_update_, const HFSAT::Watch& _watch_, double _thres_factor_,
                                     std::string _model_filename_)
    : num_indicators_(_num_indicators_),
      ready_vec_(_init_weights_.size() + 1, false),
      observation_var_(_observation_var_),
      indicator_var_vec_(_indicator_var_.begin(), _indicator_var_.end()),
      indicator_val_vec_(_init_weights_.size() + 1, 0),
      init_weights_(_init_weights_.begin(), _init_weights_.end()),
      current_weights_(_init_weights_.begin(), _init_weights_.end()),
      current_weights_var_(_init_weights_var_.begin(), _init_weights_var_.end()),
      observed_val_(0),
      pred_dur_(_pred_dur_),
      weights_update_dur_(_weights_update_dur_),
      num_samples_per_update_(_num_samples_per_update_),
      init_covar_(num_indicators_, num_indicators_, 0.0),
      curr_vals_(_init_weights_.size() + 1, 0),
      watch_(&_watch_),
      thres_factor_(_thres_factor_),
      model_filename_(_model_filename_),
      raw_data_matrix_(),
      bucketed_data_matrix_(),
      reg_data_matrix_() {
  // unsigned long int t1 = std::time(NULL);

  // over_night_file_name_ = over_night_file_name_ + s1 + s2;

  // read_from_over_night_file();
  setDim(num_indicators_, 1, num_indicators_, 1, 1);
  Vector temp_x(num_indicators_);

  for (unsigned int i = 1; i <= _init_weights_var_.size(); i++) {
    temp_x(i) = _init_weights_[i - 1];
  }

  Matrix init_covar(_init_weights_var_.size(), _init_weights_var_.size());

  for (unsigned int i = 1; i <= _init_weights_var_.size(); i++) {
    for (unsigned int j = 1; j <= _init_weights_var_.size(); j++) {
      init_covar(i, j) = 0;
      if (i == j) {
        init_covar(i, j) = current_weights_var_[i - 1];
      }
    }
  }

  this->init(temp_x, init_covar);
  Online_LR_Weights::id++;
}

void Online_LR_Weights::eod_work() {
  /*
      Matrix p = calculateP();

   std::vector<double> temp_var_vec;


      std::fstream over_night_file_;
      over_night_file_.open(over_night_file_name_, std::ios::in | std::ios::out | std::ios::trunc);
      over_night_file_.close();

       over_night_file_.open(over_night_file_name_, std::ios::in | std::ios::out | std::ios::app);

      std::copy(current_weights_.begin(), current_weights_.end(), std::ostream_iterator<double>(over_night_file_, " "));
      over_night_file_ << std::endl;

    for( int j = 0; j < current_weights_.size(); j++ ){
     temp_var_vec.clear();
     for( int i = 0; i < current_weights_.size(); i++ ){
        temp_var_vec.push_back( p(j+1, i+1));
     }
      std::copy(temp_var_vec.begin(), temp_var_vec.end(), std::ostream_iterator<double>(over_night_file_, " "));
     over_night_file_ << std::endl;
  }
   over_night_file_.close();
  */
}

void Online_LR_Weights::read_from_over_night_file() {
  /*
   std::fstream over_night_file_;
    over_night_file_.open(over_night_file_name_, std::ios::in);

    std::string l1 = "abc";

      std::getline(over_night_file_, l1);
      std::istringstream iss_1(l1);
      std::string token ="";
      char delimiter =  ' ';
      double val = 0;
      int ind = 0;
      while( std::getline(iss_1,token,delimiter ) ){
       val = atof(token.c_str() );
       current_weights_[ind] = val;
       ind++;
      }

  for( int i = 0; i < current_weights_.size(); i++ ){
    std::getline(over_night_file_, l1);
      std::istringstream iss_2(l1);
      token ="";
      val = 0;
      ind = 0;
      while( std::getline(iss_2,token,delimiter ) ) {
       val = atof(token.c_str() );
       init_covar_( i+1, ind+1 ) = val;
       ind++;
      }
    }
  over_night_file_.close();
  */
}

void Online_LR_Weights::dump_indicators() {
  if (num_ready_ == indicator_val_vec_.size()) {
    raw_data_matrix_.push_back(indicator_val_vec_);
    num_time_period_updates_++;
  }
}

void Online_LR_Weights::create_reg_data() {
  reg_data_matrix_.erase(reg_data_matrix_.begin(), reg_data_matrix_.end());
  double p1 = 0;
  double p2 = 0;

  std::vector<double> temp_vec(indicator_val_vec_.size(), 0);
  std::vector<std::vector<double> >::iterator iter_1 = raw_data_matrix_.begin();
  std::vector<std::vector<double> >::iterator iter_2 = iter_1 + pred_dur_;

  for (auto i = 0u; i < raw_data_matrix_.size() - pred_dur_; i++) {
    p1 = (*(iter_1))[0];
    p2 = (*(iter_2))[0];
    temp_vec[0] = p2 - p1;
    std::copy((*iter_1).begin() + 1, (*iter_1).end(), temp_vec.begin() + 1);
    reg_data_matrix_.push_back(temp_vec);
    iter_1++;
    iter_2++;
  }
  raw_data_matrix_.erase(raw_data_matrix_.begin(), raw_data_matrix_.end());
}

void Online_LR_Weights::makeProcess() {}

void Online_LR_Weights::makeMeasure() {
  z(1) = std::inner_product(curr_vals_.begin() + 1, curr_vals_.end(), current_weights_.begin(), 0.0);
}

void Online_LR_Weights::makeA() {
  for (int i = 1; i <= num_indicators_; i++) {
    for (int j = 1; j <= num_indicators_; j++) {
      A(i, j) = 0;
      if (i == j) {
        A(i, j) = 1;
      }
    }
  }
}

void Online_LR_Weights::makeW() {
  for (int i = 1; i <= num_indicators_; i++) {
    for (int j = 1; j <= num_indicators_; j++) {
      W(i, j) = 0;
      if (i == j) {
        W(i, j) = 1;
      }
    }
  }
}

void Online_LR_Weights::makeQ() {
  for (int i = 1; i <= num_indicators_; i++) {
    for (int j = 1; j <= num_indicators_; j++) {
      Q(i, j) = 0;
      if (i == j) {
        Q(i, j) = indicator_var_vec_[i - 1];
      }
    }
  }
}

void Online_LR_Weights::makeH() {
  for (int i = 1; i <= num_indicators_; i++) {
    H(1, i) = curr_vals_[i];
  }
}

void Online_LR_Weights::makeV() { V(1, 1) = 1; }

void Online_LR_Weights::makeR() { R(1, 1) = observation_var_; }

void Online_LR_Weights::update_indicator(double _new_val_, int _index_) {
  indicator_val_vec_[_index_] = _new_val_;
  any_update_ = true;
  if (!ready_vec_[_index_]) {
    num_ready_++;
    ready_vec_[_index_] = true;
  }
}

void Online_LR_Weights::update_pred_val() {
  pred_val_ =
      std::inner_product(indicator_val_vec_.begin() + 1, indicator_val_vec_.end(), current_weights_.begin(), 0.0);
}
void Online_LR_Weights::update_weights() {
  Vector temp_z(1);
  temp_z(1) = curr_vals_[0];
  Vector temp_u(1, 0.0);

  this->step(temp_u, temp_z);
  double temp_val = 0;
  for (int i = 1; i <= num_indicators_; i++) {
    temp_val = std::min(init_weights_[i - 1] + (thres_factor_ * std::abs(init_weights_[i - 1])), x(i));
    temp_val = std::max(init_weights_[i - 1] - (thres_factor_ * std::abs(init_weights_[i - 1])), temp_val);
    current_weights_[i - 1] = temp_val;
  }
}

void Online_LR_Weights::periodic_weights_update() {
  create_reg_data();
  int num_lines = 1;
  std::fill(curr_vals_.begin(), curr_vals_.end(), 0.0);

  for (auto i = 0u; i < reg_data_matrix_.size(); i++) {
    std::transform(reg_data_matrix_[i].begin(), reg_data_matrix_[i].end(), curr_vals_.begin(), curr_vals_.begin(),
                   std::plus<double>());

    if (num_lines % num_samples_per_update_ == 0) {
      std::transform(curr_vals_.begin(), curr_vals_.end(), curr_vals_.begin(),
                     std::bind1st(std::multiplies<double>(), 1.0 / num_samples_per_update_));
      update_weights();
      std::fill(curr_vals_.begin(), curr_vals_.end(), 0.0);
    }
    num_lines++;
  }
}

void Online_LR_Weights::OnTimePeriodUpdate(const int num_pages_) {
  if (num_time_period_updates_ > weights_update_dur_) {
    num_time_period_updates_ = 0;
    periodic_weights_update();

  } else {
    if (any_update_) {
      dump_indicators();
      any_update_ = false;
    }
  }
}
}
#endif /* MODELMATH_ONLINE_LR_WEIGHTS_HPP_ */
