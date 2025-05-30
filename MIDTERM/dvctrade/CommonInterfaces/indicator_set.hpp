/*
 * indicator_set.hpp
 *
 *  Created on: Aug 13, 2015
 *      Author: archit
 */

#ifndef DVCCODE_COMMONDATASTRUCTURES_INDICATOR_SET_HPP_
#define DVCCODE_COMMONDATASTRUCTURES_INDICATOR_SET_HPP_

#include "dvctrade/CommonInterfaces/stringable.hpp"

namespace HFSAT {
class Indicator;

class ModelLine : public Stringable {
 public:
  std::string line_;
  const Indicator* p_ind_;
  ModelLine(const std::string& _line_, const Indicator* _p_ind_) : line_(_line_), p_ind_(_p_ind_) {}
  virtual std::string ToString() const { return line_; }
};

class Indicator : public ModelLine {
 public:
  int ind_idx_;
  Indicator(const std::string& _ind_str_, int _idx_) : ModelLine(_ind_str_, this), ind_idx_(_idx_) {}
};

class IndicatorSet : public Stringable {
 protected:
  std::vector<Indicator*> p_ind_vec_;

 public:
  IndicatorSet() : p_ind_vec_() {}
  virtual void RemoveIndicatorByIdx(int _ind_idx_) = 0;
  virtual const std::vector<Indicator*>& GetIndPtrVec() const { return p_ind_vec_; }
  virtual ~IndicatorSet() {
    for (auto i = 0u; i < p_ind_vec_.size(); i++) {
      if (p_ind_vec_[i]) {
        delete p_ind_vec_[i];
      }
    }
  };
};

} /* namespace HFSAT */

#endif /* DVCCODE_COMMONDATASTRUCTURES_INDICATOR_SET_HPP_ */
