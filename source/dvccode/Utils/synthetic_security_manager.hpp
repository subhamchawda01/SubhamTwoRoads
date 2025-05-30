#ifndef SYNTHETIC_SEC_MANAGER
#define SYNTHETIC_SEC_MANAGER

#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <iostream>
#include <fstream>
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/safe_array.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {

class SyntheticSecurityManager {
 protected:
  std::map<std::string, std::vector<std::string>> synth_shc_to_const_shc_vec_;
  std::map<std::string, std::vector<double>> synth_shc_to_const_weight_vec_;
  std::tr1::unordered_set<std::string> synth_secs_;

 protected:
  static SyntheticSecurityManager* p_uniqueinstance_;

  SyntheticSecurityManager();

 public:
  static SyntheticSecurityManager& GetUniqueInstance() {
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new SyntheticSecurityManager();
    }
    return *p_uniqueinstance_;
  }

 public:
  bool IsSyntheticSecurity(std::string _shc_) { return (synth_secs_.find(_shc_) != synth_secs_.end()); }

  std::vector<std::string> GetConstituentSHC(std::string _synth_shc_) {
    return synth_shc_to_const_shc_vec_[_synth_shc_];
  }

  std::vector<double> GetConstituentWeights(std::string _synth_shc_) {
    return synth_shc_to_const_weight_vec_[_synth_shc_];
  }

  void LoadSynthVecs();
};
}

#endif
