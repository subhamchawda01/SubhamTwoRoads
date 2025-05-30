/**
    \file dvccode/CommonDataStructures/simple_security_name_indexer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_SIMPLE_SECURITY_NAME_INDEXER_H
#define BASE_COMMONDATASTRUCTURES_SIMPLE_SECURITY_NAME_INDEXER_H

#include <map>
#include "dvccode/CommonDataStructures/char16_hash_struct.hpp"

namespace HFSAT {

/// \brief a simple sorted vector map implementation that takes 16 byte char string and casts to 4 integers and then
/// uses the compare operator on that representation, hoping it is faster than strncmp
class SimpleSecurityNameIndexer {
 protected:
  std::map<Char16HashStruct, int> secname_map_;
  std::vector<const char*> secname_vec_;    ///< local storage of exchange symbols used as sources
  std::vector<std::string> shortcode_vec_;  ///< local storage of shortcodes used as sources

  SimpleSecurityNameIndexer() : secname_map_(), secname_vec_(), shortcode_vec_() {}

 public:
  static inline SimpleSecurityNameIndexer& GetUniqueInstance() {
    static SimpleSecurityNameIndexer uniqueinstance_;
    return uniqueinstance_;
  }
  static inline const unsigned int GetNumSecurityId() { return GetUniqueInstance().NumSecurityId(); }

  const unsigned int NumSecurityId() const { return secname_vec_.size(); }

  /// Adds a string to set of exchange symbols mapped. Does not assume that the given const char * refers to a fixed 16
  /// length char array
  void AddString(const char* p_secname_, const std::string& _shortcode_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      secname_map_[_temp_snhs_] =
          secname_vec_.size();                // hence secname_vec_ [ secname_map_ [ _temp_snhs_ ] ] = p_secname_
      secname_vec_.push_back(p_secname_);     // or secname_map_ maps char * to a number which maps back to the same
      shortcode_vec_.push_back(_shortcode_);  // string or shrotcode
    }
  }

  /// Returns security_id_ from exchange/datasource symbol
  /// ASSUMING that the given const char * refers to an exactly 16 length char array
  inline int GetIdFromChar16(const char* p_secname_) {
    Char16HashStruct _temp_snhs_(p_secname_);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return -1;
    } else {
      return secname_map_[_temp_snhs_];
    }
  }

  /// Returns security_id_ from exchange/datasource symbol
  /// ASSUMING that the given const char * refers to a char array of length <= 16 characters,
  /// delimited by NULL character if less than 16 characters in length
  inline int GetIdFromSecname(const char* p_secname_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return -1;
    } else {
      return secname_map_[_temp_snhs_];
    }
  }

  /// Returns security_id_ from shortcode
  inline int GetIdFromString(const std::string& _shortcode_) {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (shortcode_vec_[i] == _shortcode_) {
        return i;
      }
    }
    return -1;
  }

  /// Returns exchange symbol for the given security_id_
  const char* GetSecurityNameFromId(unsigned int key_value_) const { return secname_vec_[key_value_]; }
  /// Returns the shortcode for the given security_id_
  const std::string& GetShortcodeFromId(unsigned int key_value_) const { return shortcode_vec_[key_value_]; }

  // Returns true if the given shrotcode is one of those mapped
  bool HasString(const std::string& _shortcode_) {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (shortcode_vec_[i] == _shortcode_) {
        return true;
      }
    }
    return false;
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_SIMPLE_SECURITY_NAME_INDEXER_H
