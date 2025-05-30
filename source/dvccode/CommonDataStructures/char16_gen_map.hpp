/**
    \file dvccode/CommonDataStructures/char16_gen_map.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_CHAR16_GEN_MAP_H
#define BASE_COMMONDATASTRUCTURES_CHAR16_GEN_MAP_H

#include <tr1/unordered_map>
#include "dvccode/CommonDataStructures/char16_hash_struct.hpp"

namespace HFSAT {

/// \brief a hash_map implementation that takes 16 byte char string
/// and casts to 2 size_t(64bit) and then uses the compare operator
/// on that representation, hoping it is faster than strncmp(16)
template <typename T>
class Char16GenMap {
 protected:
  std::tr1::unordered_map<Char16HashStruct, T, Char16HashStructPkg> secname_map_;
  std::vector<const char*> secname_vec_;  ///< local storage of strings used as keys

 public:
  Char16GenMap() : secname_map_(), secname_vec_() {}

  void AddString(const char* p_secname_, const T _value_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);  // using bzero
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      secname_map_[_temp_snhs_] = _value_;
      char* local_p_secname_ = new char[16];
      memcpy(local_p_secname_, p_secname_, 16);
      secname_vec_.push_back(local_p_secname_);
    }
  }

  /// Returns the mapped value given exactly 16 length string
  inline bool GetIdFromChar16(const char* p_secname_, T& _out_val_) {
    // trying to be faster and not use bzero, assume source is 16 char exactly
    Char16HashStruct _temp_snhs_(p_secname_);

    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return false;
    } else {
      _out_val_ = secname_map_[_temp_snhs_];
      return true;
    }
  }

  /// Returns the mapped value given exactly 16 length string
  inline T* GetIdRefFromChar16(const char* p_secname_) {
    // trying to be faster and not use bzero, assume source is 16 char exactly
    Char16HashStruct _temp_snhs_(p_secname_);

    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return NULL;
    } else {
      return &(secname_map_[_temp_snhs_]);
    }
  }

  /// Returns the mapped value given upto ( not exact ) 16 length string,
  /// hence need to zero out other bytes
  inline bool GetIdFromSecname(const char* p_secname_, T& _out_val_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);  // using bzero
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return false;
    } else {
      _out_val_ = secname_map_[_temp_snhs_];
      return true;
    }
  }

  inline const std::vector<const char*>& GetKeys() { return secname_vec_; }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_CHAR16_GEN_MAP_H
