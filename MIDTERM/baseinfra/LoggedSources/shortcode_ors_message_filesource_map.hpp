#pragma once

#include <string>
#include <map>
#include <vector>

#include "dvccode/CDef/error_utils.hpp"
#include "baseinfra/LoggedSources/ors_message_filesource.hpp"

namespace HFSAT {

/** this class keeps a map from shortcode to ORSMessageFileSource *
 */
class ShortcodeORSMessageFilesourceMap {
 private:
  ShortcodeORSMessageFilesourceMap(const ShortcodeORSMessageFilesourceMap&);

 protected:
  std::map<std::string, ORSMessageFileSource*> shortcode_ors_message_filesource_map_;

  ShortcodeORSMessageFilesourceMap() : shortcode_ors_message_filesource_map_() {}

 public:
  static ShortcodeORSMessageFilesourceMap& GetUniqueInstance() {
    static ShortcodeORSMessageFilesourceMap uniqueinstance_;
    return uniqueinstance_;
  }

  static ORSMessageFileSource* StaticGetORSMessageFileSource(const std::string& _shortcode_) {
    return GetUniqueInstance().GetORSMessageFileSource(_shortcode_);
  }

  static void StaticCheckValid(const std::string& _shortcode_) { return GetUniqueInstance().CheckValid(_shortcode_); }

  ~ShortcodeORSMessageFilesourceMap(){};

  inline void AddEntry(const std::string& _shortcode_, ORSMessageFileSource* p_ors_message_filesource_) {
    if (p_ors_message_filesource_ != NULL) {
      shortcode_ors_message_filesource_map_[_shortcode_] = p_ors_message_filesource_;
    }
  }

  ORSMessageFileSource* GetORSMessageFileSource(const std::string& _shortcode_) {
    std::map<std::string, ORSMessageFileSource*>::const_iterator _citer_ =
        shortcode_ors_message_filesource_map_.find(_shortcode_);
    if (_citer_ != shortcode_ors_message_filesource_map_.end()) return _citer_->second;
    return NULL;
  }

  void CheckValid(const std::string& _shortcode_) {
    std::map<std::string, ORSMessageFileSource*>::const_iterator _citer_ =
        shortcode_ors_message_filesource_map_.find(_shortcode_);
    if (_citer_ != shortcode_ors_message_filesource_map_.end()) return;
    ExitVerbose(kShortcodeORSMessageFilesourceMapNoSMVInMap);
    return;
  }

  void GetORSMessageFileSourceVec(const std::vector<std::string>& _shortcode_vec_,
                                  std::vector<ORSMessageFileSource*>& _smvvec_) {
    _smvvec_.clear();  // just for safety .. typically we expect empty vectors here
    for (auto i = 0u; i < _shortcode_vec_.size(); i++) {
      _smvvec_.push_back(GetORSMessageFileSource(_shortcode_vec_[i]));
    }
  }
};
}
