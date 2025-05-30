/**
    \file dvccode/CommonDataStructures/char16_hash_struct.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_CHAR16_HASH_STRUCT_H
#define BASE_COMMONDATASTRUCTURES_CHAR16_HASH_STRUCT_H

#include <string.h>
#include <strings.h>
#include <vector>
#include <string>
#include <iostream>

namespace HFSAT {

/// \brief Hasher for exactly 16 sized char arrays
struct Char16HashStruct {
  size_t size_t_rep_[2];

  /// constructor assuming that the given char * is one of 16 length char array
  Char16HashStruct(const char* _secname__) {
    size_t_rep_[0] = (size_t)(*((const size_t*)(_secname__)));
    size_t_rep_[1] = (size_t)(*((const size_t*)&(_secname__[8])));
  }

  /// constructor wthout assuming anything about the length of the char array the given pointer argument points to
  Char16HashStruct(const char* _secname__, bool) {
    // zero out the space .. hopefully this works ... TODO check
    size_t_rep_[0] = 0;
    size_t_rep_[1] = 0;
    // use strncpy to be sure of size
    // Here 16 is an assumption that Exchange Symbol Manager returns strictly <=16 char string including NULL char
    strncpy((char*)size_t_rep_, _secname__, 16);
  }

  inline bool operator==(const Char16HashStruct& _thisword_) const {
    return ((size_t_rep_[0] == _thisword_.size_t_rep_[0]) && (size_t_rep_[1] == _thisword_.size_t_rep_[1]));
  }

  /** operator strict less than '<' ... for std::map */
  inline bool operator<(const Char16HashStruct& _word2_) const {
    if (size_t_rep_[0] < _word2_.size_t_rep_[0]) {
      return true;
    }
    if (size_t_rep_[0] > _word2_.size_t_rep_[0]) {
      return false;
    }

    if (size_t_rep_[1] < _word2_.size_t_rep_[1]) {
      return true;
    }
    if (size_t_rep_[1] > _word2_.size_t_rep_[1]) {
      return false;
    }

    return false;
  }
};

struct Char16HashStructPkg {
  inline std::size_t operator()(const Char16HashStruct& key) const { return (key.size_t_rep_[0] + key.size_t_rep_[1]); }
};
}
#endif  // BASE_COMMONDATASTRUCTURES_CHAR16_HASH_STRUCT_H
