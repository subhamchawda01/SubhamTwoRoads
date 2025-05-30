#ifndef _ENHANCED_SECURITY_NAME_INDEXER_HPP_
#define _ENHANCED_SECURITY_NAME_INDEXER_HPP_

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

namespace HFSAT {

class EnhancedSecurityNameIndexer : private HFSAT::SecurityNameIndexer {
 public:
  EnhancedSecurityNameIndexer();
  ~EnhancedSecurityNameIndexer();

  void AddString(const char* secname, const std::string& shortcode);
  void RemoveString(const char* secname, const std::string& shortcode);
  const char* GetShortcodeFromId(unsigned int key_value_) const;

  using HFSAT::SecurityNameIndexer::GetIdFromSecname;  // Declaring Base Function public, inherited privately
  using HFSAT::SecurityNameIndexer::GetIdFromString;   // Declaring Base Function public, inherited privately
};
}

#endif
