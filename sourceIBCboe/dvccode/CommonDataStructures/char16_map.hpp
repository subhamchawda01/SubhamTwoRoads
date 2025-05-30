/**
    \file dvccode/CommonDataStructures/char16_map.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_CHAR16_MAP_H
#define BASE_COMMONDATASTRUCTURES_CHAR16_MAP_H

#include <tr1/unordered_map>
#include "dvccode/CommonDataStructures/char16_hash_struct.hpp"

namespace HFSAT {

/// \brief a hash_map implementation that takes 16 byte char string and casts to 2 size_t(64bit) and then uses the
/// compare operator on that representation, hoping it is faster than strncmp(16)
class Char16Map {
 protected:
  std::tr1::unordered_map<Char16HashStruct, int, Char16HashStructPkg> secname_map_;
  std::vector<const char*> secname_vec_;   /**< local storage of exchange symbols used as sources */
  std::vector<std::string> shortcode_vec_; /**< local storage of shortcodes used as sources */

  static Char16Map uniqueinstance_;

 public:
  Char16Map() : secname_map_(), secname_vec_(), shortcode_vec_() {}

  void AddString(const char* p_secname_, const std::string& _shortcode_, const int _value_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);  // using bzero
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      secname_map_[_temp_snhs_] = _value_;
      secname_vec_.push_back(p_secname_);
      shortcode_vec_.push_back(_shortcode_);
    }
  }

  /// Returns the mapped value given exactly 16 length string
  inline int GetIdFromChar16(const char* p_secname_) {
    Char16HashStruct _temp_snhs_(
        p_secname_);  // trying to be faster and not use bzero, assume source is 16 char exactly
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return -1;
    } else {
      return secname_map_[_temp_snhs_];
    }
  }

  /// Returns the mapped value given upto ( not exact ) 16 length string, hence need to zero out other bytes
  inline int GetIdFromSecname(const char* p_secname_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);  // using bzero
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return -1;
    } else {
      return secname_map_[_temp_snhs_];
    }
  }

  /// Returns true/false existance based on string code
  inline bool HasString(const std::string& _shortcode_) {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (shortcode_vec_[i] == _shortcode_) {
        return true;
      }
    }
    return false;
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_CHAR16_MAP_H
