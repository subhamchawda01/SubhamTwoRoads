#ifndef _CBOE_SPOT_INDEX_GEN_HPP_
#define _CBOE_SPOT_INDEX_GEN_HPP_

#include <map>
#include <sys/time.h>
#include <unordered_map>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

#define CBOE_SPOT_INDEX_TOKEN_FILE "/spare/local/tradeinfo/CBOE_Files/spot_index_token.txt"

class CBOESpotTokenGenerator {
 public:
  static CBOESpotTokenGenerator& GetUniqueInstance();
  int GetTokenOrUpdate(std::string spot_index);
  std::string GetSpotIndex(int token);
  const char* GetSpotIndexChar(int token);
  int Update_New_token_generated(std::string spot_index);
  std::unordered_map<std::string, int> GetSpotIndexToTokenMap();

 private:
  CBOESpotTokenGenerator();
  static CBOESpotTokenGenerator* unique_instance_;
  std::unordered_map<int, std::string> token_spot_index_;
  std::unordered_map<std::string, int> spot_index_token_;
  int last_increment_value;
};
}

#endif
