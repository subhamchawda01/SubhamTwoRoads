#ifndef _SAFE_ARRAY_H_
#define _SAFE_ARRAY_H_
#include <tr1/unordered_map>

namespace HFSAT {
const int SIZE = 15000;

template <typename T>
class SafeArray {
 private:
  T arr_[SIZE];
  int offset_;

 public:
  SafeArray() {
    offset_ = 0;
    for (auto i = 0u; i < SIZE; ++i) {
      arr_[i - offset_] = 0;
    }
  }

  ~SafeArray() {}

  SafeArray(int _offset_, std::tr1::unordered_map<int, T> _arr_) {
    offset_ = _offset_;
    for (auto &it : _arr_) {
      arr_[it.first - offset_] = it.second;
    }
  }

  SafeArray(const SafeArray &_safe_arr_) {
    offset_ = _safe_arr_.offset_;
    for (unsigned int i = offset_; i - offset_ < SIZE; ++i) {
      arr_[i - offset_] = _safe_arr_.arr_[i - offset_];
    }
  }

  T &operator[](int i) {
    if (i - offset_ < SIZE) {
      return arr_[i - offset_];
    } else {
      return arr_[0];
    }
  }

  bool empty() const { return offset_ == 0; }
};
}

#endif
