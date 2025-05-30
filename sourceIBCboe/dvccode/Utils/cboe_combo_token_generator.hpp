#pragma once

#include <map>
#include <sys/time.h>
#include <unordered_map>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

#define CBOE_COMBO_TOKEN_FILE "/spare/local/tradeinfo/CBOE_Files/combo_token.txt"

class CBOEComboTokenGenerator {
 public:
  static CBOEComboTokenGenerator& GetUniqueInstance();
  int GetTokenOrUpdate(std::string combo);
  std::string GetCombo(int token);
  const char* GetComboChar(int token);
  int Update_New_token_generated(std::string combo);
  std::unordered_map<std::string, int> GetComboToTokenMap();

 private:
  CBOEComboTokenGenerator();
  static CBOEComboTokenGenerator* unique_instance_;
  std::unordered_map<int, std::string> token_combo_;
  std::unordered_map<std::string, int> combo_token_;
  int last_increment_value;
};
}
