/**
    \file testbed/cpptest15.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <vector>
#include <string>
#include <tr1/unordered_map>
#include <iostream>

struct SecurityNameHashStruct {
  // union {
  //   char c_str_ [16];
  size_t size_t_rep_[2];
  // };

  SecurityNameHashStruct(const char* _secname__) {
    size_t_rep_[0] = (size_t)(*((const size_t*)(_secname__)));
    size_t_rep_[1] = (size_t)(*((const size_t*)&(_secname__[8])));
  }

  SecurityNameHashStruct(const char* _secname__, bool) { strncpy((char*)size_t_rep_, _secname__, 16); }

  bool operator==(const SecurityNameHashStruct& _thisword_) const {
    return ((size_t_rep_[0] == _thisword_.size_t_rep_[0]) && (size_t_rep_[1] == _thisword_.size_t_rep_[1]));
  }
};

struct SecurityNameHashStructPkg {
  std::size_t operator()(const SecurityNameHashStruct& key) const { return (key.size_t_rep_[0] + key.size_t_rep_[1]); }
};

typedef std::tr1::unordered_map<SecurityNameHashStruct, int, SecurityNameHashStructPkg> SecurityNameHashStructMap;
typedef SecurityNameHashStructMap::value_type SecurityNameHashStructMapPair;

class SecurityNameIndexer {
 protected:
  SecurityNameHashStructMap secname_map_;
  std::vector<const char*> secname_vec_;
  std::vector<std::string> shortcode_vec_;

 public:
  SecurityNameIndexer() : secname_map_(), secname_vec_(), shortcode_vec_() {}

  void AddString(const char* p_secname_, const std::string _shortcode_) {
    SecurityNameHashStruct _temp_snhs_(p_secname_);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      secname_map_[_temp_snhs_] =
          secname_vec_.size();                // hence secname_vec_ [ secname_map_ [ _temp_snhs_ ] ] = p_secname_
      secname_vec_.push_back(p_secname_);     // or secname_map_ maps char * to a number which maps back to the same
      shortcode_vec_.push_back(_shortcode_);  // string or shrotcode
    }
  }

  inline int GetIdFromChar16(const char* p_secname_) {
    SecurityNameHashStruct _temp_snhs_(p_secname_);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return -1;
    } else {
      return secname_map_[_temp_snhs_];
    }
  }

  inline int GetIdFromSecname(const char* p_secname_) {
    SecurityNameHashStruct _temp_snhs_(p_secname_, true);
    if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
      return -1;
    } else {
      return secname_map_[_temp_snhs_];
    }
  }

  const char* GetSecurityNameFromId(unsigned int key_value_) const { return secname_vec_[key_value_]; }
  const std::string GetShortcodeFromId(unsigned int key_value_) const { return shortcode_vec_[key_value_]; }
};

int main() {
  SecurityNameIndexer secname_indexer_;

  char temp[16];

  bzero(temp, 16);
  strcpy(temp, "FGBS032011");
  secname_indexer_.AddString(temp, "FGBS0");

  bzero(temp, 16);
  strcpy(temp, "FGBM032011");
  secname_indexer_.AddString(temp, "FGBM0");

  bzero(temp, 16);
  strcpy(temp, "FGBL032011");
  secname_indexer_.AddString(temp, "FGBL0");

  bzero(temp, 16);
  strcpy(temp, "FGBX032011");
  secname_indexer_.AddString(temp, "FGBX0");

  bzero(temp, 16);
  strcpy(temp, "ZTH11");
  secname_indexer_.AddString(temp, "ZT0");

  bzero(temp, 16);
  strcpy(temp, "ZFH11");
  secname_indexer_.AddString(temp, "ZF0");

  bzero(temp, 16);
  strcpy(temp, "ZNH11");
  secname_indexer_.AddString(temp, "ZN0");

  bzero(temp, 16);
  strcpy(temp, "ZBH11");
  secname_indexer_.AddString(temp, "ZB0");

  char abc[] = "FGBL032011";
  abc[3] = 'S';
  bzero(temp, 16);
  strcpy(temp, abc);

  int i = 0;

  i = secname_indexer_.GetIdFromChar16(temp);
  if (i == -1) {
    std::cout << " missing " << abc << std::endl;
  } else {
    std::cout << " found " << secname_indexer_.GetShortcodeFromId(i) << " at " << i << std::endl;
  }

  i = secname_indexer_.GetIdFromSecname(abc);
  if (i == -1) {
    std::cout << " missing " << abc << std::endl;
  } else {
    std::cout << " found " << secname_indexer_.GetShortcodeFromId(i) << " at " << i << std::endl;
  }
  return 0;
}
