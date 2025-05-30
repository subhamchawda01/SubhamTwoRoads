/**
   \file UtilsCode/liffe_pro_rata.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#include "dvccode/Utils/liffe_pro_rata.hpp"
#include <algorithm>
#include <cmath>

namespace HFSAT {

bool GetLFIFills(std::vector<int> sizes_, std::vector<int> fills_, int trade_size_, bool improve_priority_) {
  return LIFFEProRata::GetFills(sizes_, fills_, trade_size_, improve_priority_, 2);
}

bool GetLFLFills(std::vector<int> sizes_, std::vector<int> fills_, int trade_size_, bool improve_priority_) {
  return LIFFEProRata::GetFills(sizes_, fills_, trade_size_, improve_priority_, 4);
}

bool LIFFEProRata::GetFills(std::vector<int> sizes_, std::vector<int> fills_, int trade_size_, bool improve_priority_,
                            int exp_) {
  int num_ords_ = sizes_.size();

  if (num_ords_ <= 0) {
    return false;
  }

  // Making the copy of sizes_ in ord_sz_ because modified order sizes
  // will be used in subsequent passes
  std::vector<int> ord_sz_(num_ords_, 0);

  // Vector to store running total sizes (queue size ahead)
  // sz_sum_[i] = sizes_[0] + sizes_[1] + ... + sizes_[i]
  std::vector<int> sz_sum_(num_ords_, 0);

  // Vector to store fills allocated
  fills_.resize(num_ords_, 0);

  // Vector to store fill ratios.
  // Since we also need the order index for heap, a pair of
  // fill ratio and order index is required.
  std::vector<std::pair<double, int> > fill_ratio_(num_ords_);

  int size_remaining_ = trade_size_;

  // First assign fills to the improve order
  if (improve_priority_) {
    int improve_cap_ = 100000;  // TODO: fix the upper limit of improve order
    int improve_fill_ = std::min(improve_cap_, sizes_[0]);
    improve_fill_ = std::min(improve_fill_, trade_size_);
    sizes_[0] -= improve_fill_;
    fills_[0] += improve_fill_;
    size_remaining_ -= improve_fill_;
  }

  // Assign initial values in these vectors
  int total_size_ = 0;
  for (int i = 0; i < num_ords_; i++) {
    ord_sz_[i] = sizes_[i];
    total_size_ += ord_sz_[i];
    sz_sum_[i] = total_size_;
  }

  // First pass
  for (int i = 0; i < num_ords_; i++) {
    // Ignore this order of the size_remaining is 0
    if (ord_sz_[i] == 0) {
      fill_ratio_[i] = std::make_pair(0, i);
      continue;
    }

    int first_term_ = total_size_ - sz_sum_[i] + ord_sz_[i];
    int second_term_ = total_size_ - sz_sum_[i];

    double time_prorata_diff_ = pow(first_term_, exp_) - pow(second_term_, exp_);
    double denom_ = pow(total_size_, exp_);

    fill_ratio_[i] = std::make_pair(size_remaining_ * time_prorata_diff_ / denom_, i);

    // Fill = min of order_size_ and floor of fill ratio
    fills_[i] = std::min(ord_sz_[i], (int)fill_ratio_[i].first);
  }

  // Update the size remaining
  for (size_t i = 0; i < sizes_.size(); i++) {
    size_remaining_ -= fills_[i];
  }

  // Subsequent passes
  while (size_remaining_ > 0) {
    // Update the values of the size/qa vectors
    total_size_ = 0;
    for (int i = 0; i < num_ords_; i++) {
      ord_sz_[i] = sizes_[i] - fills_[i];
      total_size_ += ord_sz_[i];
      sz_sum_[i] = total_size_;
    }

    for (int i = 0; i < num_ords_; i++) {
      // Ignore this order of the size_remaining is 0
      if (ord_sz_[i] == 0) {
        fill_ratio_[i] = std::make_pair(0, i);
        continue;
      }

      int first_term_ = total_size_ - sz_sum_[i] + ord_sz_[i];
      int second_term_ = total_size_ - sz_sum_[i];

      double time_prorata_diff_ = pow(first_term_, exp_) - pow(second_term_, exp_);
      double denom_ = pow(total_size_, exp_);

      fill_ratio_[i] = std::make_pair(size_remaining_ * time_prorata_diff_ / denom_, i);
    }

    // Unlike the first pass, subsequent passes require us to round up fill ratios < 1
    // Therefore, we need to sort the orders according to the fill ratio and keep
    // assigning fills as long as we can.
    std::make_heap(fill_ratio_.begin(), fill_ratio_.end());

    while (size_remaining_ > 0 && !fill_ratio_.empty()) {
      std::pair<double, int> ratio_pair_ = *fill_ratio_.begin();

      if (ratio_pair_.first >= 1) {
        fills_[ratio_pair_.second] += (int)ratio_pair_.first;
        size_remaining_ -= (int)ratio_pair_.first;
      } else {
        fills_[ratio_pair_.second]++;
        size_remaining_--;
      }

      std::pop_heap(fill_ratio_.begin(), fill_ratio_.end());
      fill_ratio_.pop_back();
    }
  }

  return true;
}
}
