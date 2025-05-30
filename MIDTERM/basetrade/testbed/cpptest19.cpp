#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

struct RetrieveKey {
  template <typename T>
  typename T::first_type operator()(T keyValuePair) const {
    return keyValuePair.first;
  }
};

int main() {
  std::map<int, int, std::greater<int> > intpx_2_sum_bid_unconfirmed_;
  std::map<int, int, std::greater<int> > intpx_2_sum_bid_confirmed_;

  intpx_2_sum_bid_confirmed_[100] = 100;
  intpx_2_sum_bid_confirmed_[101] = 101;
  intpx_2_sum_bid_confirmed_[102] = 102;
  intpx_2_sum_bid_confirmed_[103] = 103;

  intpx_2_sum_bid_unconfirmed_[99] = 9;
  intpx_2_sum_bid_unconfirmed_[100] = 10;

  std::vector<int> _intpx_2_sum_bid_unconfirmed_keys_;
  std::vector<int> _intpx_2_sum_bid_confirmed_keys_;
  std::vector<int> _intpx_2_sum_bid_keys_;

  transform(intpx_2_sum_bid_confirmed_.begin(), intpx_2_sum_bid_confirmed_.end(),
            back_inserter(_intpx_2_sum_bid_confirmed_keys_), RetrieveKey());
  transform(intpx_2_sum_bid_unconfirmed_.begin(), intpx_2_sum_bid_unconfirmed_.end(),
            back_inserter(_intpx_2_sum_bid_unconfirmed_keys_), RetrieveKey());

  // for ( size_t i = 0 ; i < _intpx_2_sum_bid_unconfirmed_keys_.size ( ) ; i ++ ) {
  //   std::cout << _intpx_2_sum_bid_unconfirmed_keys_[i] << std::endl;
  // }
  // for ( size_t i = 0 ; i < _intpx_2_sum_bid_confirmed_keys_.size ( ) ; i ++ ) {
  //   std::cout << _intpx_2_sum_bid_confirmed_keys_[i] << std::endl;
  // }

  std::vector<int>::iterator uncnf_keys_iter = _intpx_2_sum_bid_unconfirmed_keys_.begin();
  std::vector<int>::iterator cnf_keys_iter = _intpx_2_sum_bid_confirmed_keys_.begin();
  while ((uncnf_keys_iter != _intpx_2_sum_bid_unconfirmed_keys_.end()) ||
         (cnf_keys_iter != _intpx_2_sum_bid_confirmed_keys_.end())) {
    if (uncnf_keys_iter == _intpx_2_sum_bid_unconfirmed_keys_.end()) {
      for (; cnf_keys_iter != _intpx_2_sum_bid_confirmed_keys_.end(); cnf_keys_iter++) {
        _intpx_2_sum_bid_keys_.push_back(*cnf_keys_iter);
      }
      break;
    }

    if (cnf_keys_iter == _intpx_2_sum_bid_confirmed_keys_.end()) {
      for (; uncnf_keys_iter != _intpx_2_sum_bid_unconfirmed_keys_.end(); uncnf_keys_iter++) {
        _intpx_2_sum_bid_keys_.push_back(*uncnf_keys_iter);
      }
      break;
    }

    if (*cnf_keys_iter > *uncnf_keys_iter) {
      _intpx_2_sum_bid_keys_.push_back(*cnf_keys_iter);
      cnf_keys_iter++;
    } else if (*cnf_keys_iter < *uncnf_keys_iter) {
      _intpx_2_sum_bid_keys_.push_back(*uncnf_keys_iter);
      uncnf_keys_iter++;
    } else {  // =
      _intpx_2_sum_bid_keys_.push_back(*cnf_keys_iter);
      cnf_keys_iter++;
      uncnf_keys_iter++;
    }
  }

  for (size_t i = 0; i < _intpx_2_sum_bid_keys_.size(); i++) {
    std::cout << _intpx_2_sum_bid_keys_[i] << std::endl;
  }

  return 0;
}
