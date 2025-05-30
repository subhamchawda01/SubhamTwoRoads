/**
    \file dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONDATASTRUCTURES_SIMPLE_SECURITY_SYMBOL_INDEXER_H
#define BASE_COMMONDATASTRUCTURES_SIMPLE_SECURITY_SYMBOL_INDEXER_H

#include <iostream>
#include <map>
#include <vector>
#include <tr1/unordered_map>

#include "dvccode/CommonDataStructures/char16_hash_struct.hpp"

namespace HFSAT {

/// To allow classes like MarginChecker listen to changes in SimpleSecuritySymbolIndexer
class SimpleSecuritySymbolIndexerListener {
 public:
  virtual ~SimpleSecuritySymbolIndexerListener() {}
  virtual void OnAddString(unsigned int t_num_security_id_) = 0;
};

/// \brief a simple sorted vector map implementation that takes 16 byte char string and casts to 4 integers and then
/// uses the compare operator on that representation, hoping it is faster than strncmp
class SimpleSecuritySymbolIndexer {
 private:
  SimpleSecuritySymbolIndexer(const SimpleSecuritySymbolIndexer&);

 protected:
  typedef std::tr1::unordered_map<Char16HashStruct, int, Char16HashStructPkg> Char16HashToIntMap;
  typedef std::tr1::unordered_map<Char16HashStruct, int, Char16HashStructPkg>::iterator Char16HashToIntMapIter_t;
  typedef std::tr1::unordered_map<Char16HashStruct, int, Char16HashStructPkg>::const_iterator Char16HashToIntMapCiter_t;

  Char16HashToIntMap secname_map_;
  std::vector<const char*> secname_vec_;  ///< local storage of exchange symbols used as sources

  std::vector<SimpleSecuritySymbolIndexerListener*> sssi_listener_vec_;

  SimpleSecuritySymbolIndexer() : secname_map_(), secname_vec_(), sssi_listener_vec_() {}

 public:
  static inline SimpleSecuritySymbolIndexer& GetUniqueInstance() {
    static SimpleSecuritySymbolIndexer uniqueinstance_;
    return uniqueinstance_;
  }

  static inline const unsigned int GetNumSecurityId() { return GetUniqueInstance().NumSecurityId(); }

  const unsigned int NumSecurityId() const { return secname_vec_.size(); }

  /// Adds a string to set of exchange symbols mapped. Does not assume that the given const char * refers to a fixed 16
  /// length char array
  void AddString(const char* p_secname_) {
    Char16HashStruct _temp_snhs_(p_secname_, true);
    Char16HashToIntMapCiter_t chciter_ = secname_map_.find(_temp_snhs_);
    if (chciter_ == secname_map_.end()) {
      secname_map_[_temp_snhs_] =
          secname_vec_.size();             // hence secname_vec_ [ secname_map_ [ _temp_snhs_ ] ] = p_secname_
      secname_vec_.push_back(p_secname_);  // or secname_map_ maps char * to a number which maps back to the same string
      // std::cout << "AddString: " << p_secname_ << std::endl;
    }
    NotifySSSIListeners();
  }

  /// Returns security_id_ from exchange/datasource symbol
  /// ASSUMING that the given const char * refers to an exactly 16 length char array i.e. Char16HashStruct
  inline int GetIdFromChar16(const char* p_secname_) const {
    Char16HashStruct _temp_snhs_(p_secname_);
    Char16HashToIntMapCiter_t chciter_ = secname_map_.find(_temp_snhs_);
    if (chciter_ == secname_map_.end()) {
      return -1;
    } else {
      return chciter_->second;
    }
  }

  /// Returns security_id_ from exchange/datasource symbol
  /// ASSUMING that the given const char * refers to a char array of length <= 16 characters,
  /// delimited by NUL character if less than 16 characters in length
  inline int GetIdFromSecname(const char* p_secname_) const {
    Char16HashStruct _temp_snhs_(p_secname_, true);
    Char16HashToIntMapCiter_t chciter_ = secname_map_.find(_temp_snhs_);
    if (chciter_ == secname_map_.end()) {
      return -1;
    } else {
      return chciter_->second;
    }
  }

  /// Returns exchange symbol for the given security_id_
  const char* GetSecuritySymbolFromId(unsigned int key_value_) const { return secname_vec_[key_value_]; }

  void AddSSSIListener(SimpleSecuritySymbolIndexerListener* new_sssi_listener_) {
    sssi_listener_vec_.push_back(new_sssi_listener_);

    for (unsigned int i = 1; i <= NumSecurityId(); ++i) {
      new_sssi_listener_->OnAddString(i);
    }
  }

 private:
  void NotifySSSIListeners() {
    int num_sec_id = NumSecurityId();
    for (auto i = 0u; i < sssi_listener_vec_.size(); i++) {
      sssi_listener_vec_[i]->OnAddString(num_sec_id);
    }
  }
};
}

#endif  // BASE_COMMONDATASTRUCTURES_SIMPLE_SECURITY_SYMBOL_INDEXER_H
