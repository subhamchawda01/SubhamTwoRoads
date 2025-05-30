// =====================================================================================
//
//       Filename:  non_linear_wrapper.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/14/2013 05:07:25 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#ifndef BAX_MODEL_MATCH_NON_LINEAR_WRAPPER_H
#define BAX_MODEL_MATCH_NON_LINEAR_WRAPPER_H

#include <vector>
#include <algorithm>

namespace HFSAT {

struct BasicWrapper {
 public:
  std::string indicator_desc_;
  int indicator_index_;
  double constant_value_;
  bool sign_;

  BasicWrapper(const std::string& _indicator_desc_, const int& _indicator_index_, const double& _constant_value_,
               const bool& _sign_)
      : indicator_desc_(_indicator_desc_),
        indicator_index_(_indicator_index_),
        constant_value_(_constant_value_),
        sign_(_sign_) {}

  double GetElementValueFromIndicatorValue(double _new_value_) {
    if (sign_) {
      return std::max(0.0, (constant_value_ - _new_value_));
    }

    return std::max(0.0, (_new_value_ - constant_value_));
  }

  /// for debug purposes
  std::string getStringDesc() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "h(";
    if (sign_) {
      t_temp_oss_ << constant_value_ << "-" << indicator_desc_ << ")";
    } else {
      t_temp_oss_ << indicator_desc_ << "-" << constant_value_ << ")";
    }
    return (t_temp_oss_.str());
  }
};

class NonLinearWrapper {
 private:
  double weight_;
  std::vector<BasicWrapper*> non_linear_wrapper_vec_;
  std::vector<double> prev_values_;
  std::vector<double> raw_ind_values_;
  double last_value_;

 public:
  NonLinearWrapper(const double& _weight_) : weight_(_weight_), non_linear_wrapper_vec_() { last_value_ = 0; }

  // for debug
  std::string getStringDesc() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << weight_ << "     ";
    for (auto i = 0u; i < non_linear_wrapper_vec_.size(); i++) {
      t_temp_oss_ << (non_linear_wrapper_vec_[i]->getStringDesc());
    }
    return (t_temp_oss_.str());
  }

  // for debug
  double lastValue() { return last_value_; }

  std::string dumpValues() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Values for " << getStringDesc() << "\n";
    t_temp_oss_ << "Raw Indicator Values\t";
    for (auto i = 0u; i < prev_values_.size(); i++) {
      t_temp_oss_ << raw_ind_values_[i] << "\t";
    }
    t_temp_oss_ << "\nFiltered h values\t";
    for (auto i = 0u; i < prev_values_.size(); i++) {
      t_temp_oss_ << prev_values_[i] << "\t";
    }
    t_temp_oss_ << "\nCombined Value\t" << last_value_ << "\n\n";
    return (t_temp_oss_.str());
  }

  void AddBasicWrapper(BasicWrapper* this_basic_wrapper_) {
    non_linear_wrapper_vec_.push_back(this_basic_wrapper_);
    prev_values_.push_back(0.0);
    raw_ind_values_.push_back(0.0);
  }

  int TotalBaseElements() { return non_linear_wrapper_vec_.size(); }

  int GetIndicatorIndexByElementIndex(int _this_element_index_) {
    if (_this_element_index_ >= 0 && _this_element_index_ < (int)non_linear_wrapper_vec_.size()) {
      return non_linear_wrapper_vec_[_this_element_index_]->indicator_index_;
    }

    return -1;
  }

  int GetIndicatorConstantValueByElementIndex(int _this_element_index_) {
    if (_this_element_index_ >= 0 && _this_element_index_ < (int)non_linear_wrapper_vec_.size()) {
      return non_linear_wrapper_vec_[_this_element_index_]->constant_value_;
    }

    return -1;
  }

  void MultiplyWeight(const double weight) { weight_ *= weight; }

  double GetComputedValueFromNewIndicatorValue(double new_indicator_value_, int indicator_index_) {
    double _this_value_ = weight_;

    for (unsigned int base_element_counter_ = 0; base_element_counter_ < non_linear_wrapper_vec_.size();
         base_element_counter_++) {
      // affected base wrapper, compute new value
      if (non_linear_wrapper_vec_[base_element_counter_]->indicator_index_ == indicator_index_) {
        double this_new_value_for_element_ =
            non_linear_wrapper_vec_[base_element_counter_]->GetElementValueFromIndicatorValue(new_indicator_value_);

        prev_values_[base_element_counter_] = this_new_value_for_element_;
        raw_ind_values_[base_element_counter_] = new_indicator_value_;
      }

      _this_value_ *= prev_values_[base_element_counter_];
    }
    last_value_ = _this_value_;
    return _this_value_;
  }
};
}

#endif
