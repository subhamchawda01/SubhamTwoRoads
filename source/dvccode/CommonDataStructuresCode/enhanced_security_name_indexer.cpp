#include <cstring>

#include "dvccode/CommonDataStructures/enhanced_security_name_indexer.hpp"

namespace HFSAT {

EnhancedSecurityNameIndexer::EnhancedSecurityNameIndexer() : HFSAT::SecurityNameIndexer() {}

EnhancedSecurityNameIndexer::~EnhancedSecurityNameIndexer() { Clear(); }

void EnhancedSecurityNameIndexer::AddString(const char* secname, const std::string& shortcode) {
  Char16HashStruct _temp_snhs_(secname, true);
  if (secname_map_.find(_temp_snhs_) != secname_map_.end()) {
    return;  // Already Added
  }

  // Check if the security is already in secname and shortcode vectors, but only its mapping removed from secname map.
  for (auto i = 0u; i < NumSecurityId(); i++) {
    if (shortcode_vec_[i] == shortcode && (!strcmp(secname, secname_vec_[i].c_str()))) {
      secname_map_[_temp_snhs_] = i;
      return;
    }
  }

  SecurityNameIndexer::AddString(secname, shortcode);
}

void EnhancedSecurityNameIndexer::RemoveString(const char* secname, const std::string& shortcode) {
  Char16HashStruct _temp_snhs_(secname, true);
  if (secname_map_.find(_temp_snhs_) != secname_map_.end()) {
    secname_map_.erase(_temp_snhs_);
  }
}

const char* EnhancedSecurityNameIndexer::GetShortcodeFromId(unsigned int id) const {
  Char16HashStruct _temp_snhs_(GetSecurityNameFromId(id), true);
  if (secname_map_.find(_temp_snhs_) == secname_map_.end()) {
    return nullptr;
  }

  const std::string& shortcode = SecurityNameIndexer::GetShortcodeFromId(id);
  return shortcode.c_str();
}
}
