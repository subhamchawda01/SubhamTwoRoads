/**
    \file dvccode/CommonDataStructures/char8_hash_struct.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_CHAR8_HASH_STRUCT_H
#define BASE_COMMONDATASTRUCTURES_CHAR8_HASH_STRUCT_H

#include <string.h>
#include <strings.h>
#include <vector>
#include <string>
#include <iostream>

namespace HFSAT {

/** \brief Hasher for exactly 8 sized char arrays */
struct Char8HashStruct {
  size_t size_t_rep_;

  Char8HashStruct(const char* _secname__) { size_t_rep_ = (size_t)(*((const size_t*)(_secname__))); }

  Char8HashStruct(const char* _secname__, bool) { strncpy((char*)&size_t_rep_, _secname__, 8); }

  inline bool operator==(const Char8HashStruct& _thisword_) const { return (size_t_rep_ == _thisword_.size_t_rep_); }

  /** operator strict less than '<' ... for std::map */
  inline bool operator<(const Char8HashStruct& _word2_) {
    if (size_t_rep_ < _word2_.size_t_rep_) {
      return true;
    }
    if (size_t_rep_ > _word2_.size_t_rep_) {
      return false;
    }

    return false;
  }
};

struct Char8HashStructPkg {
  inline std::size_t operator()(const Char8HashStruct& key) const { return key.size_t_rep_; }
};
}
#endif  // BASE_COMMONDATASTRUCTURES_CHAR8_HASH_STRUCT_H
