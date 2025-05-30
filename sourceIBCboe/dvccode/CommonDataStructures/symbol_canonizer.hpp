/**
    \file dvccode/CommonDataStructures/simple_security_name_indexer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_SYMBOL_CANONIZER_H
#define BASE_COMMONDATASTRUCTURES_SYMBOL_CANONIZER_H

#include <vector>
#include "dvccode/CDef/defines.hpp"

namespace HFSAT {

#define DEFAULT_VEC_LEN 256

/// a class that is given a temporarily live const char *, to return a const char * that is the same text but that will
/// be alive perenially
class SymbolCanonizer {
 private:
  SymbolCanonizer(const SymbolCanonizer&);

 public:
  static SymbolCanonizer& GetUniqueInstance() {
    static SymbolCanonizer uniqueinstance_;
    return uniqueinstance_;
  }

  static const char* GetTh(const char* _new_symbol_) { return GetUniqueInstance()._GetTh(_new_symbol_); }

  const char* _GetTh(const char* _new_symbol_) {
    (*(symbol_vec_vec_.back()))[last_vec_top_index_].SetString(_new_symbol_);
    const char* retval = (*(symbol_vec_vec_.back()))[last_vec_top_index_]();

    last_vec_top_index_++;
    if (last_vec_top_index_ == DEFAULT_VEC_LEN) {  // if this file of Char16_t is full then add a new file
      symbol_vec_vec_.push_back(new std::vector<Char16_t>(DEFAULT_VEC_LEN));
      last_vec_top_index_ = 0;  // and sets the last_vec_top_index_ to the point where next item should be added
    }
    return retval;
  }

  ~SymbolCanonizer() {
    for (auto i = 0u; i < symbol_vec_vec_.size(); i++) {
      delete symbol_vec_vec_[i];
    }
    symbol_vec_vec_.clear();
  }

 protected:
 private:
  std::vector<std::vector<Char16_t>*> symbol_vec_vec_;
  unsigned int last_vec_top_index_;

  SymbolCanonizer() : symbol_vec_vec_(), last_vec_top_index_(0) {
    symbol_vec_vec_.push_back(new std::vector<Char16_t>(DEFAULT_VEC_LEN));
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_SYMBOL_CANONIZER_H
