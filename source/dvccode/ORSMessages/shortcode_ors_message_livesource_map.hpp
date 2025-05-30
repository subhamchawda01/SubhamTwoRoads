#pragma once

#include <string>
#include <map>
#include <vector>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/ORSMessages/ors_message_livesource.hpp"

namespace HFSAT {

/** this class keeps a map from shortcode to ORSMessageLiveSource *
 */
class ShortcodeORSMessageLivesourceMap {
 private:
  ShortcodeORSMessageLivesourceMap(const ShortcodeORSMessageLivesourceMap&);

 protected:
  std::map<std::string, ORSMessageLiveSource*> shortcode_ors_message_livesource_map_;

  ShortcodeORSMessageLivesourceMap() : shortcode_ors_message_livesource_map_() {}

 public:
  static ShortcodeORSMessageLivesourceMap& GetUniqueInstance() {
    static ShortcodeORSMessageLivesourceMap uniqueinstance_;
    return uniqueinstance_;
  }

  static ORSMessageLiveSource* StaticGetORSMessageLiveSource(const std::string& _shortcode_) {
    return GetUniqueInstance().GetORSMessageLiveSource(_shortcode_);
  }

  static void StaticCheckValid(const std::string& _shortcode_) { return GetUniqueInstance().CheckValid(_shortcode_); }

  ~ShortcodeORSMessageLivesourceMap(){};

  inline void AddEntry(const std::string& _shortcode_, ORSMessageLiveSource* p_ors_message_livesource_) {
    if (p_ors_message_livesource_ != NULL) {
      shortcode_ors_message_livesource_map_[_shortcode_] = p_ors_message_livesource_;
    }
  }

  ORSMessageLiveSource* GetORSMessageLiveSource(const std::string& _shortcode_) {
    std::map<std::string, ORSMessageLiveSource*>::const_iterator _citer_ =
        shortcode_ors_message_livesource_map_.find(_shortcode_);
    if (_citer_ != shortcode_ors_message_livesource_map_.end()) return _citer_->second;
    return NULL;
  }

  void CheckValid(const std::string& _shortcode_) {
    std::map<std::string, ORSMessageLiveSource*>::const_iterator _citer_ =
        shortcode_ors_message_livesource_map_.find(_shortcode_);
    if (_citer_ != shortcode_ors_message_livesource_map_.end()) return;
    ExitVerbose(kShortcodeORSMessageLivesourceMapNoSMVInMap);
    return;
  }

  void GetORSMessageLiveSourceVec(const std::vector<std::string>& _shortcode_vec_,
                                  std::vector<ORSMessageLiveSource*>& _smvvec_) {
    _smvvec_.clear();  // just for safety .. typically we expect empty vectors here
    for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
      _smvvec_.push_back(GetORSMessageLiveSource(_shortcode_vec_[i]));
    }
  }
};
}
