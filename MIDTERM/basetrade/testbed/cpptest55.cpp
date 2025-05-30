#include <iostream>
#include <cstdlib>

#include <vector>
#include <map>
#include <algorithm>

/// If the given item _new_item_ is present in _class_vec_, then remove it
template <typename T>
inline bool UniqueVectorRemove(std::vector<T>& _class_vec_, const T& _new_item_) {
  typename std::vector<T>::iterator _class_vec_iter_ = _class_vec_.begin();
  for (; _class_vec_iter_ != _class_vec_.end(); _class_vec_iter_++) {
    if ((*_class_vec_iter_) == _new_item_)  // hence need T to have a comparison operator defined
    {
      _class_vec_iter_ = _class_vec_.erase(_class_vec_iter_);
      return true;
    }
  }
  return false;
}

/// Given a vector and an iterator, get element at that point, send it to back
template <typename T>
inline void SendItemToBack(std::vector<T>& t_input_vec_, typename std::vector<T>::iterator t_iter_) {
  if (t_iter_ != t_input_vec_.end()) {
    T t_item_ = *t_iter_;  // copy this item

    typename std::vector<T>::iterator t_next_iter_ = t_iter_;
    t_next_iter_++;  // advance by 1
    if (t_next_iter_ != t_input_vec_.end()) {
      t_iter_ = t_input_vec_.erase(t_iter_);
      t_input_vec_.push_back(t_item_);
    }
  }
}

/// Function takes two sorted and unique vectors and merges them
/// maintaining sorting and keeping the output unique
template <typename T>
inline void UniqueMergeUniqueSortedVectorsGreater(const std::vector<T>& _T_sorted_vec_1_,
                                                  const std::vector<T>& _T_sorted_vec_2_,
                                                  std::vector<T>& _T_sorted_vec_out_) {
  typename std::vector<T>::const_iterator _tsv1_citer_ = _T_sorted_vec_1_.begin();
  typename std::vector<T>::const_iterator _tsv2_citer_ = _T_sorted_vec_2_.begin();
  while ((_tsv1_citer_ != _T_sorted_vec_1_.end()) || (_tsv2_citer_ != _T_sorted_vec_2_.end())) {
    if (_tsv1_citer_ ==
        _T_sorted_vec_1_.end()) {  // if we are at the end of vector 1, then just cat vector 2 into final and done
      for (; _tsv2_citer_ != _T_sorted_vec_2_.end(); _tsv2_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      }
      break;
    }

    if (_tsv2_citer_ ==
        _T_sorted_vec_2_.end()) {  // if at the end of vector 2, then cat vector 1 to end of _T_sorted_vec_out_ and done
      for (; _tsv1_citer_ != _T_sorted_vec_1_.end(); _tsv1_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      }
      break;
    }

    if (*_tsv2_citer_ >
        *_tsv1_citer_) {  // if top item of vec1 is strictly smaller, then take item from 2 and advance marker
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
    } else if (*_tsv2_citer_ <
               *_tsv1_citer_) {  // if top item of vec2 is strictly smaller, then take item from 1 and advance marker
      _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      _tsv1_citer_++;
    } else {  // both tops are same, then take 1 copy of these items and advance both markers. Sine we know that given
              // vectors are strictly unique this item shall not encountered again
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
      _tsv1_citer_++;
    }
  }
}

/// Function takes two sorted and unique vectors and merges them
/// maintaining sorting and keeping the output unique
template <typename T>
inline void UniqueMergeUniqueSortedVectorsLess(const std::vector<T>& _T_sorted_vec_1_,
                                               const std::vector<T>& _T_sorted_vec_2_,
                                               std::vector<T>& _T_sorted_vec_out_) {
  typename std::vector<T>::const_iterator _tsv1_citer_ = _T_sorted_vec_1_.begin();
  typename std::vector<T>::const_iterator _tsv2_citer_ = _T_sorted_vec_2_.begin();
  while ((_tsv1_citer_ != _T_sorted_vec_1_.end()) || (_tsv2_citer_ != _T_sorted_vec_2_.end())) {
    if (_tsv1_citer_ == _T_sorted_vec_1_.end()) {
      for (; _tsv2_citer_ != _T_sorted_vec_2_.end(); _tsv2_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      }
      break;
    }

    if (_tsv2_citer_ == _T_sorted_vec_2_.end()) {
      for (; _tsv1_citer_ != _T_sorted_vec_1_.end(); _tsv1_citer_++) {
        _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      }
      break;
    }

    if (*_tsv2_citer_ < *_tsv1_citer_) {
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
    } else if (*_tsv2_citer_ > *_tsv1_citer_) {
      _T_sorted_vec_out_.push_back(*_tsv1_citer_);
      _tsv1_citer_++;
    } else {  // *_tsv2_citer_ == *_tsv1_citer_
      _T_sorted_vec_out_.push_back(*_tsv2_citer_);
      _tsv2_citer_++;
      _tsv1_citer_++;
    }
  }
}

int main(int argc, char** argv) {
  std::vector<int> sample_vec_;
  sample_vec_.push_back(1);
  sample_vec_.push_back(2);
  sample_vec_.push_back(3);
  for (size_t i = 0; i < sample_vec_.size(); i++) {
    std::cout << sample_vec_[i] << ' ';
  }
  std::cout << "\n";
  UniqueVectorRemove(sample_vec_, 2);
  for (size_t i = 0; i < sample_vec_.size(); i++) {
    std::cout << sample_vec_[i] << ' ';
  }
  std::cout << "\n";
}
