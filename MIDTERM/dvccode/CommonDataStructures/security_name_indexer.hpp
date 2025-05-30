/**
    \file dvccode/CommonDataStructures/security_name_indexer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_SECURITY_NAME_INDEXER_H
#define BASE_COMMONDATASTRUCTURES_SECURITY_NAME_INDEXER_H

#include <tr1/unordered_map>
#include "dvccode/CommonDataStructures/char16_hash_struct.hpp"

namespace HFSAT {

typedef std::tr1::unordered_map<Char16HashStruct, int, Char16HashStructPkg> Char16HashStructIntMap;
typedef std::tr1::unordered_map<Char16HashStruct, int, Char16HashStructPkg>::const_iterator
    Char16HashStructIntMapCIter_t;

/** \brief a hash_map implementation that takes 16 byte char string and casts to 4 integers and then uses the compare
 * operator on that representation, hoping it is faster than strncmp
 */
class SecurityNameIndexer {
 private:
  /// Added copy constructor to disable it
  SecurityNameIndexer(const SecurityNameIndexer&);

 protected:
  Char16HashStructIntMap secname_map_;
  std::vector<std::string> secname_vec_;   /**< local storage of exchange symbols used as sources */
  std::vector<std::string> shortcode_vec_; /**< local storage of shortcodes used as sources */

  SecurityNameIndexer() : secname_map_(), secname_vec_(), shortcode_vec_() {}

 public:
  static inline SecurityNameIndexer& GetUniqueInstance() {
    static SecurityNameIndexer uniqueinstance_;
    return uniqueinstance_;
  }

  static inline unsigned int GetNumSecurityId() { return GetUniqueInstance().NumSecurityId(); }

  unsigned int NumSecurityId() const { return secname_vec_.size(); }

  /// Adds a string to set of exchange symbols mapped. Does not assume that the given const char * refers to a fixed 16
  /// length char array
  void AddString(const char* p_secname_, const std::string& _shortcode_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      secname_map_[_temp_snhs_] =
          secname_vec_.size();                // hence secname_vec_ [ secname_map_ [ _temp_snhs_ ] ] = p_secname_
      secname_vec_.push_back(p_secname_);     // or secname_map_ maps char * to a number which maps back to the same
      shortcode_vec_.push_back(_shortcode_);  // string or shortcode
    }
  }

  void Clear() {
    secname_map_.clear();
    secname_vec_.clear();
    shortcode_vec_.clear();
  }

  /// Returns security_id_ from exchange/datasource symbol ASSUMING that the given const char * refers to a fixed 16
  /// length char array
  inline int GetIdFromChar16(const char* p_secname_) const {
    const Char16HashStruct _temp_snhs_(p_secname_);
    Char16HashStructIntMapCIter_t t_citer_ = secname_map_.find(_temp_snhs_);
    if (t_citer_ == secname_map_.end()) {
      return -1;
    } else {
      return t_citer_->second;
    }
  }

  /// Returns security_id_ from exchange/datasource symbol but does not assume that the given const char * refers to a
  /// fixed 16 length char array
  inline int GetIdFromSecname(const char* p_secname_) const {
    // Return -1 if the pointer is NULL
    if (!p_secname_) return -1;

    const Char16HashStruct _temp_snhs_(p_secname_, true);
    Char16HashStructIntMapCIter_t t_citer_ = secname_map_.find(_temp_snhs_);
    if (t_citer_ == secname_map_.end()) {
      return -1;
    } else {
      return t_citer_->second;
    }
  }

  /// Returns security_id_ from shortcode
  inline int GetIdFromString(const std::string& _shortcode_) const {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (shortcode_vec_[i] == _shortcode_) {
        return i;
      }
    }
    return -1;
  }

  /// Returns exchange symbol for the given security_id_
  const char* GetSecurityNameFromId(unsigned int key_value_) const { return secname_vec_[key_value_].c_str(); }
  /// Returns the shortcode for the given security_id_
  const std::string& GetShortcodeFromId(unsigned int key_value_) const { return shortcode_vec_[key_value_]; }

  // Returns true if the given shrotcode is one of those mapped
  bool HasString(const std::string& _shortcode_) const {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (shortcode_vec_[i] == _shortcode_) {
        return true;
      }
    }
    return false;
  }

  // Returns true if the given shrotcode_prefix is one of those mapped
  bool HasShortCodePrefix(const std::string& _shortcode_prefix_) const {
    for (auto i = 0u; i < shortcode_vec_.size(); i++) {
      if (strncmp(shortcode_vec_[i].c_str(), _shortcode_prefix_.c_str(), _shortcode_prefix_.size()) == 0) {
        return true;
      }
    }
    return false;
  }

  // Returns true if the given secname_prefix is one of those mapped
  bool HasSecNamePrefix(const std::string& _secname_prefix_) const {
    for (auto i = 0u; i < secname_vec_.size(); i++) {
      if (strncmp(secname_vec_[i].c_str(), _secname_prefix_.c_str(), _secname_prefix_.size()) == 0) {
        return true;
      }
    }
    return false;
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_SECURITY_NAME_INDEXER_H
