#include <vector>
#include <deque>
#include <iostream>

int main() {
  std::vector<int> intvec_;

  intvec_.push_back(1);
  intvec_.push_back(2);
  intvec_.push_back(3);
  intvec_.push_back(3);
  intvec_.push_back(3);
  intvec_.push_back(6);

  int next_val_ = 3;
  std::vector<int>::iterator _iter_ = intvec_.begin();
  for (; (_iter_ != intvec_.end()) && (next_val_ > *_iter_); _iter_++) {
    std::cout << " skipping " << *_iter_ << std::endl;
  }
  intvec_.insert(_iter_, next_val_);

  for (auto i = 0u; i < intvec_.size(); i++) {
    std::cout << intvec_[i] << " ";
  }
  std::cout << std::endl;

  return 0;
}
